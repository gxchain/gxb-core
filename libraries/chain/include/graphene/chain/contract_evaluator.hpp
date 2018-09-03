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
#pragma once
#include <graphene/chain/protocol/operations.hpp>
#include <graphene/chain/evaluator.hpp>

namespace graphene { namespace chain {

class contract_deploy_evaluator : public evaluator<contract_deploy_evaluator>
{
  public:
    typedef contract_deploy_operation operation_type;

    void_result do_evaluate(const contract_deploy_operation &op);
    object_id_type do_apply(const contract_deploy_operation &op, uint32_t billed_cpu_time_us = 0);
};

class contract_call_evaluator : public evaluator<contract_call_evaluator>
{
  public:
    typedef contract_call_operation operation_type;

    void_result do_evaluate(const contract_call_operation &op);
    operation_result do_apply(const contract_call_operation &op, uint32_t billed_cpu_time_us = 0);

    contract_receipt contract_exec(database& db, const contract_call_operation& op, uint32_t billed_cpu_time_us);

    virtual void convert_fee() override;
    virtual void pay_fee() override;
};
} }
