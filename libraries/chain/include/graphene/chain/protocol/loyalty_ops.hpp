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
#include <graphene/chain/protocol/base.hpp>

namespace graphene { namespace chain {

    /**
    * @brief a balance in a @ref balanc_locked_object
    *
    * This operation is used to the balance in a given @ref balanc_locked_object.
    */
   struct balance_lock_operation : public base_operation
   {
      struct fee_parameters_type {
         uint64_t fee = 0;
      };
      asset                             fee;
      account_id_type                   account;
      time_point_sec                    create_date_time;
      fc::string                        program_id;
      asset                             amount;
      uint32_t                          lock_days = 0;
      uint32_t                          interest_rate = 0;
      fc::string                        memo;
      extensions_type                   extensions;

      account_id_type fee_payer()const { return account; }
      void validate() const {
         FC_ASSERT(fee.amount >= 0);
         FC_ASSERT(amount.asset_id == GRAPHENE_GXS_ASSET);
      }
      share_type      calculate_fee(const fee_parameters_type& k)const { return k.fee; }
   };

   struct balance_unlock_operation : public base_operation
   {
      struct fee_parameters_type {
         uint64_t fee = 0;
      };

      asset                             fee;
      account_id_type                   account;
      lock_balance_id_type              lock_id;
      extensions_type                   extensions;
      account_id_type fee_payer()const { return account; }
      void validate() const {
         FC_ASSERT(fee.amount >= 0);
      }
      share_type      calculate_fee(const fee_parameters_type& k)const { return k.fee; }
   };
} } // graphene::chain


FC_REFLECT( graphene::chain::balance_lock_operation::fee_parameters_type,  )
FC_REFLECT( graphene::chain::balance_lock_operation,
            (fee)(account)(create_date_time)(program_id)(amount)(lock_days)(interest_rate)(memo)(extensions) )

FC_REFLECT( graphene::chain::balance_unlock_operation::fee_parameters_type,  )
FC_REFLECT( graphene::chain::balance_unlock_operation,
            (fee)(account)(lock_id)(extensions) )
