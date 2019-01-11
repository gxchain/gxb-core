
#include <graphene/mongo_db/mongo_db_plugin.hpp>
#include <graphene/chain/database.hpp>

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
        mongo_db_plugin_impl(){}
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

        void update_action_histories( const transaction_trace& tra );
        void init();
        void consume_blocks();                      // consume data(actions、inline_actions)
        void process_action_trace(const transaction_trace& tra);

        template<typename T> fc::variant to_variant_with_abi( const T& obj );
        template<typename Queue, typename Entry> void queue(Queue& queue, const Entry& e);
        // link to mongo_server
        std::string db_name;
        mongocxx::instance instance;
        std::shared_ptr<mongocxx::pool> mongo_pool;
        //mongocxx::pool mongo_pool;

        // collections consum thread
        mongocxx::collection _contract_accounts;    // contract_account info (id、name、abi....)
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
        std::deque<transaction_trace> actions_trace_queue;            // production
        std::deque<transaction_trace> actions_trace_process_queue;    // consumption

        static const std::string contract_accounts_col;
        static const std::string action_traces_col;

    };
    const std::string mongo_db_plugin_impl::contract_accounts_col = "contract_accounts";
    const std::string mongo_db_plugin_impl::action_traces_col = "action_traces";
    
    template<typename T>
    fc::variant mongo_db_plugin_impl::to_variant_with_abi( const T& obj ) {
        fc::variant pretty_output;
        /*
        abi_serializer::to_variant( obj, pretty_output,
                                    [&]( account_name n ) { return get_abi_serializer( n ); },
                                    abi_serializer_max_time );*/
        return pretty_output;
    }
    template<typename Queue, typename Entry>
    void mongo_db_plugin_impl::queue( Queue& queue, const Entry& e ) {
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
    void mongo_db_plugin_impl::update_action_histories( const transaction_trace& tra ){
        ilog("chu fa plugin");
        try{
            queue( actions_trace_queue, tra );
        }FC_LOG_AND_RETHROW()
    }

    void mongo_db_plugin_impl::process_action_trace(const transaction_trace& tra){
        using namespace bsoncxx::types;
        using bsoncxx::builder::basic::kvp;
        auto trans_test_doc = bsoncxx::builder::basic::document{};
        
        auto json_str = fc::json::to_string(tra);
        const auto& value = bsoncxx::from_json(json_str);
        trans_test_doc.append( kvp("action",value));
        
        _action_traces.insert_one( trans_test_doc.view() ) ;
    }
    void mongo_db_plugin_impl::consume_blocks(){
        try{
            auto mongo_client = mongo_pool->acquire();
            auto& mongo_conn = *mongo_client;

            _contract_accounts = mongo_conn[db_name][contract_accounts_col];
            _action_traces = mongo_conn[db_name][action_traces_col];

            while(true){
                boost::mutex::scoped_lock lock(mtx);
                while (actions_trace_queue.empty() && !done){
                    condition.wait(lock);
                }

                // capture for processing
                size_t actions_trace_size = actions_trace_queue.size();
                if ( actions_trace_size > 0) {
                    actions_trace_process_queue = std::move(actions_trace_queue);
                    actions_trace_queue.clear();
                }
                lock.unlock();
                
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
            ilog("77777");
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

            // contract_accounts collection
            auto contract_accounts = mongo_conn[db_name][contract_accounts_col];

            contract_accounts.create_index( bsoncxx::from_json( R"xxx({ "account_id" : 1 })xxx" ));

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

mongo_db_plugin::mongo_db_plugin():my( new detail::mongo_db_plugin_impl){}
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
        // 2 绑定信号量
        database().applied_tra.connect( [&]( const transaction_trace& tra){ my->update_action_histories(tra); } );
        
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