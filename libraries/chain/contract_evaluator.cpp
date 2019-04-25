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

#include <algorithm>

namespace graphene { namespace chain {

void_result contract_deploy_evaluator::do_evaluate(const contract_deploy_operation &op)
{ try {
    database &d = db();

    // check contract name
    auto &account_idx = d.get_index_type<account_index>();
    auto current_account_itr = account_idx.indices().get<by_name>().find(op.name);
    FC_ASSERT(current_account_itr == account_idx.indices().get<by_name>().end(), "contract name existed, contract name ${n}", ("n", op.name));

    FC_ASSERT(op.code.size() > 0, "contract code cannot be empty");
    FC_ASSERT(op.abi.actions.size() > 0, "contract has no actions");

    // validate wasm code
    if (d.head_block_time() > HARDFORK_1006_TIME) {
        wasm_interface::validate(op.code);
    }

    return void_result();
} FC_CAPTURE_AND_RETHROW((op.name)(op.account)(op.fee)(op.vm_type)(op.vm_version)(op.abi)) }

object_id_type contract_deploy_evaluator::do_apply(const contract_deploy_operation &op, uint32_t billed_cpu_time_us)
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

void_result contract_update_evaluator::do_evaluate(const contract_update_operation &op)
{ try {
    database &d = db();

    const account_object& contract_obj = op.contract(d);
    FC_ASSERT(op.owner == contract_obj.registrar, "only owner can update contract, current owner: ${o}", ("o", contract_obj.registrar));
    if(d.head_block_time() > HARDFORK_1015_TIME) {
        FC_ASSERT(contract_obj.code.size() > 0, "can not update a normal account: ${a}", ("a", op.contract));
    }
    if (d.head_block_time() < HARDFORK_1024_TIME) {
        code_hash = fc::sha256::hash(op.code);
        FC_ASSERT(code_hash != contract_obj.code_version, "code not updated");
    }

    FC_ASSERT(op.code.size() > 0, "contract code cannot be empty");

    wasm_interface::validate(op.code);

    FC_ASSERT(op.abi.actions.size() > 0, "contract has no actions");

    if(op.new_owner.valid()) {
    	FC_ASSERT(d.find_object(*op.new_owner), "new owner not exist");
    }

    return void_result();
} FC_CAPTURE_AND_RETHROW((op.owner)(op.contract)(op.fee)(op.abi)) }

void_result contract_update_evaluator::do_apply(const contract_update_operation &op, uint32_t billed_cpu_time_us)
{ try {
    database &d = db();
    const account_object& contract_obj = op.contract(d);

    db().modify(contract_obj, [&](account_object &obj) {
        if(op.new_owner.valid()) {
            obj.registrar = *op.new_owner;
            obj.referrer = *op.new_owner;
            obj.lifetime_referrer = *op.new_owner;
        }
        obj.code = op.code;
        obj.code_version = code_hash;
        obj.abi = op.abi;
    });
    return void_result();
} FC_CAPTURE_AND_RETHROW((op.owner)(op.contract)(op.fee)(op.abi)) }

void_result contract_call_evaluator::do_evaluate(const contract_call_operation &op)
{ try {
    database& d = db();
    const account_object& contract_obj = op.contract_id(d);
    FC_ASSERT(contract_obj.code.size() > 0, "contract has no code, contract_id ${n}", ("n", op.contract_id));

    // check method_name
    const auto& actions = contract_obj.abi.actions;
    auto iter = std::find_if(actions.begin(), actions.end(),
            [&](const action_def& act) { return act.name == op.method_name; });
    FC_ASSERT(iter != actions.end(), "method_name ${m} not found in abi", ("m", op.method_name));
    if (op.amount.valid()) {
        // check method_name, must be payable
        FC_ASSERT(iter->payable, "method_name ${m} not payable", ("m", op.method_name));

        // amount must > 0
        FC_ASSERT(op.amount->amount > 0, "amount must > 0");

        // check balance
        const asset_object &asset_type = op.amount->asset_id(d);
        bool sufficient_balance = d.get_balance(op.account(d), asset_type).amount >= op.amount->amount;
        FC_ASSERT(sufficient_balance,
                  "insufficient balance: ${balance}, unable to deposit '${total_transfer}' from account '${a}' to '${t}'",
                  ("a", op.account)("t", contract_obj.id)("total_transfer", d.to_pretty_string(*(op.amount)))
                  ("balance", d.to_pretty_string(d.get_balance(op.account(d), asset_type))));
    }

    if (d.head_block_time() > HARDFORK_1011_TIME) {
        if (op.fee.amount > 0) {
            // fee_from_account is calculated by evaluator::evaluate()
            // prepare_fee --> do_evaluate -> convert_fee -> pay_fee -> do_apply
            // if cpu_fee charged, this check may fail for cpu time may different for the same opertion
            FC_ASSERT(op.fee >= fee_from_account, "insufficient fee paid in trx, ${a} needed", ("a", d.to_pretty_string(fee_from_account)));
        }
    }

    if (d.head_block_time() > HARDFORK_1016_TIME) {
        // ram-account must exists
        const auto &account_idx = d.get_index_type<account_index>().indices().get<by_name>();
        const auto &account_itr = account_idx.find("ram-account");
        FC_ASSERT(account_itr != account_idx.end(), "ram-account not exist");
        ram_account_id = account_itr->id;
    }

    return void_result();
} FC_CAPTURE_AND_RETHROW((op)) }

operation_result contract_call_evaluator::do_apply(const contract_call_operation &op, uint32_t billed_cpu_time_us)
{ try {
    // do apply:
    //  1. run contract code
    //  2. charge base fee
    //  3. charge ram fee by account

    // charge base fee:
    // 1. calculate base_fee (basic fee + cpu fee)
    // 2. convert base_fee to core asset
    //      2.1 call prepare_fee to calculate fee_from_account and core_fee_paid
    //      2.2 call convert_fee to adjust UIA fee_pool (UIA: user-issued assets)
    // 3. deposit cashback
    // 4. adjust fee_payer's balance

    auto &d = db();

    fc::microseconds max_trx_cpu_us = fc::days(1);
    if (0 == billed_cpu_time_us)
        max_trx_cpu_us = fc::microseconds(std::min(d.get_cpu_limit().trx_cpu_limit, d.get_max_trx_cpu_time()));

    action act{op.account.instance, op.contract_id.instance, op.method_name, op.data};
    if (op.amount.valid()) {
        act.amount.amount = op.amount->amount.value;
        act.amount.asset_id = op.amount->asset_id.instance;
    }

    // run contract code
    transaction_context trx_context(d, op.fee_payer().instance, max_trx_cpu_us);
    apply_context ctx{d, trx_context, act};
    ctx.exec();

    uint32_t cpu_time_us = billed_cpu_time_us > 0 ? billed_cpu_time_us : trx_context.get_cpu_usage();
    fee_param = get_contract_call_fee_parameter(d);

    if (d.head_block_time() > HARDFORK_1016_TIME) {
        // adjust balance and deposit cashback
        charge_base_fee(d, op, cpu_time_us);

        contract_receipt receipt;
        receipt.billed_cpu_time_us = cpu_time_us;
        receipt.fee = fee_from_account;

        account_receipt r;
        auto ram_statistics = trx_context.get_ram_statistics();
        for (const auto &ram : ram_statistics) {
            // map<account, ram_bytes>
            r.account = account_id_type(ram.first);
            r.ram_bytes = ram.second;

            // charge and set ram_fee
            charge_ram_fee_by_account(r, d, op);
            receipt.ram_receipts.push_back(r);
        }

        // reset fee_from_account and core_fee_paid
        convert_fee();

        return receipt;
    } else {
        uint32_t ram_usage_bs = ctx.get_ram_usage();
        auto ram_fee = fc::uint128(ram_usage_bs * fee_param.price_per_kbyte_ram) / 1024;
        auto cpu_fee = fc::uint128(cpu_time_us * fee_param.price_per_ms_cpu);
        share_type fee_core = fee_param.fee + ram_fee.to_uint64() + cpu_fee.to_uint64();

        // convert fee_core to fee_uia
        asset fee_uia = d.from_core_asset(asset{fee_core, d.current_core_asset_id()}, op.fee.asset_id);

        generic_evaluator::prepare_fee(op.fee_payer(), fee_uia, op);
        generic_evaluator::convert_fee();
        generic_evaluator::pay_fee();

        return contract_receipt_old{cpu_time_us, ram_usage_bs, fee_uia};
    }
} FC_CAPTURE_AND_RETHROW((op)(billed_cpu_time_us)) }

void contract_call_evaluator::convert_fee()
{
    // reset member variable of evaluator: fee_from_account and core_fee_paid
    fee_from_account = asset(0);
    core_fee_paid = 0;
}

void contract_call_evaluator::pay_fee()
{
}

contract_call_operation::fee_parameters_type contract_call_evaluator::get_contract_call_fee_parameter(const database &db)
{
    auto fee_param = contract_call_operation::fee_parameters_type();
    const auto& p = db.get_global_properties().parameters;
    for (auto& param : p.current_fees->parameters) {
        if (param.which() == operation::tag<contract_call_operation>::value) {
            fee_param = param.get<contract_call_operation::fee_parameters_type>();
            break;
        }
    }
    return fee_param;
}

void contract_call_evaluator::charge_base_fee(database &db, const contract_call_operation &op, uint32_t cpu_time_us)
{
    // calculate base_fee
    const auto &fee_param = get_contract_call_fee_parameter(db);
    auto cpu_fee = fc::uint128(cpu_time_us * fee_param.price_per_ms_cpu);
    share_type base_fee = fee_param.fee + cpu_fee.to_uint64();

    // convert base_fee to UIA
    asset base_fee_from_account = db.from_core_asset(asset{base_fee, asset_id_type(1)}, op.fee.asset_id);

    // adjust UIA fee_pool
    generic_evaluator::prepare_fee(op.fee_payer(), base_fee_from_account, op);
    generic_evaluator::convert_fee();

    // adjust balance
    db.deposit_contract_call_cashback(op.fee_payer()(db), base_fee);
    db.adjust_balance(op.fee_payer(), -base_fee_from_account);
}

void contract_call_evaluator::charge_ram_fee_by_account(account_receipt &r, database &db, const contract_call_operation &op)
{
    if(0 == r.ram_bytes) {
        r.ram_fee = asset{0, op.fee.asset_id};
        return;
    }

    int64_t ram_fee_core = ceil(1.0 * r.ram_bytes * fee_param.price_per_kbyte_ram / 1024);
    //make sure ram-account have enough GXC to refund
    if (ram_fee_core < 0) {
        asset ram_account_balance = db.get_balance(ram_account_id, asset_id_type(1));
        ram_fee_core = -std::min(ram_account_balance.amount.value, -ram_fee_core);
    }

    if (r.account == op.fee_payer()) { // op.fee_payer can pay fee with any UIA
        asset fee_core = asset{ram_fee_core, asset_id_type(1)};
        // convert fee_core to UIA
        asset fee_uia = db.from_core_asset(fee_core, op.fee.asset_id);
        r.ram_fee = fee_uia;

        if (ram_fee_core < 0) {
            // refund core asset
            db.adjust_balance(op.fee_payer(), -fee_core);
            db.adjust_balance(ram_account_id, fee_core);
        } else {
            // can use any UIA as fee, so need to convert UIA to core asset
            generic_evaluator::prepare_fee(op.fee_payer(), fee_uia, op);
            generic_evaluator::convert_fee();
            db.adjust_balance(op.fee_payer(), -fee_uia);
            db.adjust_balance(ram_account_id, asset{core_fee_paid, asset_id_type(1)});
        }
    } else { // contract as fee payer can only pay fee with core asset
        r.ram_fee = asset{ram_fee_core, asset_id_type(1)};
        db.adjust_balance(r.account, -r.ram_fee);
        db.adjust_balance(ram_account_id, r.ram_fee);
    }
}

void_result inter_contract_call_evaluator::do_evaluate(const inter_contract_call_operation &op)
{ try {
    database &d = db();
    FC_ASSERT(d.get_contract_transaction_ctx() != nullptr, "contract_transaction_ctx invalid");
    return void_result();
} FC_CAPTURE_AND_RETHROW((op.sender_contract)(op.contract_id)(op.fee)(op.amount)(op.method_name)(op.data)) }

void_result inter_contract_call_evaluator::do_apply(const inter_contract_call_operation &op, uint32_t billed_cpu_time_us)
{ try {
    database &d = db();
    transaction_context* contract_transaction_ctx = d.get_contract_transaction_ctx();

    action act{op.sender_contract.instance, op.contract_id.instance, op.method_name, op.data};
    if (op.amount.valid()) {
        act.amount.amount = op.amount->amount.value;
        act.amount.asset_id = op.amount->asset_id.instance;
    }

    apply_context ctx{d, *contract_transaction_ctx, act};
    ctx.exec();
    return void_result();
} FC_CAPTURE_AND_RETHROW((op.sender_contract)(op.contract_id)(op.fee)(op.amount)(op.method_name)(op.data)) }

} } // graphene::chain
