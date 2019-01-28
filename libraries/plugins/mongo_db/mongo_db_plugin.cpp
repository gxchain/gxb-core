
#include <graphene/mongo_db/mongo_db_plugin.hpp>
#include <graphene/chain/database.hpp>
#include <graphene/chain/abi_serializer.hpp>
#include <graphene/chain/action_history_object.hpp>

#include <fc/log/console_appender.hpp>
#include <fc/log/file_appender.hpp>
#include <fc/log/logger.hpp>
#include <fc/log/logger_config.hpp>

#include <fc/io/json.hpp>
#include <fc/utf8.hpp>
#include <fc/variant.hpp>

#include <boost/algorithm/string.hpp>
#include <boost/chrono.hpp>
#include <boost/signals2/connection.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition_variable.hpp>

#include <queue>

#include <bsoncxx/builder/basic/kvp.hpp>
#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/exception/exception.hpp>
#include <bsoncxx/json.hpp>

#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/pool.hpp>
#include <mongocxx/exception/operation_exception.hpp>
#include <mongocxx/exception/logic_error.hpp>

namespace graphene { namespace mongo_db {

using namespace chain;
namespace detail {
    class mongo_db_plugin_impl{
        public:
            mongo_db_plugin_impl(mongo_db_plugin& plugin):_self(plugin){}
            virtual ~mongo_db_plugin_impl(){
                try {
                    ilog( "mongo_db_plugin shutdown in process please be patient this can take a few minutes" );
                    done = true;
                    condition.notify_one();

                    consume_thread.join();

                    mongo_pool.reset();
                } catch( std::exception& e ) {
                    elog( "Exception on mongo_db_plugin shutdown of consume thread: ${e}", ("e", e.what()));
                }
            }

            void update_action_histories( const signed_block& b );
            void remove_action_histories( uint32_t num);

            void init();
            void consume_blocks();                      // consume data(actions、inline_traces)
            void process_action_trace(account_action_history_object& act);
            void wipe_database();
            static std::vector<account_action_history_object> get_action_history_mongodb(app::get_action_history_params params);

            fc::optional<abi_serializer> get_abi_serializer( uint64_t accid );
            template<typename T> fc::variant to_variant_with_abi( const T& obj );
            template<typename Queue, typename Entry> void queue(Queue& queue, const Entry e);

            // link to mongo_server
            static std::string db_name;
            mongocxx::instance instance;
            static std::shared_ptr<mongocxx::pool> mongo_pool;

            // collections consum thread
            mongocxx::collection _action_traces;        // actions(account、action、inline_traces)

            // consume thread 
            boost::thread consume_thread;
            boost::mutex mtx;
            boost::condition_variable condition;

            // production queue
            size_t max_queue_size = 0;
            int queue_sleep_time = 0;
            std::atomic_bool done{false};

            // deque(cache by production and consumption)
            std::deque<account_action_history_object> actions_trace_queue;            // production
            std::deque<account_action_history_object> actions_trace_process_queue;    // consumption

            static const std::string action_traces_col;

            // _track_accounts
            flat_set<account_id_type> _tracked_accounts;

            // access db
            mongo_db_plugin& _self;
        private:
            void collect_inline_action_account(const std::vector<action_trace>& inline_traces,std::vector<account_id_type>& collect_accs);
            bool check_trackacc(const account_action_history_object& act_obj);

        };
    const std::string mongo_db_plugin_impl::action_traces_col = "action_traces";

    std::shared_ptr<mongocxx::pool> mongo_db_plugin_impl::mongo_pool = nullptr;
    std::string mongo_db_plugin_impl::db_name = "";

    
    bool mongo_db_plugin_impl::check_trackacc(const account_action_history_object& act_obj)
    {
        bool ret_value = false;
        std::vector<account_id_type> collect_accs;
        collect_accs.push_back(act_obj.sender);
        collect_accs.push_back(act_obj.receiver);
        collect_inline_action_account(act_obj.inline_traces,collect_accs);
        for(auto &itor:collect_accs){
            if(std::find(_tracked_accounts.begin(),_tracked_accounts.end(),itor)!=_tracked_accounts.end())
                ret_value = true;
        }
        return ret_value;
    }
    
