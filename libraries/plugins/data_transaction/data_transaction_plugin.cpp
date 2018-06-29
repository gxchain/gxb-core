/*
    Copyright (C) 2017 gxb

    This file is part of gxb-core.

    gxb-core is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    gxb-core is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with gxb-core.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <graphene/data_transaction/data_transaction_plugin.hpp>
#include <graphene/chain/database.hpp>
#include <graphene/chain/data_transaction_object.hpp>
#include <graphene/chain/data_transaction_evaluator.hpp>

using namespace graphene::data_transaction;
using std::string;

namespace bpo = boost::program_options;

static const char* OPT_DATA_TRANSACTION_LIFETIME  = "data-transaction-lifetime";

void data_transaction_plugin::plugin_set_program_options(
   boost::program_options::options_description& command_line_options,
   boost::program_options::options_description& config_file_options)
{
   command_line_options.add_options()
         (OPT_DATA_TRANSACTION_LIFETIME, bpo::value<uint32_t>(), "num of data_transaction kept in memory")
         ;
   config_file_options.add(command_line_options);
}

std::string data_transaction_plugin::plugin_name()const
{
   return "data_transaction";
}

void data_transaction_plugin::plugin_initialize(const boost::program_options::variables_map& options)
{ try {
    ilog("data_transaction plugin: plugin_initialize() begin");

    if (options.count(OPT_DATA_TRANSACTION_LIFETIME)) {
        data_transaction_lifetime = options[OPT_DATA_TRANSACTION_LIFETIME].as<uint32_t>();
    } else {
        // 1 hour by default
        data_transaction_lifetime = 1;
    }
    database().applied_block.connect([&](const graphene::chain::signed_block &b) { check_data_transaction(b); });
    ilog("data_transaction plugin: plugin_initialize() end");
} FC_LOG_AND_RETHROW() }

void data_transaction_plugin::plugin_startup()
{
    ilog("data transaction plugin startup");
}

void data_transaction_plugin::plugin_shutdown()
{
    ilog("data transaction plugin shutdown");
}

void data_transaction_plugin::check_data_transaction( const graphene::chain::signed_block& b )
{ try {
    if (data_transaction_lifetime > 0) {
        // look for expired data_transaction objects, and remove them.
        graphene::chain::database& db = database();
        const auto& dt_idx = db.get_index_type<data_transaction_index>().indices().get<by_create_date_time>();

        while ((!dt_idx.empty()) && (db.head_block_time() > dt_idx.begin()->create_date_time + fc::hours(data_transaction_lifetime)))
            db.remove(*dt_idx.begin());
    }
} FC_LOG_AND_RETHROW() }
