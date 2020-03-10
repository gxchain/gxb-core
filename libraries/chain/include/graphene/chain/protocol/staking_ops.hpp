/*
    Copyright (C) 2019 gxb

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
#include <graphene/chain/protocol/ext.hpp>
#include <graphene/chain/protocol/special_authority.hpp>
#include <graphene/chain/protocol/types.hpp>
#include <graphene/chain/staking_object.hpp>

namespace graphene { namespace chain {

    struct staking_create_operation : public base_operation
    {
        struct fee_parameters_type {
           uint64_t fee  = GRAPHENE_BLOCKCHAIN_PRECISION / 1000;
        };

        asset                                       fee;
        account_id_type                             owner;
        witness_id_type                             trust_node;
        asset                                       amount;
        fc::string                                  program_id;
        uint32_t                                    weight;
        uint32_t                                    staking_days;
        extensions_type                             extensions;

        account_id_type fee_payer()const { return owner; }
        void            validate()const { }
        share_type      calculate_fee(const fee_parameters_type& k) const {
            return k.fee;
        }
    };

    struct staking_update_operation : public base_operation
    {
        struct fee_parameters_type {
           uint64_t fee  = GRAPHENE_BLOCKCHAIN_PRECISION / 1000;
        };

        asset                                       fee;
        account_id_type                             owner;
        witness_id_type                             trust_node;
        staking_id_type                             staking_id;
        extensions_type                             extensions;

        account_id_type fee_payer()const { return owner; }
        void            validate()const { }
        share_type      calculate_fee(const fee_parameters_type& k) const {
            return k.fee;
        }
    };

    struct staking_claim_operation : public base_operation
    {
        struct fee_parameters_type {
           uint64_t fee  = GRAPHENE_BLOCKCHAIN_PRECISION / 1000;
        };

        asset                                       fee;
        account_id_type                             owner;
        staking_id_type                             staking_id;
        extensions_type                             extensions;

        account_id_type fee_payer()const { return owner; }
        void            validate()const { }
        share_type      calculate_fee(const fee_parameters_type& k) const {
            return k.fee;
        }
    };

} } // graphene::chain
FC_REFLECT( graphene::chain::staking_create_operation::fee_parameters_type, (fee) )
FC_REFLECT( graphene::chain::staking_update_operation::fee_parameters_type, (fee) )
FC_REFLECT( graphene::chain::staking_claim_operation::fee_parameters_type, (fee) )


FC_REFLECT( graphene::chain::staking_create_operation,
            (fee)
            (owner)
            (trust_node)
            (amount)
            (program_id)
            (weight)
            (staking_days)
            (extensions)
)
FC_REFLECT( graphene::chain::staking_update_operation,
            (fee)
            (owner)
            (trust_node)
            (staking_id)
            (extensions)
)
FC_REFLECT( graphene::chain::staking_claim_operation,
            (fee)
            (owner)
            (staking_id)
            (extensions)
)
