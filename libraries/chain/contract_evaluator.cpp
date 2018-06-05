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
#include <graphene/chain/database.hpp>

#include <graphene/chain/apply_context.hpp>
#include <graphene/chain/wasm_interface.hpp>
#include <graphene/chain/wast_to_wasm.hpp>


namespace graphene { namespace chain {

void_result contract_deploy_evaluator::do_evaluate(const contract_deploy_operation &op)
{ try {
        dlog("contract_deploy_evaluator do_evaluator");
        database &d = db();
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
            obj.code_version = o.code_version;
            obj.abi = o.abi;
        });

        return new_acnt_object.id;
} FC_CAPTURE_AND_RETHROW((o)) }

void_result contract_call_evaluator::do_evaluate(const contract_call_operation &op)
{ try {
    dlog("contract_call_evaluator do_evaluator");
    // FC_ASSERT(op.act.name.size() > 0);
    // FC_ASSERT(op.act.method.size() > 0);

    database& d = db();
    auto& acnt_indx = d.get_index_type<account_index>();
    auto current_account_itr = acnt_indx.indices().get<by_name>().find(op.act.name);
    FC_ASSERT(current_account_itr != acnt_indx.indices().get<by_name>().end(), "contract not found, name ${n}", ("n", op.act.name));
    FC_ASSERT(current_account_itr->code.size() > 0, "contract has no code, name ${n}", ("n", op.act.name));
    FC_ASSERT(current_account_itr->abi.size() > 0, "contract has no abi, name ${n}", ("n", op.act.name));

    acnt = &(*current_account_itr);

    return void_result();
} FC_CAPTURE_AND_RETHROW((op)) }

void_result contract_call_evaluator::do_apply(const contract_call_operation &op)
{ try {
    dlog("call contract, name ${n}, method ${m}, data ${d}", ("n", op.act.name)("m", op.act.account)("d", op.act.data));
    dlog("contract_call_evaluator do_apply");

    action a{op.account, op.act.name, {}};
    apply_context ctx{db(), a};
    ctx.exec();

    return void_result();
} FC_CAPTURE_AND_RETHROW((op)) }


} } // graphene::chain
