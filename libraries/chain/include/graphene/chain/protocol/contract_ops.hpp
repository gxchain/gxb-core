/*
    Copyright (C) 2018 gxb

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
#include <graphene/chain/action.hpp>
#include <graphene/chain/abi_def.hpp>

namespace graphene { namespace chain {
struct contract_deploy_operation : public base_operation {
    struct fee_parameters_type {
        uint64_t fee = 1 * GRAPHENE_BLOCKCHAIN_PRECISION;
        uint64_t price_per_kbyte = GRAPHENE_BLOCKCHAIN_PRECISION;
    };

    asset                           fee;
    fc::string                      name;
    account_id_type                 account;
    fc::string                      vm_type;
    fc::string                      vm_version;
    bytes                           code;
    abi_def                         abi;
    extensions_type                 extensions;

    account_id_type fee_payer() const
    {
        return account;
    }

    void validate() const
    {
        FC_ASSERT(fee.amount >= 0, "fee.amount < 0");
        FC_ASSERT(is_valid_name(name), "contract name is invalid");
        FC_ASSERT(code.size() > 0, "contract code cannot be empty");
    }

    share_type calculate_fee(const fee_parameters_type &k) const
    {
        auto core_fee_required = k.fee;
        auto data_fee = calculate_data_fee(fc::raw::pack_size(*this), k.price_per_kbyte);
        core_fee_required += data_fee;
        return core_fee_required;
    }
};

struct contract_call_operation : public base_operation {
    struct fee_parameters_type {
        uint64_t fee =  GRAPHENE_BLOCKCHAIN_PRECISION / 1000;
        uint64_t price_per_kbyte_ram =  GRAPHENE_BLOCKCHAIN_PRECISION / 2;
        uint64_t price_per_ms_cpu = 0;
    };

    asset                                   fee;
    account_id_type                         account;
    account_id_type                         contract_id;
    fc::optional<asset>                     amount;
    action_name                             method_name;
    bytes                                   data;
    extensions_type                         extensions;

    account_id_type fee_payer() const { return account; }

    void validate() const
    {
        FC_ASSERT(fee.amount >= 0);
        FC_ASSERT(data.size() >= 0);
    }

    share_type calculate_fee(const fee_parameters_type &k) const
    {
        // just return basic fee, real fee will be calculated after runing
        return k.fee;
    }
};

} } // graphene::chain

FC_REFLECT(graphene::chain::contract_deploy_operation::fee_parameters_type,
            (fee)(price_per_kbyte))
FC_REFLECT(graphene::chain::contract_deploy_operation,
            (fee)
            (name)
            (account)
            (vm_type)
            (vm_version)
            (code)
            (abi)
            (extensions))

FC_REFLECT(graphene::chain::contract_call_operation::fee_parameters_type,
            (fee)(price_per_kbyte_ram)(price_per_ms_cpu))
FC_REFLECT(graphene::chain::contract_call_operation,
            (fee)
            (account)
            (contract_id)
            (amount)
            (method_name)
            (data)
            (extensions))
