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

#pragma once
#include <graphene/chain/protocol/asset.hpp>

namespace graphene { namespace chain {

   struct contract_receipt {
       uint32_t         billed_cpu_time_us = 0;
       uint32_t         ram_usage_bs = 0;
       asset            fee;

       explicit operator std::string() const
       {
           return "{\"billed_cpu_time_us\":" + std::to_string(billed_cpu_time_us) +
                  ", \"ram_usage_bs\":" + std::to_string(ram_usage_bs)  +
                  ", \"fee\":{\"asset_id\":" + std::string(object_id_type(fee.asset_id)) +
                  ", \"amount\":"+ std::to_string(fee.amount.value) + "}}";
       }
   };

} }  // graphene::chain

FC_REFLECT(graphene::chain::contract_receipt,
                            (billed_cpu_time_us)
                            (ram_usage_bs)
                            (fee))
