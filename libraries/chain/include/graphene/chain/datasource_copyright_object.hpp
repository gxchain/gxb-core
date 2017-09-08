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

#include <graphene/chain/protocol/types.hpp>
#include <graphene/db/generic_index.hpp>
#include <graphene/chain/protocol/transaction.hpp>

namespace graphene { namespace chain {

/**
 *  @brief Data source copyright registration
 *  @ingroup object
 *  @ingroup protocol
 */
 class datasource_copyright_object : public graphene :: chain :: abstract_object<datasource_copyright_object>
 {
   public:
      static const uint8_t space_id = protocol_ids;
      static const uint8_t type_id = datasource_copyright_object_type;
      string copyright_hash;
      account_id_type account_id; //datasource account id
      string data_transaction_request_id;
      time_point_sec create_date_time;
 };
}}

 FC_REFLECT_DERIVED (graphene::chain::datasource_copyright_object, (graphene::chain::object), (copyright_hash)(account_id)(data_transaction_request_id)(create_date_time))