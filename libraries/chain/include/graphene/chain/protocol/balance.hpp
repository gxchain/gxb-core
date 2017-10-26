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
#include <graphene/chain/protocol/base.hpp>

namespace graphene { namespace chain { 

   /**
    * @brief Claim a balance in a @ref balanc_object
    *
    * This operation is used to claim the balance in a given @ref balance_object. If the balance object contains a
    * vesting balance, @ref total_claimed must not exceed @ref balance_object::available at the time of evaluation. If
    * the object contains a non-vesting balance, @ref total_claimed must be the full balance of the object.
    */
   struct balance_claim_operation : public base_operation
   {
      struct fee_parameters_type {};

      asset             fee;
      account_id_type   deposit_to_account;
      balance_id_type   balance_to_claim;
      public_key_type   balance_owner_key;
      asset             total_claimed;

      account_id_type fee_payer()const { return deposit_to_account; }
      share_type      calculate_fee(const fee_parameters_type& )const { return 0; }
      void            validate()const;
      void            get_required_authorities( vector<authority>& a )const
      {
         a.push_back( authority( 1, balance_owner_key, 1 ) );
      }
   };

    /**
    * @brief a balance in a @ref balanc_locked_object
    *
    * This operation is used to the balance in a given @ref balanc_locked_object. 
    */
   struct balance_lock_operation : public base_operation
   {
      struct fee_parameters_type { 
         uint64_t fee = 1 * GRAPHENE_BLOCKCHAIN_PRECISION; 
      };
      asset                             fee;
      account_id_type                   locked_account;
      time_point_sec                    create_time;
      string                            locked_time_type;
      asset_id_type                     asset_type;
      share_type                        locked_balance;
      uint32_t                          interest_rate = 0;
      string                            memo;
      extensions_type                   extensions;
      account_id_type fee_payer()const { return locked_account; }
      void validate() const {
         FC_ASSERT(fee.amount >= 0);
      }
      share_type      calculate_fee(const fee_parameters_type& k)const { return k.fee; }
   };

   struct balance_unlock_operation : public base_operation
   {
      struct fee_parameters_type { 
        uint64_t fee = 0; 
      };
      asset                             fee;
      account_id_type                   unlocked_account;
      account_balance_locked_id_type    account_balance_locked;
      extensions_type                   extensions;
      account_id_type fee_payer()const { return unlocked_account; }
      void validate() const {
         FC_ASSERT(fee.amount >= 0);
      }
      share_type      calculate_fee(const fee_parameters_type& k)const { return k.fee; }
   };
} } // graphene::chain

FC_REFLECT( graphene::chain::balance_claim_operation::fee_parameters_type,  )
FC_REFLECT( graphene::chain::balance_claim_operation,
            (fee)(deposit_to_account)(balance_to_claim)(balance_owner_key)(total_claimed) )

FC_REFLECT( graphene::chain::balance_lock_operation::fee_parameters_type,  )
FC_REFLECT( graphene::chain::balance_lock_operation,
            (fee)(locked_account)(create_time)(locked_time_type)(asset_type)(locked_balance)(interest_rate)(memo)(extensions) )

FC_REFLECT( graphene::chain::balance_unlock_operation::fee_parameters_type,  )
FC_REFLECT( graphene::chain::balance_unlock_operation,
            (fee)(unlocked_account)(account_balance_locked)(extensions) )
