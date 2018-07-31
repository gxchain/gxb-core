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
#include <graphene/chain/protocol/contract_receipt.hpp>

#include <graphene/chain/apply_context.hpp>
#include <graphene/chain/transaction_context.hpp>
#include <graphene/chain/wasm_interface.hpp>
#include <graphene/chain/wast_to_wasm.hpp>
#include <graphene/chain/abi_serializer.hpp>


namespace graphene { namespace chain {

void_result contract_deploy_evaluator::do_evaluate(const contract_deploy_operation &op)
{ try {
    database &d = db();
    const account_object &registrar = op.account(d);

    // check contract name
    auto &account_idx = d.get_index_type<account_index>();
    auto current_account_itr = account_idx.indices().get<by_name>().find(op.name);
    FC_ASSERT(current_account_itr == account_idx.indices().get<by_name>().end(), "contract name existed, contract name ${n}", ("n", op.name));

    FC_ASSERT(op.code.size() > 0, "contract code cannot be empty");
    FC_ASSERT(op.abi.actions.size() > 0, "contract has no actions");

    return void_result();
} FC_CAPTURE_AND_RETHROW( (op) ) }

object_id_type contract_deploy_evaluator::do_apply(const contract_deploy_operation &op, uint32_t billed_cpu_time_us)
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
    if (op.amount.valid()) {
        // check method_name, must be payable
        FC_ASSERT(iter->payable, "method_name ${m} not payable", ("m", op.method_name));
    }


    // check balance
    if (op.amount.valid()) {
        FC_ASSERT(op.amount->amount > 0, "amount > 0");
        // check balance
        const asset_object &asset_type = op.amount->asset_id(d);
        bool insufficient_balance = d.get_balance(op.account(d), asset_type).amount >= op.amount->amount;
        FC_ASSERT(insufficient_balance,
                  "insufficient balance: ${balance}, unable to deposit '${total_transfer}' from account '${a}' to '${t}'",
                  ("a", op.account)("t", contract_obj.id)("total_transfer", d.to_pretty_string(op.amount->amount))("balance", d.to_pretty_string(d.get_balance(op.account(d), asset_type))));
    }

    return void_result();
} FC_CAPTURE_AND_RETHROW((op)) }

operation_result contract_call_evaluator::do_apply(const contract_call_operation &op, uint32_t billed_cpu_time_us)
{ try {
    database& d = db();
    if (op.amount.valid()) {
        auto amnt = *op.amount;
        dlog("adjust balance, amount ${a}", ("a", amnt));
        d.adjust_balance(op.account, -amnt);
        d.adjust_balance(op.contract_id, amnt);
    }

    transaction_context trx_context(d, op.fee_payer().instance);
    action act{op.contract_id, op.method_name, op.data};
    apply_context ctx{d, trx_context, act, op.amount};
    ctx.exec();

    auto fee_param = contract_call_operation::fee_parameters_type();
    const auto& p = d.get_global_properties().parameters;
    for (auto& param : p.current_fees->parameters) {
        if (param.which() == operation::tag<contract_call_operation>::value) {
            fee_param = param.get<contract_call_operation::fee_parameters_type>();
            dlog("use gpo params, ${s}", ("s", fee_param));
            break;
        }
    }


    uint32_t cpu_time_us = billed_cpu_time_us > 0 ? billed_cpu_time_us : trx_context.get_cpu_usage();
    uint32_t ram_usage_bs = ctx.get_ram_usage();
    auto ram_result = fc::uint128(ram_usage_bs * fee_param.price_per_kbyte_ram) / 1024;
    auto cpu_result = fc::uint128( cpu_time_us * fee_param.price_per_ms_cpu);
    uint64_t ram_fee = ram_result.to_uint64();
    uint64_t cpu_fee = cpu_result.to_uint64();

    const auto &asset_obj = d.get<asset_object>(op.fee.asset_id);
    asset fee = asset(ram_fee + cpu_fee, asset_id_type()) * asset_obj.options.core_exchange_rate;
    fee_from_account += fee;
    dlog("ram_fee=${rf}, cpu_fee=${cf}, ram_usage=${ru}, cpu_usage=${cu}, ram_price=${rp}, cpu_price=${cp}",
            ("rf",ram_fee)("cf",cpu_fee)("ru",ctx.get_ram_usage())("cu",trx_context.get_cpu_usage())("rp",fee_param.price_per_kbyte_ram)("cp",fee_param.price_per_ms_cpu));

    contract_receipt receipt{cpu_time_us, ram_usage_bs, fee_from_account};
    return receipt;
} FC_CAPTURE_AND_RETHROW((op)) }

} } // graphene::chain
