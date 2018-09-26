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

#include <graphene/chain/loyalty_evaluator.hpp>
#include <cmath>

namespace graphene { namespace chain {

void_result balance_lock_evaluator::do_evaluate(const balance_lock_operation& op)
{ try {
    database& _db = db();
    // gxs assets
    FC_ASSERT(op.amount.asset_id == GRAPHENE_GXS_ASSET, "lock asset must be GXS");
    FC_ASSERT(op.amount <= _db.get_balance(op.account, GRAPHENE_GXS_ASSET), "account balance not enough");
    FC_ASSERT(op.amount.amount >= GRAPHENE_BLOCKCHAIN_PRECISION, "lock amount must > 1");

    //meme length must less than 63 characters
    FC_ASSERT(op.memo.size() <= GRAPHENE_MAX_ACCOUNT_NAME_LENGTH, "memo too long");

    const chain_parameters& chain_params = _db.get_global_properties().parameters;
    vector< pair<string, interest_rate_t> > params;
    for (auto& ext : chain_params.extensions) {
        if (ext.which() == future_extensions::tag<lock_balance_params_t>::value) {
            params = ext.get<lock_balance_params_t>().params;
            break;
        }
    }
    FC_ASSERT(!params.empty(), "no lock balance params");
    auto iter_param = find_if(params.begin(), params.end(), [&](pair<string, interest_rate_t> p) {
        return p.first == op.program_id;
    });
    FC_ASSERT(iter_param != params.end(), "program_id invalid");
    FC_ASSERT(iter_param->second.is_valid, "program_id offline");
    FC_ASSERT(iter_param->second.interest_rate == op.interest_rate, "input interest_rate invalid");

    uint32_t lock_days = iter_param->second.lock_days;
    FC_ASSERT(lock_days == op.lock_days, "input lock days invalid");
    int32_t delta_seconds = op.create_date_time.sec_since_epoch() - _db.head_block_time().sec_since_epoch();
    FC_ASSERT(std::fabs(delta_seconds) <= LOCKED_BALANCE_EXPIRED_TIME, "create_date_time invalid");
    return void_result();
} FC_CAPTURE_AND_RETHROW( (op) ) }

object_id_type balance_lock_evaluator::do_apply(const balance_lock_operation& op, int32_t billed_cpu_time_us)
{ try {
    database& _db = db();
    const auto& new_object = _db.create<lock_balance_object>([&](lock_balance_object& obj){
        obj.owner = op.account;
        obj.create_date_time = op.create_date_time;
        obj.lock_days = op.lock_days;
        obj.program_id = op.program_id;
        obj.amount = op.amount;
        obj.interest_rate = op.interest_rate;
        obj.memo = op.memo;
    });
    _db.adjust_balance(op.account, -op.amount);
    return new_object.id;
} FC_CAPTURE_AND_RETHROW( (op) ) }

void_result balance_unlock_evaluator::do_evaluate(const balance_unlock_operation& op)
{ try {
    database& _db = db();
    FC_ASSERT(_db.find_object(op.lock_id) != nullptr, "lock_id not found, lock_id ${p}", ("p", op.lock_id));
    lock_balance_obj = &op.lock_id(_db);
    FC_ASSERT(lock_balance_obj->owner == op.account, "account not owner");
    // T+1 mode
    uint32_t past_days = (_db.head_block_time().sec_since_epoch() - lock_balance_obj->create_date_time.sec_since_epoch()) / SECONDS_PER_DAY;
    FC_ASSERT(lock_balance_obj->lock_days <= past_days, "unlock timepoint has not arrived yet");
    return void_result();
} FC_CAPTURE_AND_RETHROW( (op) ) }

void_result balance_unlock_evaluator::do_apply(const balance_unlock_operation& op, int32_t billed_cpu_time_us)
{ try {
    database& _db = db();
    if (nullptr != lock_balance_obj) {
        _db.adjust_balance(op.account, lock_balance_obj->amount);
        _db.remove(*lock_balance_obj);
    }
    return void_result();
} FC_CAPTURE_AND_RETHROW( (op) ) }
} } // namespace graphene::chain
