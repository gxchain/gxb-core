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

#include <graphene/chain/proxy_transfer_evaluator.hpp>
#include <graphene/chain/account_object.hpp>
#include <graphene/chain/exceptions.hpp>
#include <graphene/chain/is_authorized_asset.hpp>
#include <graphene/chain/signature_object.hpp>

namespace graphene { namespace chain {

const uint8_t MAX_OP_STRING_LENGTH = 100;

share_type proxy_transfer_evaluator::cut_fee(share_type a, uint16_t p)
{
    if (a == 0 || p == 0)
        return 0;
    if (p == GRAPHENE_100_PERCENT)
        return a;

    fc::uint128 r(a.value);
    r *= p;
    r /= GRAPHENE_100_PERCENT;
    return r.to_uint64();
}

void_result proxy_transfer_evaluator::do_evaluate(const proxy_transfer_operation &op)
{ try {
    const database& d = db();

    // check memo size
    FC_ASSERT(op.proxy_memo.size() < MAX_OP_STRING_LENGTH, "proxy_memo ${r} too long, exceeds 100 bytes", ("r", op.proxy_memo));
    FC_ASSERT(op.request_params.memo.size() < MAX_OP_STRING_LENGTH, "memo ${r} too long, exceeds 100 bytes", ("r", op.request_params.memo));

    // check expiration
    const chain_parameters& chain_parameters = d.get_global_properties().parameters;
    const auto& expiration = op.request_params.expiration;
    FC_ASSERT(expiration <= d.head_block_time() + chain_parameters.maximum_time_until_expiration
            && expiration > d.head_block_time(), "user request param expiration invalid, ${e}", ("e", expiration));

    const account_object &from_account = op.request_params.from(d);
    // check signatures
    FC_ASSERT(op.request_params.signatures.size() > 0, "no signatures");
    const auto& keys = from_account.active.get_keys();
    FC_ASSERT(keys.size() == 1, "do not support multisig acount, account ${a}", ("a", op.request_params.from));
    FC_ASSERT(op.request_params.verify_proxy_transfer_signature(keys.at(0)), "verify user signature error");

    // check signature_object
    const auto& signature_idx = d.get_index_type<signature_index>().indices().get<by_signature>();
    auto maybe_found = signature_idx.find(op.request_params.signatures.at(0));
    FC_ASSERT(maybe_found == signature_idx.end(), "user request signature already used once! signature ${s}", ("s", op.request_params.signatures.at(0)));

    // check account balance, check blacklist / whitelist
    const account_object &to_account = op.request_params.to(d);
    const auto& fee = op.request_params.amount;
    const asset_object &asset_type = fee.asset_id(d);
    try {
        GRAPHENE_ASSERT(
            is_authorized_asset(d, from_account, asset_type),
            transfer_from_account_not_whitelisted,
            "'from' account ${from} is not whitelisted for asset ${asset}",
            ("from", from_account.name)("asset", fee.asset_id));
        GRAPHENE_ASSERT(
            is_authorized_asset(d, to_account, asset_type),
            transfer_to_account_not_whitelisted,
            "'to' account ${to} is not whitelisted for asset ${asset}",
            ("to", to_account.name)("asset", fee.asset_id));

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
    FC_RETHROW_EXCEPTIONS(error, "Unable to pay ${a} from ${f} to ${t}", ("a", d.to_pretty_string(fee.amount))("f", from_account.name)("t", to_account.name));

} FC_CAPTURE_AND_RETHROW((op)) }

void_result proxy_transfer_evaluator::do_apply(const proxy_transfer_operation &op, int32_t billed_cpu_time_us)
{ try {
    // create signature_object
    const auto& new_object = db().create<signature_object>([&](signature_object& obj) {
            obj.signature     = op.request_params.signatures.at(0);
            obj.expiration    = op.request_params.expiration;
            });
    dlog("signature_object ${o}", ("o", new_object));

    // transfer asset
    share_type commission_amount = cut_fee(op.request_params.amount.amount, op.request_params.percentage);
    asset commission_asset = asset(commission_amount, op.request_params.amount.asset_id);
    asset delta_asset = op.request_params.amount - commission_asset;
    dlog("total amount: ${t}, commission amount ${c}, delta amount ${d}",
            ("t", op.request_params.amount.amount)("c", commission_asset.amount)("d", delta_asset.amount));

    db().adjust_balance(op.get_from_account(), -op.request_params.amount);
    db().adjust_balance(op.get_to_account(), delta_asset);
    db().adjust_balance(op.get_proxy_account(), commission_asset);

    return void_result();
} FC_CAPTURE_AND_RETHROW((op)) }

} } // graphene::chain