    void mongo_db_plugin_impl::collect_inline_action_account(const std::vector<action_trace>& inline_traces,std::vector<account_id_type>& collect_accs)
    {
        if(inline_traces.size()==0)
            return;
        else{
            for(auto & act_tra: inline_traces){
                if(std::find(collect_accs.begin(),collect_accs.end(),act_tra.sender)==collect_accs.end())
                    collect_accs.push_back(act_tra.sender);
                if(std::find(collect_accs.begin(),collect_accs.end(),act_tra.receiver)==collect_accs.end())
                    collect_accs.push_back(act_tra.receiver);
                collect_inline_action_account(act_tra.inline_traces,collect_accs);
            }
        }
    }
    
    std::vector<account_action_history_object> mongo_db_plugin_impl::get_action_history_mongodb(app::get_action_history_params params)
    {
        using namespace bsoncxx::types;
        using bsoncxx::builder::basic::kvp;
        using bsoncxx::builder::basic::make_document;
        using bsoncxx::builder::basic::sub_document;
        using bsoncxx::builder::basic::sub_array;

        auto mongo_client = mongo_pool->acquire();
        auto& mongo_conn = *mongo_client;
        auto _action_traces = mongo_conn[db_name][action_traces_col];

        std::vector<account_action_history_object> result;
        auto filterdoc = bsoncxx::builder::basic::document{};

        mongocxx::options::find opts{};
        opts.limit(params.limit);
        opts.projection(make_document(kvp("_id",0),kvp("action.act.data",0)));

        if(params.reverse) opts.sort(make_document(kvp("_id",-1)));
        else opts.sort(make_document(kvp("_id",1)));

        if(params.txid){
            std::string txid_str(*params.txid);
            filterdoc.append(kvp("action.txid",txid_str));
        }
        else if(params.account_instance_id && *params.account_instance_id){
            fc::variant accid_var;
            uint64_t& acc_instance_id = *params.account_instance_id;
            const account_id_type& acc_id = account_id_type(acc_instance_id);
            fc::to_variant(acc_id,accid_var);
            filterdoc.append(kvp("action.link_accounts",accid_var.as_string()));
        }

        auto cursor = _action_traces.find(filterdoc.view(),opts);
        
        for( auto doc : cursor){
            auto subdoc = doc["action"];
            auto json_doc = bsoncxx::to_json(subdoc.get_document().view());

            // also can modify abi_serializer.hpp  293 line to solved
            FC_ASSERT(json_doc.find("\"account\"") != std::string::npos &&
                json_doc.find("\"name\"") != std::string::npos , "mongdb document type error !");

            json_doc=json_doc.replace(json_doc.find("\"account\""),9,"\"contract_id\"");
            json_doc=json_doc.replace(json_doc.find("\"name\""),6,"\"method_name\"");

            if(json_doc.find("\"hex_data\"") != std::string::npos)
                json_doc=json_doc.replace(json_doc.find("\"hex_data\""),10,"\"data\"");

            auto var_doc = fc::json::from_string(json_doc);
            
            account_action_history_object obj;
            fc::from_variant(var_doc,obj,20);
            result.push_back(obj);
        }
        
        return result;
    }

    fc::optional<abi_serializer> mongo_db_plugin_impl::get_abi_serializer( uint64_t accid ) {
        using bsoncxx::builder::basic::kvp;
        using bsoncxx::builder::basic::make_document;
        try {
            fc::microseconds abi_serializer_max_time = fc::microseconds(1000*1500);
            auto& db =_self.database();
            const account_object& accobj = account_id_type(accid)(db);
            abi_serializer abis;
            abis.set_abi(accobj.abi,abi_serializer_max_time);
            return abis;
        }FC_LOG_AND_RETHROW() 
        
    }

    template<typename T>
    fc::variant mongo_db_plugin_impl::to_variant_with_abi( const T& obj ) {
        fc::variant pretty_output;
        fc::microseconds abi_serializer_max_time = fc::microseconds(1000*1500);
        
        abi_serializer::to_variant( obj, pretty_output,
                                    [&]( uint64_t accid ) { return get_abi_serializer( accid ); },
                                    abi_serializer_max_time );
        
        return pretty_output;
    }
    template<typename Queue, typename Entry>
    void mongo_db_plugin_impl::queue( Queue& queue, const Entry e ) {
        try{
        boost::mutex::scoped_lock lock( mtx );
        auto queue_size = queue.size();
        if( queue_size > max_queue_size ) {
            lock.unlock();
            condition.notify_one();
            queue_sleep_time += 10;
            if( queue_sleep_time > 1000 )
                wlog("queue size: ${q}", ("q", queue_size));
            boost::this_thread::sleep_for( boost::chrono::milliseconds( queue_sleep_time ));
            lock.lock();
        } else {
            queue_sleep_time -= 10;
            if( queue_sleep_time < 0 ) queue_sleep_time = 0;
        }
        queue.emplace_back( e );
        lock.unlock();
        condition.notify_one();
        }FC_LOG_AND_RETHROW() 
    }

