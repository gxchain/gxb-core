
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
    class mongo_db_plugin_impl {
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
        void consume_blocks();                      // consume data(actions、inline_actions)
        void process_action_trace(const transaction_id_trace& tra);
        std::vector<account_action_history_object> get_action_history_mongodb();

        fc::optional<abi_serializer> get_abi_serializer( uint64_t accid );
        template<typename T> fc::variant to_variant_with_abi( const T& obj );
        template<typename Queue, typename Entry> void queue(Queue& queue, const Entry e);
        // link to mongo_server
        std::string db_name;
        mongocxx::instance instance;
        std::shared_ptr<mongocxx::pool> mongo_pool;
        //mongocxx::pool mongo_pool;

        // collections consum thread
        mongocxx::collection _action_traces;        // actions(account、action、inline_action)

        // consume thread 
        boost::thread consume_thread;
        boost::mutex mtx;
        boost::condition_variable condition;

        // production queue
        size_t max_queue_size = 0;
        int queue_sleep_time = 0;
        std::atomic_bool done{false};

        // deque(cache by production and consumption)
        std::deque<transaction_id_trace> actions_trace_queue;            // production
        std::deque<transaction_id_trace> actions_trace_process_queue;    // consumption

        static const std::string action_traces_col;

        // access db
        mongo_db_plugin& _self;

    };
    const std::string mongo_db_plugin_impl::action_traces_col = "action_traces";
    std::vector<account_action_history_object> mongo_db_plugin_impl::get_action_history_mongodb()
    {
        std::vector<account_action_history_object> result;
        //mongocxx::cursor cursor = _action_traces.find().sort({"_id":-1}).limit(5);
        //result.push_back
        return result;
    }

    fc::optional<abi_serializer> mongo_db_plugin_impl::get_abi_serializer( uint64_t accid ) {
        using bsoncxx::builder::basic::kvp;
        using bsoncxx::builder::basic::make_document;
        try {
            fc::microseconds abi_serializer_max_time = fc::microseconds(1000*1500);
            auto& db =_self.database();
            auto accidx = db.get_index_type<account_index>().indices();
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
                    transaction_id_trace trx_id_trace_obj;
                    transaction_trace &tractobj = trx_id_trace_obj.trx_trace;
                    tractobj.block_num = itor->block_num;
                    tractobj.traces.receiver = itor->receiver;
                    tractobj.traces.sender = itor->sender;
                    tractobj.traces.act = itor->act;
                    tractobj.result = itor->result;
                    trx_id_trace_obj._id=std::to_string(itor->block_num)+"_"+std::to_string(itor->trx_in_block)+"_"+std::to_string(itor->op_in_trx);
                    //tractobj.traces.inline_traces = itor->inline_actions;
                    queue( actions_trace_queue, trx_id_trace_obj);
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
                    db.create<account_action_history_object>( [&]( account_action_history_object& h )
                    {
                        h.block_num    = o_act->block_num;
                        h.trx_in_block = o_act->trx_in_block;
                        h.op_in_trx    = o_act->op_in_trx;
                        h.sender       = o_act->sender;
                        h.receiver     = o_act->receiver;
                        h.act          = o_act->act;
                        h.inline_actions = o_act->inline_actions;
                        h.result = o_act->result;
                    });
                }
            }
        }FC_LOG_AND_RETHROW()
    }

    void mongo_db_plugin_impl::process_action_trace(const transaction_id_trace& tra){
        try{
            using namespace bsoncxx::types;
            using bsoncxx::builder::basic::kvp;
            using bsoncxx::builder::basic::make_document;
            auto actions_trans_doc = bsoncxx::builder::basic::document{};
            
            //const base_action_trace& bact = tra.traces;
            auto abi_json_str = to_variant_with_abi(tra.trx_trace);
            auto json_str = fc::json::to_string(abi_json_str);
            const auto& value = bsoncxx::from_json(json_str);

            actions_trans_doc.append( kvp("_id",tra._id));
            actions_trans_doc.append( kvp("action",value));
            for( auto i=0;i<2;i++){
                auto doc = _action_traces.find_one( make_document( kvp("_id", tra._id)) );
                if(!doc)
                    _action_traces.insert_one(actions_trans_doc.view() ) ;
            }                
        }FC_LOG_AND_RETHROW()
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

            action_traces.create_index( bsoncxx::from_json( R"xxx({ "trx_id" : 1 })xxx" ));
            action_traces.create_index( bsoncxx::from_json( R"xxx({ "reciver" : 1 })xxx" ));
            action_traces.create_index( bsoncxx::from_json( R"xxx({ "sender" : 1 })xxx" ));


        } FC_LOG_AND_RETHROW()

        ilog("starting db plugin thread");

        consume_thread = boost::thread([this] { consume_blocks(); });

        //startup = false;
    }
}

mongo_db_plugin::mongo_db_plugin():my( new detail::mongo_db_plugin_impl(*this)){}
mongo_db_plugin::~mongo_db_plugin(){}
void mongo_db_plugin::plugin_initialize(const boost::program_options::variables_map& options)
{
    try{
        ilog("mongodb test");
        // 1 连接到mongodb
        std::string uri_str = "mongodb://localhost:27017/gxchain";
        mongocxx::uri uri = mongocxx::uri{uri_str};
        my->db_name = uri.database();
        if( my->db_name.empty())
            my->db_name = "gxchain";
        my->mongo_pool = std::make_shared<mongocxx::pool>(uri);

        database().add_index< primary_index< action_history_index > >();
        // 2 绑定信号量
        //database().applied_tra.connect( [&]( const transaction_trace& tra){ my->update_action_histories(tra); } );
        database().applied_block.connect( [&]( const signed_block& tra){ my->update_action_histories(tra); } );
        
        database().applied_irr_num.connect( [&]( uint32_t num){ my->remove_action_histories(num); } );
        // 3 处理接收到的信号
        my->init();     
    } FC_LOG_AND_RETHROW()
}
void mongo_db_plugin::plugin_startup(){

}
void mongo_db_plugin::plugin_set_program_options(boost::program_options::options_description&,
                                boost::program_options::options_description& cfg){

}
}}