/*
 * Copyright (c) 2015 Cryptonomex, Inc., and contributors.
 *
 * The MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#include <graphene/chain/stale_evaluator.hpp>
#include <graphene/chain/database.hpp>

namespace graphene { namespace chain {

void_result stale_data_market_category_create_evaluator::do_evaluate( const stale_data_market_category_create_operation& op )
{ try {
   return void_result();
} FC_CAPTURE_AND_RETHROW( (op) ) }

object_id_type stale_data_market_category_create_evaluator::do_apply(const stale_data_market_category_create_operation& op, int32_t billed_cpu_time_us)
{ try {
   return  object_id_type();
} FC_CAPTURE_AND_RETHROW( (op) ) }


void_result stale_data_market_category_update_evaluator::do_evaluate(const stale_data_market_category_update_operation& op)
{ try {
   return void_result();
} FC_CAPTURE_AND_RETHROW( (op) ) }

void_result stale_data_market_category_update_evaluator::do_apply(const stale_data_market_category_update_operation& op, int32_t billed_cpu_time_us)
{ try {
  return void_result();
} FC_CAPTURE_AND_RETHROW( (op) ) }

void_result stale_free_data_product_create_evaluator::do_evaluate(const stale_free_data_product_create_operation& op)
{ try {
   return void_result();
} FC_CAPTURE_AND_RETHROW( (op) ) }

object_id_type stale_free_data_product_create_evaluator::do_apply(const stale_free_data_product_create_operation& op, int32_t billed_cpu_time_us)
{ try {
   return  object_id_type();
} FC_CAPTURE_AND_RETHROW( (op) ) }

void_result stale_free_data_product_update_evaluator::do_evaluate( const stale_free_data_product_update_operation& op )
{ try {
    return void_result();
} FC_CAPTURE_AND_RETHROW( (op) ) }

void_result stale_free_data_product_update_evaluator::do_apply(const stale_free_data_product_update_operation& op, int32_t billed_cpu_time_us)
{ try {
    return void_result();
} FC_CAPTURE_AND_RETHROW( (op) ) }

void_result stale_league_create_evaluator::do_evaluate( const stale_league_create_operation& op )
{ try {
    return void_result();
} FC_CAPTURE_AND_RETHROW( (op) ) }

object_id_type stale_league_create_evaluator::do_apply(const stale_league_create_operation& op, int32_t billed_cpu_time_us)
{ try {
   return  object_id_type();
} FC_CAPTURE_AND_RETHROW( (op) ) }

void_result stale_league_update_evaluator::do_evaluate(const stale_league_update_operation& op)
{ try {
    return void_result();
} FC_CAPTURE_AND_RETHROW( (op) ) }

void_result stale_league_update_evaluator::do_apply(const stale_league_update_operation& op, int32_t billed_cpu_time_us)
{ try {
    return void_result();
} FC_CAPTURE_AND_RETHROW( (op) ) }

void_result stale_league_data_product_create_evaluator::do_evaluate( const stale_league_data_product_create_operation& op )
{ try {
    return void_result();
} FC_CAPTURE_AND_RETHROW( (op) ) }

object_id_type stale_league_data_product_create_evaluator::do_apply(const stale_league_data_product_create_operation& op, int32_t billed_cpu_time_us)
{ try {
   return  object_id_type();
} FC_CAPTURE_AND_RETHROW( (op) ) }


void_result stale_league_data_product_update_evaluator::do_evaluate( const stale_league_data_product_update_operation& op )
{ try {
    return void_result();
} FC_CAPTURE_AND_RETHROW( (op) ) }

void_result stale_league_data_product_update_evaluator::do_apply(const stale_league_data_product_update_operation& op, int32_t billed_cpu_time_us)
{ try {
    return void_result();
} FC_CAPTURE_AND_RETHROW( (op) ) }

} } // graphene::chain