    void mongo_db_plugin_impl::remove_action_histories( uint32_t num){
        try{
            graphene::chain::database& db = _self.database();
            const auto& act_idx = db.get_index_type<action_history_index>();
            const auto& by_blocknum_idx = act_idx.indices().get<by_blocknum>();

            if(by_blocknum_idx.size() > 0 &&
                by_blocknum_idx.begin()->block_num < num){
                auto itor = by_blocknum_idx.begin();
                for(;itor->block_num < num;){
                    queue( actions_trace_queue, *itor);
                    ilog("first_num: ${num}, irr_block_num: ${irr_num}, count: ${count}",("num",by_blocknum_idx.begin()->block_num)("irr_num",num)("count",by_blocknum_idx.size()));
                    db.remove(*itor);
                    if(by_blocknum_idx.size() == 0)
                        break;
                    itor = by_blocknum_idx.begin();
                }
            }
        }FC_LOG_AND_RETHROW()
    }

    void mongo_db_plugin_impl::update_action_histories( const signed_block& b ){
        try{
            graphene::chain::database& db = _self.database();
            const vector<optional< account_action_history_object > >& hist = db.get_applied_actions();
            for( const optional< account_action_history_object >& o_act : hist ){
                ilog("block_num: ${num} , senderid: ${sender},method: ${method_name}",("num",o_act->block_num)("sender",o_act->sender)("method_name",o_act->act.method_name));
                if(o_act.valid()){
                    if(_tracked_accounts.size() == 0 || check_trackacc(*o_act))
                    {
                        db.create<account_action_history_object>( [&]( account_action_history_object& h )
                        {
                            h.mongodb_id     = o_act->mongodb_id;
                            h.block_num      = o_act->block_num;
                            h.trx_in_block   = o_act->trx_in_block;
                            h.op_in_trx      = o_act->op_in_trx;
                            h.sender         = o_act->sender;
                            h.receiver       = o_act->receiver;
                            h.act            = o_act->act;
                            h.inline_traces  = o_act->inline_traces;
                            h.result         = o_act->result;
                            h.txid           = o_act->txid;
                            h.irreversible_state = false;
                            std::vector<account_id_type> collect_accs;
                            collect_accs.push_back(o_act->sender);
                            collect_accs.push_back(o_act->receiver);
                            collect_inline_action_account(o_act->inline_traces,collect_accs);
                            h.link_accounts  = collect_accs;        // related account list
                        });
                    }
                }
            }
        }FC_LOG_AND_RETHROW()
    }

