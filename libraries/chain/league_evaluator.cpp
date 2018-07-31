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

#include <graphene/chain/league_evaluator.hpp>
#include <graphene/chain/database.hpp>

namespace graphene { namespace chain {

/******* league  *******/

void_result league_create_evaluator::do_evaluate( const league_create_operation& op )
{ try {
    // check datasource members, new added
    for (auto &datasource : op.members) {
        FC_ASSERT(db().get(datasource).is_datasource_member());
    }

    // check product_id is existed
    for (auto& data_opt : op.data_products) {
        FC_ASSERT(db().find_object(data_opt), "This product_id is not found, product_id is ${id}", ("id", data_opt));
    }

    // size check
    for (auto& ext : op.extensions) {
        if (ext.which() == future_extensions::tag<pocs_threshold_league_t>::value) {
            // pocs_thresholds, fee_bases -- members
            FC_ASSERT(op.members.size() == ext.get<pocs_threshold_league_t>().pocs_thresholds.size(), "pocs_thresholds size error");
            FC_ASSERT(op.members.size() == ext.get<pocs_threshold_league_t>().fee_bases.size(), "fee_bases size error");
            // product_pocs_weights -- data_products
            FC_ASSERT(op.data_products.size() == ext.get<pocs_threshold_league_t>().product_pocs_weights.size(), "product_pocs_weights size error");
            break;
        }
    }

    // check issuer is lifetime member
    FC_ASSERT(db().get(op.issuer).is_lifetime_member());

    // check category_id is existed
    FC_ASSERT(db().find_object(op.category_id));

    return void_result();
} FC_CAPTURE_AND_RETHROW( (op) ) }

object_id_type league_create_evaluator::do_apply(const league_create_operation& op, int32_t billed_cpu_time_us)
{ try {

   const auto& new_object = db().create<league_object>( [&]( league_object& obj ){
        obj.league_name = op.league_name;
        obj.brief_desc = op.brief_desc;
        obj.data_products = op.data_products;
        obj.prices = op.prices;
        obj.status = league_undo_status;
        obj.category_id = op.category_id;
        obj.icon = op.icon;
        obj.issuer = op.issuer;
        obj.total = 0;
        obj.create_date_time = op.create_date_time;
        obj.recommend_expiration_date_time = op.recommend_expiration_date_time;
        obj.members = op.members;

        // pocs_threshold, 0 as default
        // fee_base, 1 as default
        // pocs_weight, 1 as default
        obj.pocs_thresholds = vector<uint64_t>(op.members.size(), 0);
        obj.fee_bases = vector<uint64_t>(op.members.size(), 1);
        obj.product_pocs_weights = vector<uint64_t>(op.data_products.size(), 1);
        // assign new value
        for (auto& ext : op.extensions) {
           if (ext.which() == future_extensions::tag<pocs_threshold_league_t>::value) {
              obj.pocs_thresholds = ext.get<pocs_threshold_league_t>().pocs_thresholds;
              obj.fee_bases = ext.get<pocs_threshold_league_t>().fee_bases;
              obj.product_pocs_weights = ext.get<pocs_threshold_league_t>().product_pocs_weights;
              break;
           }
        }
   });
   return  new_object.id;

} FC_CAPTURE_AND_RETHROW( (op) ) }

void_result league_update_evaluator::do_evaluate( const league_update_operation& op )
{ try {
   // must be varified before propose transaction being stored
    FC_ASSERT(trx_state->_is_proposed_trx);

    // size check
    for (auto& ext : op.extensions) {
        if (ext.which() == future_extensions::tag<pocs_threshold_league_t>::value) {
            FC_ASSERT(op.new_members->size() == ext.get<pocs_threshold_league_t>().pocs_thresholds.size(), "pocs_thresholds size error");
            FC_ASSERT(op.new_members->size() == ext.get<pocs_threshold_league_t>().fee_bases.size(), "fee_bases size error");
            FC_ASSERT(op.new_data_products->size() == ext.get<pocs_threshold_league_t>().product_pocs_weights.size(), "product_pocs_weights size error");
            break;
        }
    }
    return void_result();
} FC_CAPTURE_AND_RETHROW( (op) ) }

void_result league_update_evaluator::do_apply(const league_update_operation& op, int32_t billed_cpu_time_us)
{
    try {
        database& _db = db();
        _db.modify(
           _db.get(op.league),
           [&]( league_object& obj )
           {
              if(op.new_league_name.valid())
                 obj.league_name =  *op.new_league_name;
              if( op.new_brief_desc.valid() )
                 obj.brief_desc =  *op.new_brief_desc;
              if( op.new_data_products.valid() )
                 obj.data_products = *op.new_data_products;
              if( op.new_prices.valid() )
                 obj.prices =  *op.new_prices;
              if( op.new_status.valid() )
                 obj.status =  *op.new_status;
              if( op.new_category_id.valid() )
                 obj.category_id =  *op.new_category_id;
              if( op.new_icon.valid() )
                 obj.icon =  *op.new_icon;
              if(op.new_recommend_expiration_date_time.valid())
                 obj.recommend_expiration_date_time =  *op.new_recommend_expiration_date_time;
              if (op.new_members.valid())
                 obj.members = *op.new_members;

              obj.pocs_thresholds = vector<uint64_t>(op.new_members->size(), 0);
              obj.fee_bases = vector<uint64_t>(op.new_members->size(), 1);
              obj.product_pocs_weights = vector<uint64_t>(op.new_data_products->size(), 1);
              for (auto& ext : op.extensions) {
                 if (ext.which() == future_extensions::tag<pocs_threshold_league_t>::value) {
                    obj.pocs_thresholds = ext.get<pocs_threshold_league_t>().pocs_thresholds;
                    obj.fee_bases = ext.get<pocs_threshold_league_t>().fee_bases;
                    obj.product_pocs_weights = ext.get<pocs_threshold_league_t>().product_pocs_weights;
                    break;
                 }
              }
           });
        return void_result();

} FC_CAPTURE_AND_RETHROW( (op) ) }
} } // graphene::chain
