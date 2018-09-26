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
#include <graphene/chain/datasource_copyright_evaluator.hpp>
#include <graphene/chain/database.hpp>

namespace graphene { namespace chain {

void_result datasource_copyright_clear_evaluator::do_evaluate( const datasource_copyright_clear_operation& op )
{ try {
    FC_ASSERT(trx_state->_is_proposed_trx);
    return void_result();
} FC_CAPTURE_AND_RETHROW( (op) ) }


void_result datasource_copyright_clear_evaluator::do_apply(const datasource_copyright_clear_operation& op, uint32_t billed_cpu_time_us)
{ try {
    database& _db = db();
    const auto& datasource_copyright_idx = _db.get_index_type<datasource_copyright_index>().indices().get<by_multi_id>();
    const auto range = datasource_copyright_idx.equal_range(boost::make_tuple(op.datasource_account, op.product_id));
    for (const datasource_copyright_object& obj : boost::make_iterator_range(range.first, range.second)) {
        dlog("remove ${o}", ("o", obj));
        _db.remove(obj);
    }
    return void_result();
} FC_CAPTURE_AND_RETHROW((op)) }

}}
