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

const uint8_t max_op_string_length = 100;

void_result data_storage_evaluator::do_evaluate(const data_storage_operation &op)
{ try {
    const database& d = db();

    FC_ASSERT(op.data_hash.size() < max_op_string_length, "data_hash ${r} too long, must < 100", ("r", op.data_hash));
    FC_ASSERT(op.params.data_md5.size() < max_op_string_length, "data_md5 ${r} too long, must < 100", ("r", op.params.data_md5));

    // check expiration
    const chain_parameters& chain_parameters = d.get_global_properties().parameters;
    const auto& expiration = op.params.expiration;
    FC_ASSERT(expiration <= d.head_block_time() + chain_parameters.maximum_time_until_expiration
            && expiration >= d.head_block_time());

    // check signature
    dlog("account signature ${s}", ("s", op.signature));

    // check data_storage_object

    // check account balance, check blacklist / whitelist
    const account_object &from_account = op.account(d);
    const account_object &to_account = op.proxy_account(d);
    const auto& fee = op.params.fee;
    const asset_object &asset_type = fee.asset_id(d);
    try {
        GRAPHENE_ASSERT(
            is_authorized_asset(d, from_account, asset_type),
            transfer_from_account_not_whitelisted,
            "'from' account ${from} is not whitelisted for asset ${asset}",
            ("from", op.account)("asset", fee.asset_id));
        GRAPHENE_ASSERT(
            is_authorized_asset(d, to_account, asset_type),
            transfer_to_account_not_whitelisted,
            "'to' account ${to} is not whitelisted for asset ${asset}",
            ("to", op.proxy_account)("asset", fee.asset_id));

        if (asset_type.is_transfer_restricted()) {
            GRAPHENE_ASSERT(
                from_account.id == asset_type.issuer || to_account.id == asset_type.issuer,
                transfer_restricted_transfer_asset,
                "Asset ${asset} has transfer_restricted flag enabled",
                ("asset", fee.asset_id));
        }

        bool insufficient_balance = d.get_balance(from_account, asset_type).amount >= fee.amount;
        FC_ASSERT(insufficient_balance,
                  "Insufficient Balance: ${balance}, unable to transfer '${total_transfer}' from account '${a}' to '${t}'",
                  ("a", from_account.name)("t", to_account.name)("total_transfer", d.to_pretty_string(fee.amount))("balance", d.to_pretty_string(d.get_balance(from_account, asset_type))));

        return void_result();
    }
    FC_RETHROW_EXCEPTIONS(error, "Unable to pay ${a} from ${f} to ${t}", ("a", d.to_pretty_string(fee.amount))("f", op.account)("t", op.proxy_account));

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
