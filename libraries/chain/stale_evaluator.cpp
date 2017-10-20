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
#include <graphene/chain/stale_evaluator.hpp>
#include <graphene/chain/database.hpp>

namespace graphene { namespace chain {

void_result stale_data_market_category_create_evaluator::do_evaluate( const stale_data_market_category_create_operation& op )
{ try {
   return void_result();
} FC_CAPTURE_AND_RETHROW( (op) ) }

object_id_type stale_data_market_category_create_evaluator::do_apply( const stale_data_market_category_create_operation& op )
{ try {
   return  object_id_type();
} FC_CAPTURE_AND_RETHROW( (op) ) }


void_result stale_data_market_category_update_evaluator::do_evaluate( const stale_data_market_category_update_operation& op )
{ try {
   return void_result();
} FC_CAPTURE_AND_RETHROW( (op) ) }

void_result stale_data_market_category_update_evaluator::do_apply( const stale_data_market_category_update_operation& op )
{ try {
  return void_result();
} FC_CAPTURE_AND_RETHROW( (op) ) }

void_result stale_free_data_product_create_evaluator::do_evaluate( const stale_free_data_product_create_operation& op )
{ try {
   return void_result();
} FC_CAPTURE_AND_RETHROW( (op) ) }

object_id_type stale_free_data_product_create_evaluator::do_apply( const stale_free_data_product_create_operation& op )
{ try {
   return  object_id_type();
} FC_CAPTURE_AND_RETHROW( (op) ) }

void_result stale_free_data_product_update_evaluator::do_evaluate( const stale_free_data_product_update_operation& op )
{ try {
    return void_result();
} FC_CAPTURE_AND_RETHROW( (op) ) }

void_result stale_free_data_product_update_evaluator::do_apply( const stale_free_data_product_update_operation& op )
{ try {
    return void_result();
} FC_CAPTURE_AND_RETHROW( (op) ) }

void_result stale_league_create_evaluator::do_evaluate( const stale_league_create_operation& op )
{ try {
    return void_result();
} FC_CAPTURE_AND_RETHROW( (op) ) }

object_id_type stale_league_create_evaluator::do_apply( const stale_league_create_operation& op )
{ try {
   return  object_id_type();
} FC_CAPTURE_AND_RETHROW( (op) ) }

void_result stale_league_update_evaluator::do_evaluate( const stale_league_update_operation& op )
{ try {
    return void_result();
} FC_CAPTURE_AND_RETHROW( (op) ) }

void_result stale_league_update_evaluator::do_apply( const stale_league_update_operation& op )
{ try {
    return void_result();
} FC_CAPTURE_AND_RETHROW( (op) ) }

void_result stale_league_data_product_create_evaluator::do_evaluate( const stale_league_data_product_create_operation& op )
{ try {
    return void_result();
} FC_CAPTURE_AND_RETHROW( (op) ) }

object_id_type stale_league_data_product_create_evaluator::do_apply( const stale_league_data_product_create_operation& op )
{ try {
   return  object_id_type();
} FC_CAPTURE_AND_RETHROW( (op) ) }


void_result stale_league_data_product_update_evaluator::do_evaluate( const stale_league_data_product_update_operation& op )
{ try {
    return void_result();
} FC_CAPTURE_AND_RETHROW( (op) ) }

void_result stale_league_data_product_update_evaluator::do_apply( const stale_league_data_product_update_operation& op )
{ try {
    return void_result();
} FC_CAPTURE_AND_RETHROW( (op) ) }

} } // graphene::chain
