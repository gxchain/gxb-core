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
#include <graphene/chain/trust_node_pledge_helper.hpp>
#include <graphene/chain/witness_evaluator.hpp>
#include <graphene/chain/witness_object.hpp>
#include <graphene/chain/committee_member_object.hpp>
#include <graphene/chain/account_object.hpp>
#include <graphene/chain/database.hpp>
#include <graphene/chain/protocol/vote.hpp>
#include <graphene/chain/vesting_balance_object.hpp>

namespace graphene { namespace chain {

void_result witness_create_evaluator::do_evaluate( const witness_create_operation& op )
{ try {
   database& _db = db();
   FC_ASSERT(_db.get(op.witness_account).is_lifetime_member());
   if(_db.head_block_time() > HARDFORK_1129_TIME) {
	   trust_node_pledge_helper::do_evaluate(_db, op);
   }
   return void_result();
} FC_CAPTURE_AND_RETHROW( (op) ) }

object_id_type witness_create_evaluator::do_apply(const witness_create_operation& op, int32_t billed_cpu_time_us)
{ try {
   database& _db = db();
   vote_id_type vote_id;

   auto &global_prop = _db.get_global_properties();
   _db.modify( global_prop, [&vote_id](global_property_object& p) {
      vote_id = get_next_vote_id(p, vote_id_type::witness);
   });

   const auto& new_witness_object = _db.create<witness_object>([&op,&vote_id](witness_object& obj) {
         obj.witness_account  = op.witness_account;
         obj.signing_key      = op.block_signing_key;
         obj.vote_id          = vote_id;
         obj.url              = op.url;
   });

   if(_db.head_block_time() > HARDFORK_1129_TIME) {
	   trust_node_pledge_helper::do_apply(_db, op);
   }

   return new_witness_object.id;
} FC_CAPTURE_AND_RETHROW( (op) ) }

void_result witness_update_evaluator::do_evaluate( const witness_update_operation& op )
{ try {
	database& _db = db();
	FC_ASSERT(_db.get(op.witness).witness_account == op.witness_account);

   if(_db.head_block_time() > HARDFORK_1129_TIME) {
	   trust_node_pledge_helper::do_evaluate(_db, op);
   }
   return void_result();
} FC_CAPTURE_AND_RETHROW( (op) ) }

void_result witness_update_evaluator::do_apply(const witness_update_operation& op, int32_t billed_cpu_time_us)
{ try {
   database& _db = db();
   _db.modify(
      _db.get(op.witness),
      [&op]( witness_object& wit )
      {
         if( op.new_url.valid() )
            wit.url = *op.new_url;
         if( op.new_signing_key.valid() )
            wit.signing_key = *op.new_signing_key;
      }
   );

   if(_db.head_block_time() > HARDFORK_1129_TIME) {
	   trust_node_pledge_helper::do_apply(_db, op);
   }

   return void_result();
} FC_CAPTURE_AND_RETHROW( (op) ) }

void_result trust_node_pledge_withdraw_evaluator::do_evaluate(const trust_node_pledge_withdraw_operation& op)
{ try {
   trust_node_pledge_helper::do_evaluate(db(), op);
   return void_result();
} FC_CAPTURE_AND_RETHROW( (op) ) }

void_result trust_node_pledge_withdraw_evaluator::do_apply(const trust_node_pledge_withdraw_operation& op, int32_t billed_cpu_time_us)
{ try {
   trust_node_pledge_helper::do_apply(db(), op);
   return void_result();
} FC_CAPTURE_AND_RETHROW( (op) ) }

} } // graphene::chain
