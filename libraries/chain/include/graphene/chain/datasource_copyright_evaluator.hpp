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
#include <graphene/chain/protocol/operations.hpp>
#include <graphene/chain/evaluator.hpp>
#include <graphene/chain/database.hpp>
#include <graphene/chain/datasource_copyright_object.hpp>

namespace graphene { namespace chain {

class datasource_copyright_clear_evaluator : public evaluator<datasource_copyright_clear_evaluator>
{
   public:
       typedef datasource_copyright_clear_operation operation_type;

       void_result do_evaluate(const datasource_copyright_clear_operation& o);
       void_result do_apply(const datasource_copyright_clear_operation& o, uint32_t billed_cpu_time_us = 0);
};

} } // graphene::chain
