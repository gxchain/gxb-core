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

#include <graphene/chain/league_data_product_evaluator.hpp>
#include <graphene/chain/database.hpp>

namespace graphene { namespace chain {

/******* league_data_product  *******/

void_result league_data_product_create_evaluator::do_evaluate( const league_data_product_create_operation& op )
{ try {
    FC_ASSERT(db().get(op.issuer).is_lifetime_member());

    for (auto& ext : op.extensions) {
        if (ext.which() == future_extensions::tag<pocs_threshold_league_data_product_t>::value) {
            uint64_t pocs_threshold = ext.get<pocs_threshold_league_data_product_t>().pocs_threshold;
            FC_ASSERT(pocs_threshold >= 1, "pocs_threshold must be larger than 0");
            break;
        }
    }
    return void_result();
} FC_CAPTURE_AND_RETHROW( (op) ) }

object_id_type league_data_product_create_evaluator::do_apply(const league_data_product_create_operation& op, int32_t billed_cpu_time_us)
{ try {

   const auto& new_object = db().create<league_data_product_object>( [&]( league_data_product_object& obj ){

           obj.product_name = op.product_name;
           obj.brief_desc = op.brief_desc;
           obj.category_id = op.category_id;
           obj.refer_price = op.refer_price;
           obj.status = data_product_undo_status;
           obj.icon = op.icon;
           obj.schema_contexts = op.schema_contexts;
           obj.issuer = op.issuer;
           obj.create_date_time = op.create_date_time;
           obj.total = 0;
           for (auto& ext : op.extensions) {
              if (ext.which() == future_extensions::tag<pocs_threshold_league_data_product_t>::value) {
                 obj.pocs_threshold = ext.get<pocs_threshold_league_data_product_t>().pocs_threshold;
                 break;
              }
           }

   });
   return  new_object.id;

} FC_CAPTURE_AND_RETHROW( (op) ) }


void_result league_data_product_update_evaluator::do_evaluate( const league_data_product_update_operation& op )
{ try {
    FC_ASSERT(trx_state->_is_proposed_trx);
    for (auto& ext : op.extensions) {
        if (ext.which() == future_extensions::tag<pocs_threshold_league_data_product_t>::value) {
            uint64_t pocs_threshold = ext.get<pocs_threshold_league_data_product_t>().pocs_threshold;
            FC_ASSERT(pocs_threshold >= 1, "pocs_threshold must be larger than 0");
            break;
        }
    }
    return void_result();
} FC_CAPTURE_AND_RETHROW( (op) ) }


void_result league_data_product_update_evaluator::do_apply(const league_data_product_update_operation& op, int32_t billed_cpu_time_us)
{
    try {
        database& _db = db();
        _db.modify(
           _db.get(op.league_data_product),
           [&]( league_data_product_object& obj )
           {
              if( op.new_product_name.valid() )
                 obj.product_name =  *op.new_product_name;
              if( op.new_brief_desc.valid() )
                 obj.brief_desc =  *op.new_brief_desc;
              if( op.new_category_id.valid() )
                 obj.category_id =  *op.new_category_id;
              if( op.new_refer_price.valid() )
                 obj.refer_price =  *op.new_refer_price;
              if( op.new_status.valid() )
                 obj.status =  *op.new_status;
              if( op.new_icon.valid() )
                 obj.icon =  *op.new_icon;
              if( op.new_schema_contexts.valid() )
                 obj.schema_contexts =  *op.new_schema_contexts;
              if(obj.status == data_product_enable_status){
                 // obj.pull_date_time = time_point_sec::time_point_sec(0xffffffff);
              }
              for (auto& ext : op.extensions) {
                 if (ext.which() == future_extensions::tag<pocs_threshold_league_data_product_t>::value) {
                    obj.pocs_threshold = ext.get<pocs_threshold_league_data_product_t>().pocs_threshold;
                    break;
                 }
              }
           });
        return void_result();

} FC_CAPTURE_AND_RETHROW( (op) ) }

}}
