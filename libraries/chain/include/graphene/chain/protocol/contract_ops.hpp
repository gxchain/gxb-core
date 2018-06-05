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
#include <graphene/chain/protocol/ext.hpp>
#include <graphene/chain/action.hpp>

namespace graphene { namespace chain {
struct contract_deploy_operation : public base_operation {
    struct fee_parameters_type {
        uint64_t fee = 1000 * GRAPHENE_BLOCKCHAIN_PRECISION;
        uint64_t price_per_kbyte = GRAPHENE_BLOCKCHAIN_PRECISION;
    };

    asset                           fee;
    fc::string                      name;
    account_id_type                 account;

    fc::string                      vm_type;
    fc::string                      vm_version;
    std::vector<uint8_t>            code;
    digest_type                     code_version;
    fc::string                      abi;

    account_id_type fee_payer() const
    {
        return account;
    }

    void validate() const
    {
        FC_ASSERT(fee.amount >= 0, "fee.amount < 0");
        FC_ASSERT(is_valid_name(name), "contract name is invalid");
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
        uint64_t fee = 0 * GRAPHENE_BLOCKCHAIN_PRECISION;
    };

    account_id_type                         account;
    asset                                   fee;
    action                                  act;
    extensions_type                         extensions;

    account_id_type fee_payer() const { return account; }

    void validate() const
    {
        FC_ASSERT(fee.amount >= 0);
        FC_ASSERT(act.data.size() > 0);
    }

    share_type calculate_fee(const fee_parameters_type &k) const
    {
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
            (code_version)
            (abi))

FC_REFLECT(graphene::chain::contract_call_operation::fee_parameters_type, (fee))

FC_REFLECT(graphene::chain::contract_call_operation,
            (account)
            (fee)
            (act)
            (extensions))


