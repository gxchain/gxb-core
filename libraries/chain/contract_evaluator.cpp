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

#include <fc/smart_ref_impl.hpp>
#include <graphene/chain/contract_evaluator.hpp>
#include <graphene/chain/database.hpp>
#include <graphene/chain/protocol/operations.hpp>
#include <graphene/chain/protocol/fee_schedule.hpp>
#include <graphene/chain/protocol/name.hpp>

#include <graphene/chain/apply_context.hpp>
#include <graphene/chain/transaction_context.hpp>
#include <graphene/chain/wasm_interface.hpp>
#include <graphene/chain/wast_to_wasm.hpp>
#include <graphene/chain/abi_serializer.hpp>


namespace graphene { namespace chain {

void_result contract_deploy_evaluator::do_evaluate(const contract_deploy_operation &op)
{ try {
    dlog("contract_deploy_evaluator do_evaluator");

    database &d = db();

    // check contract name
    auto &acnt_indx = d.get_index_type<account_index>();
    if (op.name.size()) {
        auto current_account_itr = acnt_indx.indices().get<by_name>().find(op.name);
        FC_ASSERT(current_account_itr == acnt_indx.indices().get<by_name>().end(), "Contract Name Existed, please change your contract name.");
    }

    return void_result();
} FC_CAPTURE_AND_RETHROW( (op) ) }

object_id_type contract_deploy_evaluator::do_apply(const contract_deploy_operation &o)
{ try {
    dlog("contract_deploy_evaluator do_apply");
    const auto &new_acnt_object = db().create<account_object>([&](account_object &obj) {
            obj.registrar = o.account;
            obj.referrer = o.account;
            obj.lifetime_referrer = o.account;

            auto &params = db().get_global_properties().parameters;
            obj.network_fee_percentage = params.network_percent_of_fee;
            obj.lifetime_referrer_fee_percentage = params.lifetime_referrer_percent_of_fee;
            obj.referrer_rewards_percentage = 0;

            obj.name = o.name;
            obj.vm_type = o.vm_type;
            obj.vm_version = o.vm_version;
            obj.code = o.code;
            obj.code_version = (fc::sha256::hash(o.code)).str();
            obj.abi = o.abi;
            });

    return new_acnt_object.id;
} FC_CAPTURE_AND_RETHROW((o)) }

void_result contract_call_evaluator::do_evaluate(const contract_call_operation &op)
{ try {
    idump((op.act));
    database& d = db();
    const account_object& contract_obj = op.act.contract_id(d);
    FC_ASSERT(contract_obj.code.size() > 0, "contract has no code, contract_id ${n}", ("n", op.act.contract_id));

    acnt = &(contract_obj);

    return void_result();
} FC_CAPTURE_AND_RETHROW((op)) }

void_result contract_call_evaluator::do_apply(const contract_call_operation &op)
{ try {
    database& d = db();
    dlog("call contract, contract_id ${n}, method_name ${m}, data ${d}", ("n", op.act.contract_id)("m", op.act.method_name)("d", op.act.data));

    transaction_context trx_context(d, op.fee_payer().instance);
    apply_context ctx{d, trx_context, op.act};
    ctx.exec();

    // dynamic trx fee
    dlog("before fee_from_account=${b}", ("b", fee_from_account));
    if(!trx_state->skip_fee) {
        // get global fee params
        auto fee_param = contract_call_operation::fee_parameters_type();
        const auto& p = d.get_global_properties().parameters;
        auto itr = p.current_fees->parameters.find(contract_call_operation::fee_parameters_type());
        if (itr != p.current_fees->parameters.end()) {
            fee_param = itr->get<contract_call_operation::fee_parameters_type>();
        }
        idump((fee_param));

        uint64_t ram_fee = ctx.get_ram_usage() * fee_param.price_per_kbyte_ram;
        uint64_t cpu_fee = trx_context.get_cpu_usage() * fee_param.price_per_ms_cpu;

        // TODO: support all asset for fee
        asset fee = asset(ram_fee + cpu_fee, asset_id_type(0));
        fee_from_account += fee;
    }
    dlog("after fee_from_account=${b}", ("b", fee_from_account));

    return void_result();
} FC_CAPTURE_AND_RETHROW((op)) }

void_result contract_deposit_evaluator::do_evaluate(const contract_deposit_operation &op)
{ try {
    idump((op));

    database& d = db();
    const account_object &from_account = op.from(d);
    const account_object &to_account = op.to(d);
    const asset_object &asset_type = op.amount.asset_id(d);

    acnt = &(to_account);

    FC_ASSERT(to_account.code.size() > 0, "contract has no code");
    FC_ASSERT(to_account.abi.actions.size() > 0, "contract has no code");

    bool insufficient_balance = d.get_balance(from_account, asset_type).amount >= op.amount.amount;
    FC_ASSERT(insufficient_balance,
              "Insufficient Balance: ${balance}, unable to transfer '${total_transfer}' from account '${a}' to '${t}'",
              ("a", from_account.name)("t", to_account.name)("total_transfer", d.to_pretty_string(op.amount))("balance", d.to_pretty_string(d.get_balance(from_account, asset_type))));

    return void_result();
} FC_CAPTURE_AND_RETHROW((op)) }

void_result contract_deposit_evaluator::do_apply(const contract_deposit_operation &op)
{ try {
    database& d = db();

    stringstream ss;
    ss << "{\"from\":";
    ss << std::to_string((uint64_t)op.from.instance);
    ss << ",\"value\":{\"amount\":";
    ss << std::to_string(op.amount.amount.value);
    ss << ",\"contract_asset_id\":";
    ss << object_id_type(op.amount.asset_id).instance();
    ss << "}}";
    idump((ss.str()));

    fc::variant action_args_var = fc::json::from_string(ss.str(), fc::json::relaxed_parser);
    abi_serializer abis(acnt->abi);
    auto action_type = abis.get_action_type("deposit");
    GRAPHENE_ASSERT(!action_type.empty(), action_validate_exception, "Unknown action in contract ${contract}", ("contract", acnt->name));
    action act {op.to, N(deposit), abis.variant_to_binary(action_type, action_args_var)};

    // call contract
    transaction_evaluation_state deposit_context(&d);
    deposit_context.skip_fee = true;
    contract_call_operation o;
    o.account = op.from;
    o.act = act;
    o.fee = d.current_fee_schedule().calculate_fee(o);
    deposit_context.skip_fee = true;
    d.apply_operation(deposit_context, o);

    return void_result();
} FC_CAPTURE_AND_RETHROW((op)) }

} } // graphene::chain
