/*
 * Copyright (c) 2015 Cryptonomex, Inc., and contributors.
 *
 * The MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <graphene/chain/database.hpp>

#include <graphene/chain/asset_object.hpp>
#include <graphene/chain/chain_property_object.hpp>
#include <graphene/chain/global_property_object.hpp>

#include <fc/smart_ref_impl.hpp>

namespace graphene { namespace chain {

const asset_object& database::get_core_asset() const
{
    if (head_block_time() > HARDFORK_1008_TIME) {
        return get(asset_id_type(1));
    } else {
        return get(asset_id_type());
    }
}

const global_property_object& database::get_global_properties()const
{
   return get( global_property_id_type() );
}

const data_transaction_commission_percent_t database::get_commission_percent() const
{
    const chain_parameters& params = get_global_properties().parameters;
    for (auto& ext : params.extensions) {
        if (ext.which() == future_extensions::tag<data_transaction_commission_percent_t>::value) {
            return ext.get<data_transaction_commission_percent_t>();
        }
    }
    // return default value
    return data_transaction_commission_percent_t();
}

const vm_cpu_limit_t database::get_cpu_limit() const
{
    const chain_parameters& params = get_global_properties().parameters;
    for (auto& ext : params.extensions) {
        if (ext.which() == future_extensions::tag<vm_cpu_limit_t>::value) {
            return ext.get<vm_cpu_limit_t>();
        }
    }
    // return default value
    return vm_cpu_limit_t();
}

const trust_node_pledge_t database::get_trust_node_pledge() const
{
	const chain_parameters& params = get_global_properties().parameters;
	for (auto& ext : params.extensions) {
		if (ext.which() == future_extensions::tag<trust_node_pledge_t>::value) {
			return ext.get<trust_node_pledge_t>();
		}
	}
	// return default value
	return trust_node_pledge_t();
}

const inter_contract_calling_params_t& database::get_inter_contract_calling_params() const
{
    const chain_parameters& params = get_global_properties().parameters;
    for (auto& ext : params.extensions) {
        if (ext.which() == future_extensions::tag<inter_contract_calling_params_t>::value) {
            return ext.get<inter_contract_calling_params_t>();
        }
    }

    // return default value
    static inter_contract_calling_params_t default_params;
    return default_params;
}

const chain_property_object& database::get_chain_properties()const
{
   return get( chain_property_id_type() );
}

const dynamic_global_property_object&database::get_dynamic_global_properties() const
{
   return get( dynamic_global_property_id_type() );
}

const fee_schedule&  database::current_fee_schedule()const
{
   return get_global_properties().parameters.current_fees;
}

time_point_sec database::head_block_time()const
{
   return get( dynamic_global_property_id_type() ).time;
}

uint32_t database::head_block_num()const
{
   return get( dynamic_global_property_id_type() ).head_block_number;
}

block_id_type database::head_block_id()const
{
   return get( dynamic_global_property_id_type() ).head_block_id;
}

decltype( chain_parameters::block_interval ) database::block_interval( )const
{
   return get_global_properties().parameters.block_interval;
}

const chain_id_type& database::get_chain_id( )const
{
   return get_chain_properties().chain_id;
}

const node_property_object& database::get_node_properties()const
{
   return _node_property_object;
}

node_property_object& database::node_properties()
{
   return _node_property_object;
}

uint32_t database::last_non_undoable_block_num() const
{
   return head_block_num() - (_undo_db.size() - _undo_db.active_sessions());
}

asset_id_type database::current_core_asset_id()
{
    asset_id_type core_asset_id = asset_id_type();
    if (head_block_time() > HARDFORK_1008_TIME)
    {
        core_asset_id = asset_id_type(1);
    }
    return core_asset_id;
}

asset database::from_core_asset(const asset &a, const asset_id_type &id)
{
    asset_id_type core_asset_id = current_core_asset_id();
    FC_ASSERT(a.asset_id == core_asset_id, "asset is not core_asset");

    if(id == core_asset_id) {
        return a;
    }

    const auto &pr = get<asset_object>(id).options.core_exchange_rate;
    if (head_block_time() > HARDFORK_1013_TIME)
    {
        return asset(a.amount * pr.quote.amount / pr.base.amount, id);
    }
    else
    {
        return asset(a.amount / uint64_t(pr.to_real()), id);
    }
}
asset database::get_update_contract_fee( const operation& op, asset_id_type id )
{
    fc::variant result;
    FC_ASSERT(op.which() == operation::tag<contract_update_operation>::value, "get_update_contract_fee error: op type is not contract_update_operation");
    const contract_update_operation &cu_op = op.get<contract_update_operation>();
    auto &contract_account = cu_op.contract;
    auto acc_idx = get_index_type<account_index>().indices();
    auto itor = acc_idx.find(contract_account);
    FC_ASSERT(itor != acc_idx.end(), "contract account is not exist");
    auto oldsize = fc::raw::pack_size(itor->abi) + itor->code.size();
    auto newsize = fc::raw::pack_size(cu_op.abi) + cu_op.code.size();
    share_type amount;
    if (oldsize >= newsize) {
        amount = contract_update_operation::fee_parameters_type().fee;
    } else {
        amount = contract_update_operation::fee_parameters_type().fee;
        auto data_fee = cu_op.calculate_data_fee(newsize - oldsize, contract_update_operation::fee_parameters_type().price_per_kbyte);
        amount += data_fee;
    }
    asset_id_type core_asset_id = current_core_asset_id();
    asset op_fee(amount, core_asset_id);
    op_fee = from_core_asset(op_fee, id);
    return op_fee;
}

} }
