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

#include <graphene/chain/contract_evaluator.hpp>
#include <graphene/chain/account_object.hpp>
#include <graphene/chain/exceptions.hpp>
#include <graphene/chain/is_authorized_asset.hpp>
#include <graphene/chain/signature_object.hpp>
#include <graphene/chain/hardfork.hpp>
#include <graphene/chain/database.hpp>
#include <graphene/chain/evaluator.hpp>
#include <graphene/db/object_database.hpp>

namespace graphene { namespace chain {

void_result contract_call_evaluator::do_evaluate(const contract_call_operation &op)
{ try {
    dlog("contract_call_evaluator do_evaluator");
    return void_result();
} FC_CAPTURE_AND_RETHROW((op)) }

void_result contract_call_evaluator::do_apply(const contract_call_operation &op)
{ try {
    dlog("contract_call_evaluator do_apply");
    return void_result();
} FC_CAPTURE_AND_RETHROW((op)) }


void_result contract_deploy_evaluator::do_evaluate( const contract_deploy_operation& op )
{ try {

    dlog("contract_deploy_evaluator do_evaluator");
    FC_ASSERT(fee_paying_account->is_lifetime_member(), "Only Lifetime members may deploy an contract.");

    database& d = db();
    auto& acnt_indx = d.get_index_type<account_index>();
    if (op.name.size()) {
        auto current_account_itr = acnt_indx.indices().get<by_name>().find(op.name);
        FC_ASSERT(current_account_itr == acnt_indx.indices().get<by_name>().end(), "Contract Name Existed, please change your contract name.");
    }

    FC_ASSERT(op.vm_type.size(), "invalid vm_type ${vm_type}", ("vm_type", op.vm_type)); //TODO check
    FC_ASSERT(op.vm_version.size(), "invalid vm_version"); //TODO check
    FC_ASSERT(op.code.size(), "code empty"); //TODO check
    FC_ASSERT(op.abi.size(), "abi empty"); //TODO check

    return void_result();
} FC_CAPTURE_AND_RETHROW( (op) ) }

object_id_type contract_deploy_evaluator::do_apply( const contract_deploy_operation& o )
{ try {

    dlog("contract_deploy_evaluator do_apply");

    const auto& new_acnt_object = db().create<account_object>([&]( account_object& obj ) {
            obj.registrar = o.creator_account;
            obj.referrer = o.creator_account;
            obj.lifetime_referrer = o.creator_account;

            auto& params = db().get_global_properties().parameters;
            obj.network_fee_percentage = params.network_percent_of_fee;
            obj.lifetime_referrer_fee_percentage = params.lifetime_referrer_percent_of_fee;
            obj.referrer_rewards_percentage = 0;

            obj.name = o.name;
            obj.vm_type = o.vm_type;
            obj.vm_version = o.vm_version;
            obj.code = o.code;
            obj.abi = o.abi;
            obj.statistics = db().create<account_statistics_object>([&](account_statistics_object& s) {s.owner = obj.id;}).id;

        }
    );

    const auto& dynamic_properties = db().get_dynamic_global_properties();
    db().modify(dynamic_properties, [](dynamic_global_property_object& p) {
        ++p.accounts_registered_this_interval;
    });

    const auto& global_properties = db().get_global_properties();
    if (dynamic_properties.accounts_registered_this_interval
            % global_properties.parameters.accounts_per_fee_scale == 0)
        db().modify(global_properties,
                [&](global_property_object& p) {
                    p.parameters.current_fees->get<account_create_operation>().basic_fee <<= p.parameters.account_fee_scale_bitshifts;
                });

    return new_acnt_object.id;
} FC_CAPTURE_AND_RETHROW((o)) }


} } // graphene::chain
