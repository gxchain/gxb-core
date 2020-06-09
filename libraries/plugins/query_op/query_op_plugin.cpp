/*
 * Copyright (c) 2015 Cryptonomex, Inc., and contributors.
 *
 * The MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#pragma once
#include <graphene/query_op/query_op_plugin.hpp>
#include <fc/io/fstream.hpp>
#include <graphene/chain/operation_entry_object.hpp>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <leveldb/db.h>
#include <leveldb/write_batch.h>
#include <fc/signals.hpp>
#include <graphene/chain/impacted.hpp>
#include <graphene/chain/account_evaluator.hpp>
#include <graphene/chain/account_object.hpp>
#include <graphene/chain/config.hpp>
#include <graphene/chain/database.hpp>
#include <graphene/chain/evaluator.hpp>
#include <graphene/chain/operation_history_object.hpp>
#include <graphene/chain/transaction_evaluation_state.hpp>
#include <fc/smart_ref_impl.hpp>
#include <fc/thread/thread.hpp>

namespace graphene{ namespace query_op{

namespace detail
{
using namespace leveldb;
class query_op_plugin_impl
{
  public:
    query_op_plugin_impl(query_op_plugin &_plugin)
        : _self(_plugin)
    {
    }
    ~query_op_plugin_impl()
    {
    }

    void update_account_operations(const signed_block &b);
    graphene::chain::database &database()
    {
        return _self.database();
    }
    void init();
    static optional<op_entry_object> query_op_by_index(std::string op_index);
    std::string db_path = "op_entry.db";
    uint64_t limit_batch = 1000; //limit of leveldb batch
    primary_index< operation_history_index >* _oho_index;
 	 query_op_plugin &_self;

  private:
    fc::signal<void(const uint64_t)> sig_remove;
	 void add_account_operation(const account_id_type account_id, const optional<operation_history_object>& op);
    static leveldb::DB *leveldb;                              
    void remove_op_index(const uint64_t op_entry_id); //Remove op_index in db
};
leveldb::DB *query_op_plugin_impl::leveldb = nullptr;

void query_op_plugin_impl::init()
{
    try {
        //Create leveldb
        leveldb::Options options;
        options.create_if_missing = true;
        leveldb::Status s = leveldb::DB::Open(options, db_path, &leveldb);

        sig_remove.connect([&](const uint64_t op_entry_id) { remove_op_index(op_entry_id); });
    }
    FC_LOG_AND_RETHROW()
}
optional<op_entry_object> query_op_plugin_impl::query_op_by_index(std::string op_index)
{
    try {
        if (leveldb == nullptr) return optional<op_entry_object>();
        std::string value;
        leveldb::Status s = leveldb->Get(leveldb::ReadOptions(), op_index, &value);
        if (!s.ok()) return optional<op_entry_object>();
        std::vector<char> data(value.begin(), value.end());
        auto result = fc::raw::unpack<op_entry_object>(data);
        return result;
    }
    FC_LOG_AND_RETHROW()
}

void query_op_plugin_impl::update_account_operations(const signed_block &b)
{
	graphene::chain::database& db = database();
	const vector<optional< operation_history_object > >& hist = db.get_applied_operations();
	bool is_first = true;
	auto skip_oho_id = [&is_first,&db,this]() {
		if( is_first && db._undo_db.enabled() ) 
		{
			db.remove( db.create<operation_history_object>( []( operation_history_object& obj) {} ) );
			is_first = false;
		}
		else
			_oho_index->use_next_id();
	};

   for( const optional< operation_history_object >& o_op : hist )
   {
      optional<operation_history_object> oho;

      auto create_oho = [&]() {
         is_first = false;
         return optional<operation_history_object>( db.create<operation_history_object>( [&]( operation_history_object& h )
         {
            if( o_op.valid() )
            {
               h.op           = o_op->op;
               h.result       = o_op->result;
               h.block_num    = o_op->block_num;
               h.trx_in_block = o_op->trx_in_block;
               h.op_in_trx    = o_op->op_in_trx;
               h.virtual_op   = o_op->virtual_op;
            }
         } ) );
      };
      
      if( !o_op.valid())
      {
         skip_oho_id();
         continue;
      }
      else{
         // add to the operation history index
         oho = create_oho();
	  }

		const operation_history_object& op = *o_op;

		flat_set<account_id_type> impacted;
        vector<authority> other;

		operation_get_required_authorities( op.op, impacted, impacted, other ); 

		if( op.op.which() == operation::tag< account_create_operation >::value )
         impacted.insert( op.result.get<object_id_type>() );
      else
         graphene::chain::operation_get_impacted_accounts( op.op, impacted );

		for( auto& a : other )
         for( auto& item : a.account_auths )
            impacted.insert( item.first );

		if (!impacted.empty() && !oho.valid()) { oho = create_oho(); }

      for( auto& account_id : impacted )
         {
            add_account_operation( account_id, oho );
         }
		if (! oho.valid())
         skip_oho_id();
      operation_history_id_type remove_op_id = oho->id;
	  db.remove( remove_op_id(db));
         
   }
}

void query_op_plugin_impl::add_account_operation(const account_id_type account_id, const optional<operation_history_object>& op)
{
	graphene::chain::database& db = database();
   const auto& stats_obj = account_id(db).statistics(db);

	const auto& ath = db.create<op_entry_object>([&](op_entry_object& obj){
		obj.block_num 			= op->block_num;
		obj.trx_in_block 		= op->trx_in_block;
		obj.op_in_trx  		    = op->op_in_trx;
		obj.sequence 			= stats_obj.total_ops + 1;
		obj.op_index 			= std::to_string(account_id.space_id)+"."+std::to_string(account_id.type_id)+"."+std::to_string(account_id.instance.value)+"_"
														  +std::to_string(obj.sequence);
		if(op->op.which() == operation::tag< inline_transfer_operation >::value || op->op.which() == operation::tag< inter_contract_call_operation >::value)
		{
			obj.is_virtual = true;
			obj.virtual_op = op->op;
		}
	});

	db.modify( stats_obj, [&]( account_statistics_object& obj ){
       obj.total_ops = ath.sequence;
   });

	const auto &dpo = db.get_dynamic_global_properties();
	uint64_t irr_num = dpo.last_irreversible_block_num;
	const auto &op_idx = db.get_index_type<op_entry_index>().indices();
	const auto &op_bn_idx = op_idx.get<by_blocknum>();
	if (op_idx.begin() == op_idx.end()) return;
	auto itor_begin = op_bn_idx.begin();
   auto itor_end = op_bn_idx.lower_bound(irr_num);
   auto number = std::distance(itor_begin,itor_end);
   auto backupnum = number;
   auto put_index = itor_begin->id.instance();
	while (number > limit_batch) {
		leveldb::WriteBatch batch;
		auto itor_backup = itor_begin;
		for (auto idx = 0; idx < limit_batch; idx++) {
				auto serialize = fc::raw::pack(*itor_begin);
				std::string op_index(itor_begin->op_index);
				batch.Put(op_index, {serialize.data(), serialize.size()});
				put_index = itor_begin->id.instance();
				itor_begin++;
				if (itor_begin == itor_end) break;
		}
		leveldb::WriteOptions write_options;
		write_options.sync = true;
		Status s = leveldb->Write(write_options, &batch);
		if (!s.ok()) {
				itor_begin = itor_backup;
				put_index = itor_begin->id.instance();
				break;
		}
		number -= limit_batch;
		}
		if (backupnum > limit_batch)
		sig_remove(put_index);
}


void query_op_plugin_impl::remove_op_index(const uint64_t op_entry_id)
{
    try {
        graphene::chain::database &db = database();
        const auto &op_idx = db.get_index_type<op_entry_index>().indices();
        ilog("remove,${op_ent_id},bengin: ${begin},end: ${end}",("op_ent_id",op_entry_id)("begin",op_idx.begin()->id.instance())("end",op_idx.rbegin()->id.instance()));
        for (auto itor = op_idx.begin(); itor != op_idx.end();) {
            auto backup_itr = itor;
            ++itor;
            if (itor->id.instance() < op_entry_id) {
                db.remove(*backup_itr);
            } else {
                break;
            }
        }
    }
    FC_LOG_AND_RETHROW()
}
} // namespace detail

// -----------------------------------query_op_plugin --------------------------------------

query_op_plugin::query_op_plugin()
    : my(new detail::query_op_plugin_impl(*this))
{
}

query_op_plugin::~query_op_plugin()
{
}

std::string query_op_plugin::plugin_name() const
{
    return "query_op";
}

void query_op_plugin::plugin_set_program_options(
    boost::program_options::options_description &cli,
    boost::program_options::options_description &cfg)
{
    cli.add_options()("query-op-path", boost::program_options::value<std::string>(), "Save the leveldb path of the accounts'operation history")("limit-batch", boost::program_options::value<uint64_t>(), "Number of records written to leveldb in batches");
    cfg.add(cli);
}

void query_op_plugin::plugin_initialize(const boost::program_options::variables_map &options)
{
    try {
        ilog("query_op plugin initialized");
        // Add the index of the op_entry_index object table to the database
        // Respond to the apply_block signal
        database().applied_block.connect([&](const signed_block &b) { my->update_account_operations(b); });
        my->_oho_index = database().add_index< primary_index< operation_history_index > >();
        database().add_index<primary_index<op_entry_index>>();
        if (options.count("query-op-path")) {
            my->db_path = options["query-op-path"].as<std::string>();
            if (!fc::exists(my->db_path))
                fc::create_directories(my->db_path);
        }
        if (options.count("limit-batch")) {
            my->limit_batch = options["limit-batch"].as<uint64_t>();
        }
        // Initialize the plugin instance
        my->init();
    }
    FC_LOG_AND_RETHROW()
}

void query_op_plugin::plugin_startup()
{
}

optional<op_entry_object> query_op_plugin::query_op_by_index(std::string op_index)
{
    return detail::query_op_plugin_impl::query_op_by_index(op_index);
}

} // namespace query_op
} // namespace graphene