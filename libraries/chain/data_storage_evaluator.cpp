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

#include <graphene/chain/data_storage_evaluator.hpp>
#include <graphene/chain/account_object.hpp>
#include <graphene/chain/exceptions.hpp>
#include <graphene/chain/is_authorized_asset.hpp>

namespace graphene { namespace chain {
void_result data_storage_evaluator::do_evaluate(const data_storage_operation &op)
{ try {
    const database& d = db();

    // check expiration
    const chain_parameters& chain_parameters = d.get_global_properties().parameters;
    const auto& expiration = op.params.expiration;
    FC_ASSERT(expiration <= d.head_block_time() + chain_parameters.maximum_time_until_expiration
            && expiration >= d.head_block_time());

    // check signature
    dlog("account signature ${s}", ("s", op.signature));

    // check data_storage_object

    // checko account balance
    const account_object &from_account = op.account(d);
    const account_object &to_account = op.proxy_account(d);
    const asset_object &asset_type = op.params.fee.asset_id(d);
    return void_result();
} FC_CAPTURE_AND_RETHROW((op)) }

void_result data_storage_evaluator::do_apply(const data_storage_operation &op)
{ try {
    // create data_storage_object

    // pay to proxy_account
    db().adjust_balance(op.account, -op.params.fee);
    db().adjust_balance(op.proxy_account, op.params.fee);
    return void_result();
} FC_CAPTURE_AND_RETHROW((op)) }

} } // graphene::chain
