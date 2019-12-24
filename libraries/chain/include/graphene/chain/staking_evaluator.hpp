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

#pragma once

#include <graphene/chain/database.hpp>
#include <graphene/chain/staking_object.hpp>
#include <graphene/chain/evaluator.hpp>
#include <graphene/chain/exceptions.hpp>

namespace graphene { namespace chain {

#define STAKING_EXPIRED_TIME 30
class staking_create_evaluator : public evaluator<staking_create_evaluator>
{
public:
    typedef staking_create_operation operation_type;

    void_result do_evaluate(const staking_create_operation& op);
    object_id_type do_apply(const staking_create_operation& op, int32_t billed_cpu_time_us = 0);
};

class staking_update_evaluator : public evaluator<staking_update_evaluator>
{
public:
    typedef staking_update_operation operation_type;

    void_result do_evaluate(const staking_update_operation& op);
    void_result do_apply(const staking_update_operation& op, int32_t billed_cpu_time_us = 0);

};

class staking_claim_evaluator : public evaluator<staking_claim_evaluator>
{
public:
    typedef staking_claim_operation operation_type;

    void_result do_evaluate(const staking_claim_operation& op);
    void_result do_apply(const staking_claim_operation& op, int32_t billed_cpu_time_us = 0);

};

} } // graphene::chain
