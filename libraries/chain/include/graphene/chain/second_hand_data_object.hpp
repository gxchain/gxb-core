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
#include <graphene/chain/datasource_copyright_object.hpp>

namespace graphene { namespace chain {

/**
 *  @brief Recorded secondary data information for sale
 *  @ingroup object
 *  @ingroup protocol
 */
 class second_hand_data_object : public graphene :: chain :: abstract_object<second_hand_data_object>
 {
   public:
      static const uint8_t space_id = protocol_ids;
      static const uint8_t type_id = datasource_copyright_object_type;
      account_id_type second_hand_account_id; //second-hand account id
      string copyright_hash;
      account_id_type datasource_account_id;
      string data_transaction_request_id;
      time_point_sec create_date_time;
 };


}}

 FC_REFLECT_DERIVED (graphene::chain::second_hand_data_object, (graphene::chain::object), (account_id)(datasource_copyright_id)(data_transaction_request_id)(create_date_time))