    void mongo_db_plugin_impl::process_action_trace( account_action_history_object& act){
        try{
            using namespace bsoncxx::types;
            using bsoncxx::builder::basic::kvp;
            using bsoncxx::builder::basic::make_document;
            using namespace bsoncxx::v_noabi::types;
            auto actions_trans_doc = bsoncxx::builder::basic::document{};
            
            act.irreversible_state = true;
            auto abi_json_str = to_variant_with_abi(act);
            auto json_str = fc::json::to_string(abi_json_str);
            try{
                const auto& value = bsoncxx::from_json(json_str);
                actions_trans_doc.append( kvp("_id",b_int64{static_cast<int64_t>(act.mongodb_id)}));
                actions_trans_doc.append( kvp("action",value));
                  
            } catch (...) {
                ilog( "Unable to parse json ,txid: ${txid}", ( "txid", act.txid ));
                actions_trans_doc.append( kvp("_id",b_int64{static_cast<int64_t>(act.mongodb_id)}));
            }
            auto doc = _action_traces.find_one( make_document( kvp("_id", b_int64{static_cast<int64_t>(act.mongodb_id)})) );
            if(!doc)
                _action_traces.insert_one(actions_trans_doc.view() ) ; 

                        
        }FC_LOG_AND_RETHROW()
    }
    void mongo_db_plugin_impl::wipe_database()
    {
        ilog("mongo db wipe_database");

        auto client = mongo_pool->acquire();
        auto& mongo_conn = *client;

        auto action_traces = mongo_conn[db_name][action_traces_col];
        action_traces.drop();

        ilog("done wipe_database");
    }
    void mongo_db_plugin_impl::consume_blocks(){
        try{
            auto mongo_client = mongo_pool->acquire();
            auto& mongo_conn = *mongo_client;

            _action_traces = mongo_conn[db_name][action_traces_col];

            while(true){
                boost::mutex::scoped_lock lock(mtx);
                while (actions_trace_queue.empty()  && !done){
                    condition.wait(lock);
                }

                // capture for processing
                size_t actions_trace_size = actions_trace_queue.size();
                if ( actions_trace_size > 0) {
                    actions_trace_process_queue = std::move(actions_trace_queue);
                    actions_trace_queue.clear();
                }
                lock.unlock();
                
                // process actions
                auto start_time = fc::time_point::now();
                auto size = actions_trace_process_queue.size();
                
                while(!actions_trace_process_queue.empty()){
                    auto& tra = actions_trace_process_queue.front();
                    process_action_trace(tra);
                    actions_trace_process_queue.pop_front();
                }
                auto time = fc::time_point::now() - start_time;
                auto per = size > 0 ? time.count()/size : 0;
                if( time > fc::microseconds(500000) ) // reduce logging, .5 secs
                    ilog( "process_applied_transaction,  time per: ${p}, size: ${s}, time: ${t}", ("s", size)("t", time)("p", per) );
               
                if( actions_trace_size == 0 && done)
                    break;
            }
        }FC_LOG_AND_RETHROW()
    }
    void mongo_db_plugin_impl::init() {
        using namespace bsoncxx::types;
        using bsoncxx::builder::basic::make_document;
        using bsoncxx::builder::basic::kvp;
        // Create the native contract accounts manually; sadly, we can't run their contracts to make them create themselves
        // See native_contract_chain_initializer::prepare_database()

        ilog("init mongo");
        try {
            auto client = mongo_pool->acquire();
            auto& mongo_conn = *client;

            // action_traces collection and create indexes
            auto action_traces = mongo_conn[db_name][action_traces_col];

            action_traces.create_index( bsoncxx::from_json( R"xxx({ "txid" : 1 })xxx" ));
            action_traces.create_index( bsoncxx::from_json( R"xxx({ "receiver" : 1 })xxx" ));
            action_traces.create_index( bsoncxx::from_json( R"xxx({ "sender" : 1 })xxx" ));


        } FC_LOG_AND_RETHROW()

        ilog("starting db plugin thread");

        consume_thread = boost::thread([this] { consume_blocks(); });

        //startup = false;
    }
}

mongo_db_plugin::mongo_db_plugin():my(new detail::mongo_db_plugin_impl(*this)){
}
mongo_db_plugin::~mongo_db_plugin(){}

std::vector<account_action_history_object> mongo_db_plugin::get_action_history_mongodb(app::get_action_history_params params)
{
    return detail::mongo_db_plugin_impl::get_action_history_mongodb(params);
}
void mongo_db_plugin::plugin_initialize(const boost::program_options::variables_map& options)
{
    try{
        if( options.count("mongodb-uri") && options.count("track-action-account")){

            LOAD_VALUE_SET(options, "track-action-account", my->_tracked_accounts, graphene::chain::account_id_type);
            std::string uri_str = options.at( "mongodb-uri" ).as<std::string>();
            ilog( "connecting to ${u}", ("u", uri_str));
            
            mongocxx::uri uri = mongocxx::uri{uri_str};
            my->db_name = uri.database();

            if( my->db_name.empty())
                my->db_name = "gxchain";
            detail::mongo_db_plugin_impl::mongo_pool = std::make_shared<mongocxx::pool>(uri);

            database().add_index< primary_index< action_history_index > >();
            database().applied_block.connect( [&]( const signed_block& tra){ my->update_action_histories(tra); } );
            database().applied_irr_num.connect( [&]( uint32_t num){ my->remove_action_histories(num); } );

            if( options.count( "replay-blockchain" ) || options.count( "resync-blockchain" ) ) {
                ilog( "Wiping mongo database on startup" );
                my->wipe_database();
            }
            
            my->init();   
        }  
        else{
            ilog( "mongo_db_plugin disabled." );
        }
    } FC_LOG_AND_RETHROW()
}
void mongo_db_plugin::plugin_startup(){

}
void mongo_db_plugin::plugin_set_program_options(boost::program_options::options_description& cli,
                                boost::program_options::options_description& cfg){
    cli.add_options()
         ("mongodb-uri",   boost::program_options::value<std::string>(),"MongoDB URI connection string")
         ("track-action-account", boost::program_options::value<std::vector<std::string>>()->composing()->multitoken(), "Account ID to track history for (may specify multiple times)")
         ;
    cfg.add(cli);                              

}
}}