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
#pragma once
#include <graphene/chain/protocol/operations.hpp>
#include <graphene/chain/evaluator.hpp>
#include <graphene/chain/database.hpp>

namespace graphene { namespace chain {

   class pay_data_transaction_evaluator : public evaluator<pay_data_transaction_evaluator>
   {
      public:
         typedef pay_data_transaction_operation operation_type;

         void_result do_evaluate( const pay_data_transaction_operation& op );
         operation_result do_apply(const pay_data_transaction_operation& op, int32_t billed_cpu_time_us = 0);

         // @override
         void prepare_fee(account_id_type account_id, asset fee, const operation& o);

      private:
         // user defined
         void update_league_pocs(league_id_type league_id, object_id_type product_id, const pay_data_transaction_operation& op);
         share_type cut_fee(share_type a, uint16_t p);
         optional<account_object> get_account_by_name(const string& account_name);

      private:
        uint64_t transaction_fee = 0;
   };

} } // graphene::chain

