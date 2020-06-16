#pragma once
#include <graphene/app/plugin.hpp>
#include <graphene/chain/database.hpp>

namespace graphene
{
namespace account_history_leveldb
{

using namespace chain;
namespace detail
{
class account_history_leveldb_plugin_impl;
}
class account_history_leveldb_plugin : public graphene::app::plugin
{
  public:
    account_history_leveldb_plugin();
    virtual ~account_history_leveldb_plugin();

    std::string plugin_name() const override;

    virtual void plugin_set_program_options(
        boost::program_options::options_description &cli,
        boost::program_options::options_description &cfg) override;

    virtual void plugin_initialize(const boost::program_options::variables_map &options) override;
    virtual void plugin_startup() override;

    static optional<op_entry_object> query_op_by_index(std::string op_index);

    friend class detail::account_history_leveldb_plugin_impl;

    std::unique_ptr<detail::account_history_leveldb_plugin_impl> my;
};
} // namespace account_history_leveldb
} // namespace graphene
