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
    database &d = db();

    // check contract name
    auto &acnt_indx = d.get_index_type<account_index>();
    if (op.name.size()) {
        auto current_account_itr = acnt_indx.indices().get<by_name>().find(op.name);
        FC_ASSERT(current_account_itr == acnt_indx.indices().get<by_name>().end(), "Contract Name Existed, please change your contract name.");
    }

    return void_result();
} FC_CAPTURE_AND_RETHROW( (op) ) }

object_id_type contract_deploy_evaluator::do_apply(const contract_deploy_operation &op)
{ try {
    dlog("contract_deploy_evaluator do_apply");
    const auto &params = db().get_global_properties().parameters;
    const auto &new_acnt_object = db().create<account_object>([&](account_object &obj) {
            obj.registrar = op.account;
            obj.referrer = op.account;
            obj.lifetime_referrer = op.account;
            obj.network_fee_percentage = params.network_percent_of_fee;
            obj.lifetime_referrer_fee_percentage = params.lifetime_referrer_percent_of_fee;
            obj.referrer_rewards_percentage = 0;

            obj.name = op.name;
            obj.vm_type = op.vm_type;
            obj.vm_version = op.vm_version;
            obj.code = op.code;
            obj.code_version = fc::sha256::hash(op.code);
            obj.abi = op.abi;
            });

    return new_acnt_object.id;
} FC_CAPTURE_AND_RETHROW((op)) }

void_result contract_call_evaluator::do_evaluate(const contract_call_operation &op)
{ try {
    database& d = db();
    const account_object& contract_obj = op.contract_id(d);
    acnt = &(contract_obj);
    FC_ASSERT(acnt->code.size() > 0, "contract has no code, contract_id ${n}", ("n", op.contract_id));

    // check method_name
    const auto& actions = acnt->abi.actions;
    auto iter = std::find_if(actions.begin(), actions.end(),
            [&](const action_def& act) { return act.name == op.method_name; });
    FC_ASSERT(iter != actions.end(), "method_name ${m} not found in abi", ("m", op.method_name));

    // check balance
    if (op.amount.valid()) {
        FC_ASSERT(op.amount->amount > 0, "amount > 0");
        const asset_object &asset_type = op.amount->asset_id(d);
        bool insufficient_balance = d.get_balance(op.account(d), asset_type).amount >= op.amount->amount;
        FC_ASSERT(insufficient_balance,
                  "insufficient balance: ${balance}, unable to deposit '${total_transfer}' from account '${a}' to '${t}'",
                  ("a", op.account)("t", contract_obj.id)("total_transfer", d.to_pretty_string(op.amount->amount))("balance", d.to_pretty_string(d.get_balance(op.account(d), asset_type))));
    }

    return void_result();
} FC_CAPTURE_AND_RETHROW((op)) }

void_result contract_call_evaluator::do_apply(const contract_call_operation &op)
{ try {
    database& d = db();
    if (op.amount.valid()) {
        auto amnt = *op.amount;
        dlog("adjust balance, amount ${a}", ("a", amnt));
        d.adjust_balance(op.account, -amnt);
        d.adjust_balance(op.contract_id, amnt);
    }

    transaction_context trx_context(d, op.fee_payer().instance);
    action act{op.contract_id, op.amount, op.method_name, op.data};
    apply_context ctx{d, trx_context, act};
    ctx.exec();

    auto fee_param = contract_call_operation::fee_parameters_type();
    idump((fee_param));
    const auto& p = d.get_global_properties().parameters;
    for (auto& param : p.current_fees->parameters) {
        if (param.which() == operation::tag<contract_call_operation>::value) {
            fee_param = param.get<contract_call_operation::fee_parameters_type>();
            dlog("use gpo params, ${s}", ("s", fee_param));
            break;
        }
    }

    uint64_t ram_fee = (uint64_t)(1.0f * ctx.get_ram_usage() / 1000 * fee_param.price_per_kbyte_ram);
    uint64_t cpu_fee = trx_context.get_cpu_usage() * fee_param.price_per_ms_cpu;

    const auto &asset_obj = d.get<asset_object>(op.fee.asset_id);
    asset fee = asset(ram_fee + cpu_fee, asset_id_type()) * asset_obj.options.core_exchange_rate;
    fee_from_account += fee;
    dlog("ram_fee=${rf}, cpu_fee=${cf}, ram_usage=${ru}, cpu_usage=${cu}, ram_price=${rp}, cpu_price=${cp}",
            ("rf",ram_fee)("cf",cpu_fee)("ru",ctx.get_ram_usage())("cu",trx_context.get_cpu_usage())("rp",fee_param.price_per_kbyte_ram)("cp",fee_param.price_per_ms_cpu));
    
    return void_result();
} FC_CAPTURE_AND_RETHROW((op)) }

} } // graphene::chain
