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
#include <graphene/db/object.hpp>
#include <boost/multi_index/composite_key.hpp>
#include <graphene/chain/protocol/operations.hpp>

namespace graphene { namespace chain {

   class account_action_history_object : public abstract_object<account_action_history_object>
   {
      public:
         static const uint8_t space_id = implementation_ids;
         static const uint8_t type_id  = impl_account_action_history_object_type;
         
         uint64_t                mongodb_id;
         account_id_type         sender;
         account_id_type         receiver;
         action                  act;
         std::vector<action>     inline_actions;
         contract_receipt        result;
         transaction_id_type     txid;
         bool                    irreversible_state;
         
         uint32_t          block_num = 0;
         uint16_t          trx_in_block = 0;
         uint16_t          op_in_trx = 0;

   };

   struct by_id;
   struct by_mongodb_id;
   struct by_sender;
   struct by_receiver;
   struct by_blocknum;

   typedef multi_index_container<
      account_action_history_object,
      indexed_by<
         ordered_unique< tag<by_id>, member< object, object_id_type, &object::id > >,
         ordered_unique< tag<by_mongodb_id>, member< account_action_history_object,uint64_t,&account_action_history_object::mongodb_id >>,
         ordered_non_unique< tag<by_sender>,member< account_action_history_object, account_id_type, &account_action_history_object::sender>>,
         ordered_non_unique< tag<by_receiver>,member< account_action_history_object, account_id_type, &account_action_history_object::receiver>>,
         ordered_non_unique< tag<by_blocknum>,member< account_action_history_object, uint32_t, &account_action_history_object::block_num>>
      >
   > action_history_multi_index_type;

   typedef generic_index<account_action_history_object, action_history_multi_index_type> action_history_index;


} } // graphene::chain

FC_REFLECT_DERIVED( graphene::chain::account_action_history_object, (graphene::chain::object),
                    (mongodb_id)(sender)(receiver)(act)(inline_actions)(result)(txid)(irreversible_state)(block_num)(trx_in_block)(op_in_trx) )

