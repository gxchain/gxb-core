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
#include <graphene/chain/asset_evaluator.hpp>
#include <graphene/chain/asset_object.hpp>
#include <graphene/chain/account_object.hpp>
#include <graphene/chain/market_object.hpp>
#include <graphene/chain/database.hpp>
#include <graphene/chain/exceptions.hpp>
#include <graphene/chain/hardfork.hpp>
#include <graphene/chain/is_authorized_asset.hpp>

#include <functional>

namespace graphene { namespace chain {

void_result asset_create_evaluator::do_evaluate( const asset_create_operation& op )
{ try {

   database& d = db();

   const auto& chain_parameters = d.get_global_properties().parameters;
   FC_ASSERT( op.common_options.whitelist_authorities.size() <= chain_parameters.maximum_asset_whitelist_authorities );
   FC_ASSERT( op.common_options.blacklist_authorities.size() <= chain_parameters.maximum_asset_whitelist_authorities );

   // Check that all authorities do exist
   for( auto id : op.common_options.whitelist_authorities )
      d.get_object(id);
   for( auto id : op.common_options.blacklist_authorities )
      d.get_object(id);

   if (d.head_block_time() > HARDFORK_1008_TIME) {
       FC_ASSERT(GRAPHENE_SYMBOL_GXS != op.symbol, "new asset symbol can not be GXS");
   }

   auto& asset_indx = d.get_index_type<asset_index>().indices().get<by_symbol>();
   auto asset_symbol_itr = asset_indx.find( op.symbol );
   FC_ASSERT( asset_symbol_itr == asset_indx.end() );

   auto dotpos = op.symbol.rfind( '.' );
   if (dotpos != std::string::npos) {
       auto prefix = op.symbol.substr( 0, dotpos );
       auto asset_symbol_itr = asset_indx.find( prefix );
       FC_ASSERT( asset_symbol_itr != asset_indx.end(), "Asset ${s} may only be created by issuer of ${p}, but ${p} has not been registered",
               ("s",op.symbol)("p",prefix) );
       FC_ASSERT( asset_symbol_itr->issuer == op.issuer, "Asset ${s} may only be created by issuer of ${p}, ${i}",
               ("s",op.symbol)("p",prefix)("i", op.issuer(d).name) );
   }

   // check core_exchange_rate
   if (d.head_block_time() > HARDFORK_1008_TIME) {
       FC_ASSERT(op.common_options.core_exchange_rate.base.asset_id == asset_id_type(1) ||
                 op.common_options.core_exchange_rate.quote.asset_id == asset_id_type(1));
   } else {
       FC_ASSERT(op.common_options.core_exchange_rate.base.asset_id == asset_id_type() ||
                 op.common_options.core_exchange_rate.quote.asset_id == asset_id_type());
   }

   if (d.head_block_time() > HARDFORK_1008_TIME) {
       asset dummy = asset(1, asset_id_type(1)) * op.common_options.core_exchange_rate;
       FC_ASSERT(dummy.asset_id == asset_id_type(2));

       if (op.bitasset_opts) {
           // disable bitasset creation
           FC_ASSERT(false, "disabled since hardfork 1008");
       }
   } else {
       asset dummy = asset(1) * op.common_options.core_exchange_rate;
       FC_ASSERT(dummy.asset_id == asset_id_type(1));
   }

   return void_result();
} FC_CAPTURE_AND_RETHROW( (op) ) }

void asset_create_evaluator::pay_fee()
{
   fee_is_odd = core_fee_paid.value & 1;
   core_fee_paid -= core_fee_paid.value/2;
   generic_evaluator::pay_fee();
}

object_id_type asset_create_evaluator::do_apply(const asset_create_operation& op, uint32_t billed_cpu_time_us)
{ try {
   database& d = db();

   bool hf_1005 = fee_is_odd && d.head_block_time() > HARDFORK_1005_TIME;

   const asset_dynamic_data_object& dyn_asset =
      d.create<asset_dynamic_data_object>( [&]( asset_dynamic_data_object& a ) {
         a.current_supply = 0;
         a.fee_pool = core_fee_paid - (hf_1005 ? 1 : 0);
      });

   if (fee_is_odd && !hf_1005) {
       const auto &core_dd = d.get<asset_object>(asset_id_type()).dynamic_data(d);
       d.modify(core_dd, [=](asset_dynamic_data_object &dd) {
           dd.current_supply++;
       });
   }

   // for core_exchange_rate
   asset_id_type core_asset_id = asset_id_type();
   if (d.head_block_time() > HARDFORK_1008_TIME) {
       core_asset_id = asset_id_type(1);
   }

   auto next_asset_id = d.get_index_type<asset_index>().get_next_id();
   const asset_object& new_asset =
     d.create<asset_object>( [&]( asset_object& a ) {
         a.issuer = op.issuer;
         a.symbol = op.symbol;
         a.precision = op.precision;
         a.options = op.common_options;
         if( a.options.core_exchange_rate.base.asset_id == core_asset_id )
            a.options.core_exchange_rate.quote.asset_id = next_asset_id;
         else
            a.options.core_exchange_rate.base.asset_id = next_asset_id;
         a.dynamic_asset_data_id = dyn_asset.id;
      });
   FC_ASSERT(new_asset.id == next_asset_id, "Unexpected object database error, object id mismatch");

   return new_asset.id;
} FC_CAPTURE_AND_RETHROW( (op) ) }

void_result asset_issue_evaluator::do_evaluate( const asset_issue_operation& o )
{ try {
   const database& d = db();

   const asset_object& a = o.asset_to_issue.asset_id(d);
   FC_ASSERT( o.issuer == a.issuer );
   FC_ASSERT( !a.is_market_issued(), "Cannot manually issue a market-issued asset." );

   to_account = &o.issue_to_account(d);
   FC_ASSERT( is_authorized_asset( d, *to_account, a ) );

   asset_dyn_data = &a.dynamic_asset_data_id(d);
   FC_ASSERT( (asset_dyn_data->current_supply + o.asset_to_issue.amount) <= a.options.max_supply );

   return void_result();
} FC_CAPTURE_AND_RETHROW( (o) ) }

void_result asset_issue_evaluator::do_apply(const asset_issue_operation& o, uint32_t billed_cpu_time_us)
{ try {
   db().adjust_balance( o.issue_to_account, o.asset_to_issue );

   db().modify( *asset_dyn_data, [&]( asset_dynamic_data_object& data ){
        data.current_supply += o.asset_to_issue.amount;
   });

   return void_result();
} FC_CAPTURE_AND_RETHROW( (o) ) }

void_result asset_reserve_evaluator::do_evaluate( const asset_reserve_operation& o )
{ try {
   const database& d = db();

   const asset_object& a = o.amount_to_reserve.asset_id(d);
   GRAPHENE_ASSERT(
      !a.is_market_issued(),
      asset_reserve_invalid_on_mia,
      "Cannot reserve ${sym} because it is a market-issued asset",
      ("sym", a.symbol)
   );

   from_account = &o.payer(d);
   FC_ASSERT( is_authorized_asset( d, *from_account, a ) );

   asset_dyn_data = &a.dynamic_asset_data_id(d);
   FC_ASSERT( (asset_dyn_data->current_supply - o.amount_to_reserve.amount) >= 0 );

   return void_result();
} FC_CAPTURE_AND_RETHROW( (o) ) }

void_result asset_reserve_evaluator::do_apply(const asset_reserve_operation& o, uint32_t billed_cpu_time_us)
{ try {
   db().adjust_balance( o.payer, -o.amount_to_reserve );

   db().modify( *asset_dyn_data, [&]( asset_dynamic_data_object& data ){
        data.current_supply -= o.amount_to_reserve.amount;
   });

   return void_result();
} FC_CAPTURE_AND_RETHROW( (o) ) }

void_result asset_fund_fee_pool_evaluator::do_evaluate(const asset_fund_fee_pool_operation& o)
{ try {
   database& d = db();

   // check fee asset type
   if (d.head_block_time() > HARDFORK_1008_TIME) {
       FC_ASSERT(o.fee.asset_id == asset_id_type(1));
   } else {
       FC_ASSERT(o.fee.asset_id == asset_id_type());
   }

   const asset_object& a = o.asset_id(d);
   asset_dyn_data = &a.dynamic_asset_data_id(d);

   return void_result();
} FC_CAPTURE_AND_RETHROW( (o) ) }

void_result asset_fund_fee_pool_evaluator::do_apply(const asset_fund_fee_pool_operation& o, uint32_t billed_cpu_time_us)
{ try {
   database& d = db();

   if (d.head_block_time() > HARDFORK_1008_TIME) {
       d.adjust_balance(o.from_account, asset(-o.amount, asset_id_type(1)));
   } else {
       d.adjust_balance(o.from_account, -o.amount);
   }

   d.modify( *asset_dyn_data, [&]( asset_dynamic_data_object& data ) {
      data.fee_pool += o.amount;
   });

   return void_result();
} FC_CAPTURE_AND_RETHROW( (o) ) }

void_result asset_update_evaluator::do_evaluate(const asset_update_operation& o)
{ try {
   database& d = db();

   const asset_object& a = o.asset_to_update(d);
   auto a_copy = a;
   a_copy.options = o.new_options;
   a_copy.validate();

   // check core_exchange_rate
   if (d.head_block_time() > HARDFORK_1008_TIME) {
       FC_ASSERT(a_copy.options.core_exchange_rate.base.asset_id == asset_id_type(1) ||
                 a_copy.options.core_exchange_rate.quote.asset_id == asset_id_type(1));
   } else {
       FC_ASSERT(a_copy.options.core_exchange_rate.base.asset_id == asset_id_type() ||
                 a_copy.options.core_exchange_rate.quote.asset_id == asset_id_type());
   }

   if (o.new_issuer) {
       FC_ASSERT(d.find_object(*o.new_issuer));
   }

   for (const auto &ex : o.extensions) {
       if (ex.which() == future_extensions::tag<asset_symbol_t>::value) {
           FC_ASSERT(d.head_block_time() < HARDFORK_1103_TIME, "you can not change asset symbol after the timestamp:${x}", ("x", HARDFORK_1103_TIME));

           const asset_symbol_t &ast = ex.get<asset_symbol_t>();
           FC_ASSERT(is_valid_symbol(ast.symbol));
           FC_ASSERT(GRAPHENE_SYMBOL_GXS != ast.symbol, "new asset symbol can not be GXS");
           auto& asset_indx = d.get_index_type<asset_index>().indices().get<by_symbol>();
           auto itr = asset_indx.find(ast.symbol);
           FC_ASSERT(itr == asset_indx.end(), "symbol: ${x} exist", ("x", ast.symbol));
           new_symbol = ast.symbol;
           break;
       }
   }
   
   if( (d.head_block_time() < HARDFORK_572_TIME) || (a.dynamic_asset_data_id(d).current_supply != 0) )
   {
      // new issuer_permissions must be subset of old issuer permissions
      FC_ASSERT(!(o.new_options.issuer_permissions & ~a.options.issuer_permissions),
                "Cannot reinstate previously revoked issuer permissions on an asset.");
   }

   // changed flags must be subset of old issuer permissions
   FC_ASSERT(!((o.new_options.flags ^ a.options.flags) & ~a.options.issuer_permissions),
             "Flag change is forbidden by issuer permissions");

   asset_to_update = &a;
   if (a.issuer == GRAPHENE_NULL_ACCOUNT && a.get_id() == asset_id_type()) {
       FC_ASSERT(o.issuer == account_id_type(), "", ("o.issuer", o.issuer)("a.issuer", a.issuer));
   } else {
       FC_ASSERT(o.issuer == a.issuer, "", ("o.issuer", o.issuer)("a.issuer", a.issuer));
   }

   const auto& chain_parameters = d.get_global_properties().parameters;

   FC_ASSERT( o.new_options.whitelist_authorities.size() <= chain_parameters.maximum_asset_whitelist_authorities );
   for( auto id : o.new_options.whitelist_authorities )
      d.get_object(id);
   FC_ASSERT( o.new_options.blacklist_authorities.size() <= chain_parameters.maximum_asset_whitelist_authorities );
   for( auto id : o.new_options.blacklist_authorities )
      d.get_object(id);

   // check core_exchange_rate
   if (d.head_block_time() > HARDFORK_1008_TIME) {
       asset dummy = asset(1, o.asset_to_update) * o.new_options.core_exchange_rate;
       FC_ASSERT(dummy.asset_id == asset_id_type(1));
   } else {
       asset dummy = asset(1, o.asset_to_update) * o.new_options.core_exchange_rate;
       FC_ASSERT(dummy.asset_id == asset_id_type());
   }

   return void_result();
} FC_CAPTURE_AND_RETHROW((o)) }

void_result asset_update_evaluator::do_apply(const asset_update_operation& o, uint32_t billed_cpu_time_us)
{ try {
   database& d = db();

   // clear uia fee pool
   bool clear_fee_pool = false;
   if (d.head_block_time() > HARDFORK_1008_TIME) {
       if (asset_id_type() == o.asset_to_update) {
           // for 1.3.0
       } else if (asset_id_type(1) == o.asset_to_update) {
           // for 1.3.1
           clear_fee_pool = true;
       } else if (asset_to_update->options.core_exchange_rate.base.asset_id != asset_id_type(1)
               && asset_to_update->options.core_exchange_rate.quote.asset_id != asset_id_type(1)) {
           clear_fee_pool = true;
       }
   }
   if (clear_fee_pool) {
       ilog("clear ${a} asset fee pool", ("a", asset_to_update->symbol));
       d.modify(asset_to_update->dynamic_asset_data_id(d), [&](asset_dynamic_data_object &data) { data.fee_pool = 0; });
   }

   d.modify(*asset_to_update, [&](asset_object &a) {
       if (o.new_issuer) {
           a.issuer = *o.new_issuer;
       }
       a.options = o.new_options;
       if(!new_symbol.empty()) {
           a.symbol = new_symbol;
       }
   });

   return void_result();
} FC_CAPTURE_AND_RETHROW( (o) ) }

void_result asset_update_bitasset_evaluator::do_evaluate(const asset_update_bitasset_operation& o)
{ try {
    // disable
    if (db().head_block_time() > HARDFORK_1008_TIME) {
        FC_ASSERT(false, "disabled since hardfork 1008");
    }
    return void_result();
} FC_CAPTURE_AND_RETHROW( (o) ) }

void_result asset_update_bitasset_evaluator::do_apply(const asset_update_bitasset_operation& o, uint32_t billed_cpu_time_us)
{ try {
   return void_result();
} FC_CAPTURE_AND_RETHROW( (o) ) }

void_result asset_update_feed_producers_evaluator::do_evaluate(const asset_update_feed_producers_evaluator::operation_type& o)
{ try {
    // disable
    if (db().head_block_time() > HARDFORK_1008_TIME) {
        FC_ASSERT(false, "disabled since hardfork 1008");
    }
    return void_result();
} FC_CAPTURE_AND_RETHROW( (o) ) }

void_result asset_update_feed_producers_evaluator::do_apply(const asset_update_feed_producers_evaluator::operation_type& o, uint32_t billed_cpu_time_us)
{ try {
   return void_result();
} FC_CAPTURE_AND_RETHROW( (o) ) }

void_result asset_global_settle_evaluator::do_evaluate(const asset_global_settle_evaluator::operation_type& op)
{ try {
    // disable
    if (db().head_block_time() > HARDFORK_1008_TIME) {
        FC_ASSERT(false, "disabled since hardfork 1008");
    }
    return void_result();
} FC_CAPTURE_AND_RETHROW( (op) ) }

void_result asset_global_settle_evaluator::do_apply(const asset_global_settle_evaluator::operation_type& op, uint32_t billed_cpu_time_us)
{ try {
   return void_result();
} FC_CAPTURE_AND_RETHROW( (op) ) }

void_result asset_settle_evaluator::do_evaluate(const asset_settle_evaluator::operation_type& op)
{ try {
    if (db().head_block_time() > HARDFORK_1008_TIME) {
        FC_ASSERT(false, "disabled since hardfork 1008");
    }
    return void_result();
} FC_CAPTURE_AND_RETHROW( (op) ) }

operation_result asset_settle_evaluator::do_apply(const asset_settle_evaluator::operation_type& op, uint32_t billed_cpu_time_us)
{ try {
   return void_result();
} FC_CAPTURE_AND_RETHROW( (op) ) }

void_result asset_publish_feeds_evaluator::do_evaluate(const asset_publish_feed_operation& o)
{ try {
    if (db().head_block_time() > HARDFORK_1008_TIME) {
        FC_ASSERT(false, "disabled since hardfork 1008");
    }
    return void_result();
} FC_CAPTURE_AND_RETHROW((o)) }

void_result asset_publish_feeds_evaluator::do_apply(const asset_publish_feed_operation& o, uint32_t billed_cpu_time_us)
{ try {
   return void_result();
} FC_CAPTURE_AND_RETHROW((o)) }

void_result asset_claim_fees_evaluator::do_evaluate( const asset_claim_fees_operation& o )
{ try {
   FC_ASSERT( db().head_block_time() > HARDFORK_413_TIME );
   FC_ASSERT( o.amount_to_claim.asset_id(db()).issuer == o.issuer, "Asset fees may only be claimed by the issuer" );
   return void_result();
} FC_CAPTURE_AND_RETHROW( (o) ) }


void_result asset_claim_fees_evaluator::do_apply(const asset_claim_fees_operation& o, uint32_t billed_cpu_time_us)
{ try {
   database& d = db();

   const asset_object& a = o.amount_to_claim.asset_id(d);
   const asset_dynamic_data_object& addo = a.dynamic_asset_data_id(d);
   FC_ASSERT( o.amount_to_claim.amount <= addo.accumulated_fees, "Attempt to claim more fees than have accumulated", ("addo",addo) );

   d.modify( addo, [&]( asset_dynamic_data_object& _addo  ) {
     _addo.accumulated_fees -= o.amount_to_claim.amount;
   });

   d.adjust_balance( o.issuer, o.amount_to_claim );

   return void_result();
} FC_CAPTURE_AND_RETHROW( (o) ) }


} } // graphene::chain
