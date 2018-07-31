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

#pragma once

#include <graphene/chain/protocol/transaction.hpp>
#include <graphene/chain/database.hpp>
#include <graphene/chain/balance_object.hpp>
#include <graphene/chain/evaluator.hpp>
#include <graphene/chain/exceptions.hpp>

namespace graphene { namespace chain {

#define LOCKED_BALANCE_EXPIRED_TIME 30

class balance_lock_evaluator : public evaluator<balance_lock_evaluator>
{
public:
    typedef balance_lock_operation operation_type;

    void_result do_evaluate(const balance_lock_operation& op);
    object_id_type do_apply(const balance_lock_operation& op, int32_t billed_cpu_time_us = 0);
};

class balance_unlock_evaluator : public evaluator<balance_unlock_evaluator>
{
public:
    typedef balance_unlock_operation operation_type;

    void_result do_evaluate(const balance_unlock_operation& op);
    void_result do_apply(const balance_unlock_operation& op, int32_t billed_cpu_time_us = 0);

private:
    const lock_balance_object* lock_balance_obj = nullptr;
};

} } // graphene::chain
