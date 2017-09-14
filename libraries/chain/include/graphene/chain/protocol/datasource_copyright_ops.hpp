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
#include <graphene/chain/protocol/ext.hpp>
#include <graphene/chain/protocol/special_authority.hpp>
#include <graphene/chain/protocol/types.hpp>

namespace graphene { namespace chain {

struct datasource_copyright_clear_operation : public base_operation {
    struct fee_parameters_type {
        uint64_t fee = 1 * GRAPHENE_BLOCKCHAIN_PRECISION;
    };

    account_id_type         datasource_account;
    object_id_type          product_id;
    asset                   fee;
    extensions_type         extensions;

    account_id_type fee_payer() const { return account_id_type(); }
    void validate() const {
        FC_ASSERT(fee.amount >= 0);
    }
    share_type calculate_fee(const fee_parameters_type& k) const { return k.fee; }

};

} } // graphene::chain

FC_REFLECT(graphene::chain::datasource_copyright_clear_operation::fee_parameters_type, (fee))
FC_REFLECT(graphene::chain::datasource_copyright_clear_operation,
           (datasource_account)(product_id)(fee)(extensions))
