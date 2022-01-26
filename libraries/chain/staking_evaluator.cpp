/*
    Copyright (C) 2019 gxb

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

#include <graphene/chain/staking_evaluator.hpp>
#include <graphene/chain/witness_object.hpp>
#include <cmath>

namespace graphene { namespace chain {

void_result staking_create_evaluator::do_evaluate(const staking_create_operation& op)
{ try {
        database& _db = db();
        // gxc assets
        FC_ASSERT(op.amount.asset_id == GRAPHENE_GXS_ASSET, "staking asset must be GXC");
        FC_ASSERT(op.amount <= _db.get_balance(op.owner, GRAPHENE_GXS_ASSET), "account balance not enough");
        FC_ASSERT(op.amount.amount >= _db.get_vote_params().min_staking_amount, "staking amount must > ${amount}",("amount",_db.get_vote_params().min_staking_amount));

        // Check the staking time, such as 7 days, 30 days, 60 days, 90 days, with global parameters

        const chain_parameters& chain_params = _db.get_global_properties().parameters;
        vector< pair<string, staking_weight_t> > params;
        for (auto& ext : chain_params.extensions) {
            if (ext.which() == future_extensions::tag<staking_params_t>::value) {
                params = ext.get<staking_params_t>().params;
                break;
            }
        }
        FC_ASSERT(!params.empty(), "no staking weight params");
        auto iter_param = find_if(params.begin(), params.end(), [&](pair<string, staking_weight_t> p) {
            return p.first == op.program_id;
        });
        FC_ASSERT(iter_param != params.end(), "program_id invalid");
        FC_ASSERT(iter_param->second.is_valid, "program_id offline");
        FC_ASSERT(iter_param->second.weight == op.weight, "input weight invalid");

        uint32_t staking_days = iter_param->second.staking_days;
        FC_ASSERT(staking_days == op.staking_days, "input staking days invalid");

        auto staking_ranges = _db.get_index_type<staking_index>().indices().get<by_owner>().equal_range(op.owner);
        FC_ASSERT(std::distance(staking_ranges.first, staking_ranges.second) < _db.get_vote_params().max_staking_count, "mortgages have reached their maximum number");

        // check trust_node account
        const auto& witness_objects = _db.get_index_type<witness_index>().indices();
        auto wit_obj_itor = witness_objects.find(op.trust_node);
        FC_ASSERT(wit_obj_itor != witness_objects.end(), "nonexistent trust node account ${id}",("id",op.trust_node));
        FC_ASSERT(wit_obj_itor->is_valid == true, "invalid trust node account ${id}",("id",op.trust_node));

        return void_result();
} FC_CAPTURE_AND_RETHROW( (op) ) }

object_id_type staking_create_evaluator::do_apply(const staking_create_operation& op, int32_t billed_cpu_time_us)
{ try {
    database& _db = db();
    const auto& new_object = _db.create<staking_object>([&](staking_object& obj){
        obj.owner = op.owner;
        obj.create_date_time = _db.head_block_time();
        obj.staking_days = op.staking_days;
        obj.program_id = op.program_id;
        obj.amount = op.amount;
        obj.weight = op.weight;
        obj.trust_node = op.trust_node;
        obj.is_valid = true;
    });
    _db.adjust_balance(op.owner, -op.amount); // adjust balance

    // adjust trust node total_vote_weight
    const auto& witness_objects = _db.get_index_type<witness_index>().indices();
    auto wit_obj_itor = witness_objects.find(op.trust_node);
    _db.modify(*wit_obj_itor, [&](witness_object& obj) {
        obj.total_vote_weights += op.amount.amount * op.weight;
    });
    return new_object.id;
} FC_CAPTURE_AND_RETHROW( (op) ) }

void_result staking_update_evaluator::do_evaluate(const staking_update_operation& op)
{ try {
    database& _db = db();

    // check trust_node account
    const auto& witness_objects = _db.get_index_type<witness_index>().indices();
    auto wit_obj_itor = witness_objects.find(op.trust_node);
    FC_ASSERT(wit_obj_itor != witness_objects.end(), "nonexistent trust node account ${id}",("id",op.trust_node));
    FC_ASSERT(wit_obj_itor->is_valid == true, "invalid trust node account ${id}",("id",op.trust_node));
    // check staking_id
    const auto& staking_objects = _db.get_index_type<staking_index>().indices();
    auto stak_itor  = staking_objects.find(op.staking_id);
    FC_ASSERT(stak_itor != staking_objects.end(), "invalid staking_id ${id}",("id",op.staking_id));
    
    if(_db.head_block_time() > HARDFORK_1027_TIME){
        // check owner
        FC_ASSERT(stak_itor->owner == op.owner, "non-staking owners cannot update");
    }
    auto pre_wit_itor = witness_objects.find(stak_itor->trust_node);
    FC_ASSERT(pre_wit_itor != witness_objects.end(), "The previous trust node no longer exists, ${id}",("id",stak_itor->trust_node));
    FC_ASSERT(pre_wit_itor->total_vote_weights >= (stak_itor->amount.amount * stak_itor->weight), "An error occurred in the old node-ticket statistics");
    
    if(_db.head_block_time() > HARDFORK_1028_TIME){
        // check updated to the same trust node
        FC_ASSERT(stak_itor->trust_node != op.trust_node, "Cannot be updated to the same trust node");
    }

    // T+1 mode
    FC_ASSERT(stak_itor->staking_days * SECONDS_PER_DAY > (_db.head_block_time().sec_since_epoch() - stak_itor->create_date_time.sec_since_epoch()), "staking expired");
    FC_ASSERT(stak_itor->is_valid == true, "staking is not valid");
    return void_result();
} FC_CAPTURE_AND_RETHROW( (op) ) }

operation_result staking_update_evaluator::do_apply(const staking_update_operation& op, int32_t billed_cpu_time_us)
{ try {
    database& _db = db();
    // modify staking object "trust_node" field
    witness_id_type prev_trust_node;
    share_type prev_vote_weights;
    const auto& staking_objects = _db.get_index_type<staking_index>().indices();
    auto stak_itor  = staking_objects.find(op.staking_id);
    _db.modify(*stak_itor, [&](staking_object& obj) {
        prev_trust_node = obj.trust_node;
        prev_vote_weights = obj.amount.amount * obj.weight;
        obj.trust_node = op.trust_node;
    });
    // reduce the number of votes received on the previous node
    const auto& witness_objects = _db.get_index_type<witness_index>().indices();
    auto prev_wit_itor  = witness_objects.find(prev_trust_node);
    prev_vote_weights = std::min(prev_vote_weights,prev_wit_itor->total_vote_weights);
    _db.modify(*prev_wit_itor, [&](witness_object& obj) {
        obj.total_vote_weights -= prev_vote_weights;
    });
    // increase the number of votes for new nodes
    auto wit_obj_itor = witness_objects.find(op.trust_node);
    _db.modify(*wit_obj_itor, [&](witness_object& obj) {
        obj.total_vote_weights += prev_vote_weights;
    });

    return prev_wit_itor->id;
} FC_CAPTURE_AND_RETHROW( (op) ) }

void_result staking_claim_evaluator::do_evaluate(const staking_claim_operation& op)
{ try {
    database& _db = db();
    // check staking_id
    const auto& staking_objects = _db.get_index_type<staking_index>().indices();
    auto stak_itor  = staking_objects.find(op.staking_id);
    FC_ASSERT(stak_itor != staking_objects.end(), "invalid staking_id ${id}",("id",op.staking_id));
    if(_db.head_block_time() > HARDFORK_1027_TIME){
        // check owner
        FC_ASSERT(stak_itor->owner == op.owner, "non-staking owners cannot claim");
    }
    // T+1 mode
    FC_ASSERT(stak_itor->staking_days * SECONDS_PER_DAY < (_db.head_block_time().sec_since_epoch() - stak_itor->create_date_time.sec_since_epoch()), "claim timepoint has not arrived yet");
    if(_db.get_vote_params().staking_mode_on == true){
        FC_ASSERT(stak_itor->is_valid == false,"please wait for the dividend to end");
        }
    return void_result();
} FC_CAPTURE_AND_RETHROW( (op) ) }

operation_result staking_claim_evaluator::do_apply(const staking_claim_operation& op, int32_t billed_cpu_time_us)
{ try {
    database& _db = db();
    const auto& staking_objects = _db.get_index_type<staking_index>().indices();
    auto stak_itor  = staking_objects.find(op.staking_id);
    if(_db.get_vote_params().staking_mode_on == false){ 
        const auto& witness_objects = _db.get_index_type<witness_index>().indices();
        auto wit_obj_itor = witness_objects.find(stak_itor->trust_node);
        _db.modify(*wit_obj_itor, [&](witness_object& obj) {
         share_type total_vote_weights = stak_itor->amount.amount * stak_itor->weight;
         share_type reduce_vote_weights = std::min(total_vote_weights, obj.total_vote_weights);
         obj.total_vote_weights -= reduce_vote_weights;
      });
    }
    asset ret_amount = stak_itor->amount;
    _db.adjust_balance(stak_itor->owner, stak_itor->amount); // adjust balance
    _db.remove(*stak_itor);
    return ret_amount;
} FC_CAPTURE_AND_RETHROW( (op) ) }
} } // namespace graphene::chain
