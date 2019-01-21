#pragma once
#include <graphene/app/plugin.hpp>
#include <graphene/chain/database.hpp>

namespace graphene { namespace mongo_db{

    namespace detail {
        class mongo_db_plugin_impl; 
    }

    class mongo_db_plugin : public graphene::app::plugin{

        public:
            mongo_db_plugin();
            virtual ~mongo_db_plugin();

            virtual void plugin_initialize(const boost::program_options::variables_map& options) override;
            virtual void plugin_startup() override;

            virtual void plugin_set_program_options(boost::program_options::options_description&,
                                                boost::program_options::options_description& cfg) override;

            std::string plugin_name()const override { return "mongo_db"; }

            static std::vector<chain::account_action_history_object> get_action_history_mongodb();
            
        private:
            friend class detail::mongo_db_plugin_impl;
            std::unique_ptr<detail::mongo_db_plugin_impl> my;
    };
}
}