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

#include <graphene/chain/contract_js_evaluator.hpp>
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

#include <algorithm>

namespace graphene { namespace chain {


void_result contract_js_deploy_evaluator::do_evaluate(const contract_js_deploy_operation &op)
{ try {
    database &d = db();

    auto &account_idx_by_name = d.get_index_type<account_index>().indices().get<by_name>();
    auto current_account_itr = account_idx_by_name.find(op.name);
    FC_ASSERT(current_account_itr == account_idx_by_name.end(), "contract name existed, contract name ${n}", ("n", op.name));

    FC_ASSERT(op.code.size() > 0, "contract code cannot be empty");
    FC_ASSERT(op.abi.actions.size() > 0, "contract has no actions");

    return void_result();
} FC_CAPTURE_AND_RETHROW((op.name)(op.account)(op.fee)(op.vm_type)(op.vm_version)(op.abi)) }

object_id_type contract_js_deploy_evaluator::do_apply(const contract_js_deploy_operation &op, uint32_t billed_cpu_time_us)
{ try {
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
            obj.statistics = db().create<account_statistics_object>([&](account_statistics_object& s){s.owner = obj.id;}).id;
            });
    
    return new_acnt_object.id;
} FC_CAPTURE_AND_RETHROW((op.name)(op.account)(op.fee)(op.vm_type)(op.vm_version)(op.abi)) }

contract_receipt contract_js_call_evaluator::contract_exec(database& db, const contract_js_call_operation& op, uint32_t billed_cpu_time_us)
{ try {
    dlog("contract_js_call_evaluator contract_exec");
    return contract_receipt{0, 0, {0, asset_id_type(0)}};
} FC_CAPTURE_AND_RETHROW((op)(billed_cpu_time_us)) }

void_result contract_js_call_evaluator::do_evaluate(const contract_js_call_operation &op)
{ try {
    dlog("contract_js_call_evaluator do_evaluate");
    database& d = db();
    const account_object& contract_obj = op.contract_id(d);
    FC_ASSERT(contract_obj.code.size() > 0, "contract has no code, contract_id ${n}", ("n", op.contract_id));

    const auto& actions = contract_obj.abi.actions;
    auto iter = std::find_if(actions.begin(), actions.end(),
            [&](const action_def& act) { return act.name == op.method_name; });
    FC_ASSERT(iter != actions.end(), "method_name ${m} not found in abi", ("m", op.method_name));
    if (op.amount.valid()) {
        FC_ASSERT(iter->payable, "method_name ${m} not payable", ("m", op.method_name));
        
        FC_ASSERT(op.amount->amount > 0, "amount > 0");
        const asset_object &asset_type = op.amount->asset_id(d);
        bool sufficient_balance = d.get_balance(op.account(d), asset_type).amount >= op.amount->amount;
        FC_ASSERT(sufficient_balance,
                  "insufficient balance: ${balance}, unable to deposit '${total_transfer}' from account '${a}' to '${t}'",
                  ("a", op.account)("t", contract_obj.id)("total_transfer", d.to_pretty_string(op.amount->amount))
                  ("balance", d.to_pretty_string(d.get_balance(op.account(d), asset_type))));
    }

    return void_result();
} FC_CAPTURE_AND_RETHROW((op)) }

operation_result contract_js_call_evaluator::do_apply(const contract_js_call_operation &op, uint32_t billed_cpu_time_us)
{ try {
    dlog("contract_js_call_evaluator do_apply");
    database& d = db();
    if (op.amount.valid()) {
        auto amnt = *op.amount;
        dlog("contract_js_call adjust balance, ${f} -> ${t}, asset ${a}", ("f", op.account)("t", op.contract_id)("a", amnt));
        d.adjust_balance(op.account, -amnt);
        d.adjust_balance(op.contract_id, amnt);
    }

    return contract_exec(d, op, billed_cpu_time_us);
} FC_CAPTURE_AND_RETHROW((op)) }

void contract_js_call_evaluator::convert_fee()
{
    fee_from_account = asset(0);
    core_fee_paid = 0;
}

void contract_js_call_evaluator::pay_fee()
{
}

} } // graphene::chain
