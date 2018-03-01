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
#include <graphene/chain/protocol/data_storage_params.hpp>

namespace graphene { namespace chain {

struct data_storage_operation : public base_operation {
    struct fee_parameters_type {
        uint64_t fee = 0;
    };

    account_id_type                 proxy_account;
    account_id_type                 account;
    fc::string                      data_hash;
    asset                           fee;
    data_storage_params             params;
    signature_type                  signature;
    extensions_type                 extensions;

    account_id_type fee_payer() const { return proxy_account; }

    void validate() const
    {
        FC_ASSERT(data_hash.size() > 0);
    }

    share_type calculate_fee(const fee_parameters_type &k) const
    {
        return k.fee;
    }
};


} } // graphene::chain

FC_REFLECT(graphene::chain::data_storage_operation::fee_parameters_type, (fee))

FC_REFLECT(graphene::chain::data_storage_operation,
            (proxy_account)
            (account)
            (data_hash)
            (fee)
            (params)
            (signature)
            (extensions))
