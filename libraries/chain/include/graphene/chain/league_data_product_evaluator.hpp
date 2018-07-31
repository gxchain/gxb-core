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
#include <graphene/chain/league_data_product_object.hpp>
#include <graphene/chain/committee_member_object.hpp>

namespace graphene { namespace chain {

   class league_data_product_create_evaluator : public evaluator<league_data_product_create_evaluator>
   {
      public:
         typedef league_data_product_create_operation operation_type;

         void_result do_evaluate( const league_data_product_create_operation& o );
         object_id_type do_apply(const league_data_product_create_operation& o, int32_t billed_cpu_time_us = 0);
   };

   class league_data_product_update_evaluator : public evaluator<league_data_product_update_evaluator>
   {
      public:
         typedef league_data_product_update_operation operation_type;

         void_result do_evaluate( const league_data_product_update_operation& o );
         void_result do_apply(const league_data_product_update_operation& o, int32_t billed_cpu_time_us = 0);
   };

} } // graphene::chain
