
#include <graphene/mongo_db/mongo_db_plugin.hpp>
#include <graphene/chain/database.hpp>

#include <fc/log/console_appender.hpp>
#include <fc/log/file_appender.hpp>
#include <fc/log/logger.hpp>
#include <fc/log/logger_config.hpp>

#include <bsoncxx/builder/basic/kvp.hpp>


namespace graphene { namespace mongo_db {

using namespace chain;
namespace detail {
    class mongo_db_plugin_impl {
    public:

        mongo_db_plugin_impl(mongo_db_plugin& _plugin)
        : _self( _plugin )
                { }
        virtual ~mongo_db_plugin_impl(){}

        void update_action_histories( const signed_transaction& t ){
            ilog("transaction-fffffffffffff: ${id:}",("id" , t.id()));
        }

        fc::optional<boost::signals2::scoped_connection> accepted_block_connection;
        mongo_db_plugin& _self;
    };
}

mongo_db_plugin::mongo_db_plugin():my( new detail::mongo_db_plugin_impl(*this)){}
mongo_db_plugin::~mongo_db_plugin(){}
void mongo_db_plugin::plugin_initialize(const boost::program_options::variables_map& options)
{
    try{
        ilog("mongodb test");
        database().applied_transaction.connect( [&]( const signed_transaction& t){ my->update_action_histories(t); } );

    } FC_LOG_AND_RETHROW()
}
void mongo_db_plugin::plugin_startup(){

}
void mongo_db_plugin::plugin_set_program_options(boost::program_options::options_description&,
                                boost::program_options::options_description& cfg){

}
}}