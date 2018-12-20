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
#pragma once

#include <fc/api.hpp>

namespace graphene { namespace app {

struct order
{
   double                     price;
   double                     quote;
   double                     base;
};

struct get_table_rows_result {
   vector<fc::variant> rows;
   bool                more = false;
};
struct get_table_rows_params {
    uint64_t lower_bound = 0;
    uint64_t upper_bound = -1; // 0xFFFFFFFFFFFFFFFF
    uint32_t limit = 10;
    std::string index_position = "1"; // 1 - primary (first), 2 - secondary index (in order defined by multi_index), 3 - third index, etc
    optional<bool> reverse = false;
};
}} //

FC_REFLECT( graphene::app::order, (price)(quote)(base) );
FC_REFLECT( graphene::app::get_table_rows_result, (rows)(more) );
FC_REFLECT( graphene::app::get_table_rows_params, (lower_bound)(upper_bound)(limit)(index_position)(reverse));


