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

#include <graphene/chain/data_transaction_evaluator.hpp>
#include <graphene/chain/data_transaction_object.hpp>
#include <graphene/chain/datasource_copyright_object.hpp>
#include <graphene/chain/second_hand_data_object.hpp>
#include <graphene/chain/database.hpp>

#include <boost/range/iterator_range.hpp>
#include <boost/rational.hpp>
#include <boost/algorithm/string.hpp>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

namespace graphene { namespace chain {

// max length of request_id
const uint8_t max_size_request_id = 100;

vector<schema_context_object> get_schema_contexts_from_variant(const variant &v)
{ try {
        league_data_product_object lpo;
        v.as<league_data_product_object>(lpo);
        return lpo.schema_contexts;
    } catch (const fc::exception& e) {
        elog("get free_data_product schema_contexts exception ${e}", ("e", e.to_detail_string()));
    }

    try {
        free_data_product_object fpo;
        v.as<free_data_product_object>(fpo);
        return fpo.schema_contexts;
    } catch (const fc::exception& e) {
        elog("get free_data_product schema_contexts exception ${e}", ("e", e.to_detail_string()));
    }
    return {};
}

void_result data_transaction_create_evaluator::do_evaluate(const data_transaction_create_operation& op)
{ try {
    FC_ASSERT(db().get(op.requester).is_merchant_member());
    FC_ASSERT(op.request_id.size() < max_size_request_id, "request_id ${r} too long, must <= 100", ("r", op.request_id));

    // check request_id
    database& _db = db();
    const auto& data_transaction_idx = _db.get_index_type<data_transaction_index>().indices().get<by_request_id>();
    auto maybe_found = data_transaction_idx.find(op.request_id);
    FC_ASSERT(maybe_found == data_transaction_idx.end(), "request_id already exists!");

    // check product id
    FC_ASSERT(_db.find_object(op.product_id), "product id not found, product_id ${p}", ("p", op.product_id));

    // check league
    if (op.league_id.valid()) {
        // league_data_proudct
        if (auto obj = db().find(*op.league_id)) {
            auto iter = find_if(obj->members.begin(), obj->members.end(),
                    [&](const account_id_type& league_member) {
                        return league_member == op.requester;
                    });
            FC_ASSERT(iter != obj->members.end(), "requester ${r} is not league member!", ("r", op.requester));
        } else {
            elog("league_id not found");
            FC_THROW("league_id not found");
        }
    }
    return void_result();
} FC_CAPTURE_AND_RETHROW((op)) }

object_id_type data_transaction_create_evaluator::do_apply(const data_transaction_create_operation& op, uint32_t billed_cpu_time_us)
{ try {
   // get status
   uint8_t data_status = data_transaction_status_init;

   variant search_data_obj;
   // get object
   if (auto obj = db().find_object(op.product_id)) {
       search_data_obj = obj->to_variant();

       // get schema_contexts from object
       vector<schema_context_object> schema_contexts = get_schema_contexts_from_variant(search_data_obj);

       bool is_private = false;
       for (auto &item : schema_contexts) {
           if (op.version == item.version) {
               // 解析item中的privacy字段
               stringstream s(item.schema_context);
               boost::property_tree::ptree pt;
               read_json(s, pt);
               string flag = pt.get<string>("privacy", "");
               if ("true" == flag) {
                   is_private = true;
               }
           }
       }

       if (is_private) {
           data_status = data_transaction_status_init;
       } else {
           data_status = data_transaction_status_confirmed;
       }
   }

   vector<data_transaction_datasource_status_object> datasources_status;
   if (op.league_id.valid()) {
       // get league members as datasource list
       auto league = db().get(*op.league_id);
       for (auto& member : league.members) {
           data_transaction_datasource_status_object obj;
           obj.datasource = member;
           datasources_status.push_back(obj);
       }
   } else {
       // get free_data_product datasource
       auto product_id = static_cast<free_data_product_id_type>(op.product_id);
       free_data_product_object free_data_obj = db().get(product_id);
       data_transaction_datasource_status_object obj;
       obj.datasource = free_data_obj.datasource;
       datasources_status.push_back(obj);
   }

   const auto& new_object = db().create<data_transaction_object>([&](data_transaction_object& obj) {
       obj.request_id     = op.request_id;
       obj.product_id     = op.product_id;
       if (op.league_id.valid()) {
           obj.league_id = *op.league_id;
       } else {
           obj.league_id = fc::optional<league_id_type>();
       }
       obj.datasources_status= datasources_status;
       obj.version        = op.version;
       obj.params         = op.params;
       obj.status         = data_status;
       obj.requester      = op.requester;
       obj.create_date_time= op.create_date_time;
   });
   return  new_object.id;

} FC_CAPTURE_AND_RETHROW( (op) ) }

void_result data_transaction_update_evaluator::do_evaluate(const data_transaction_update_operation& op)
{ try {
    // check object by request_id
    database& _db = db();
    account_object account = _db.get(op.new_requester);
    FC_ASSERT(account.is_data_transaction_member());
    // get data_transaction_object by request_id
    const auto& data_transaction_idx = _db.get_index_type<data_transaction_index>().indices().get<by_request_id>();
    auto dto = data_transaction_idx.find(op.request_id);
    FC_ASSERT(dto != data_transaction_idx.end());

    // check status, must be 0
    FC_ASSERT(data_transaction_status_init == dto->status, "previoud status must be 0");
    FC_ASSERT(((data_transaction_status_confirmed == op.new_status) || (data_transaction_status_user_rejected == op.new_status)), "new_status cannot out of {1,99}");

    return void_result();
} FC_CAPTURE_AND_RETHROW( (op) ) }

void_result data_transaction_update_evaluator::do_apply(const data_transaction_update_operation& op, uint32_t billed_cpu_time_us)
{ try {
    database& _db = db();
    // get data_transaction_object by request_id
    const auto& data_transaction_idx = _db.get_index_type<data_transaction_index>().indices().get<by_request_id>();
    auto maybe_found = data_transaction_idx.find(op.request_id);
    if (maybe_found == data_transaction_idx.end()) {
        // elog("data_transaction_object not found, request_id: ${request_id}", ("request_id", request_id));
        return void_result();
    }

    const data_transaction_object& data_transaction_obj = *maybe_found;
    _db.modify(data_transaction_obj, [&](data_transaction_object& obj) {
        obj.status = op.new_status;
        if (op.new_memo.valid()) {
            obj.memo = *op.new_memo;
        }
    });
    return void_result();
} FC_CAPTURE_AND_RETHROW((op)) }

void_result data_transaction_datasource_upload_evaluator::do_evaluate(const data_transaction_datasource_upload_operation& op)
{ try {
    // check object by request_id
    database& _db = db();
    account_object account = _db.get(op.requester);
    FC_ASSERT(account.is_data_transaction_member());

    // get data_transaction_object by request_id
    const auto& data_transaction_idx = _db.get_index_type<data_transaction_index>().indices().get<by_request_id>();
    auto dto = data_transaction_idx.find(op.request_id);
    FC_ASSERT(dto != data_transaction_idx.end());

    // op.datasource must in datasources_status list
    auto iter = find_if(dto->datasources_status.begin(), dto->datasources_status.end(),
            [&](const data_transaction_datasource_status_object& status_obj) -> bool {
                return status_obj.datasource == op.datasource;
            });
    FC_ASSERT(iter != dto->datasources_status.end(), "datasource account is not found datasources status!");

    // check status, must be 1
    FC_ASSERT(data_transaction_status_confirmed == dto->status, "data_transaction status ${status} != 1", ("status", dto->status));

    return void_result();
} FC_CAPTURE_AND_RETHROW( (op) ) }

void_result data_transaction_datasource_upload_evaluator::do_apply(const data_transaction_datasource_upload_operation& op, uint32_t billed_cpu_time_us)
{ try {
    database& _db = db();
    // get data_transaction_object by request_id
    const auto& data_transaction_idx = _db.get_index_type<data_transaction_index>().indices().get<by_request_id>();
    auto maybe_found = data_transaction_idx.find(op.request_id);
    if (maybe_found == data_transaction_idx.end()) {
        elog("data_transaction_object not found, request_id: ${request_id}", ("request_id", op.request_id));
        return void_result();
    }

    // set datasource status
    const data_transaction_object& dto = *maybe_found;
    _db.modify(dto, [&](data_transaction_object& obj) {
        for (auto& status_obj : obj.datasources_status) {
            if (status_obj.datasource == op.datasource)
                status_obj.status = data_transaction_datasource_status_uploaded;
        }
    });

    // copyright verify
    bool copyright_verify = false;
    if (auto obj = db().find_object(dto.product_id)) {
        variant search_data_obj = obj->to_variant();
        vector<schema_context_object> schema_contexts = get_schema_contexts_from_variant(search_data_obj);
        for (auto& item : schema_contexts) {
            if (dto.version == item.version) {
                stringstream ss(item.schema_context);
                boost::property_tree::ptree root;
                read_json(ss, root);
                string flag = root.get<string>("copyright_verify", "");
                if ("true" == flag) {
                    copyright_verify = true;
                    break;
                }
            }
        }
    }
    if (!copyright_verify) {
        return void_result();
    }

    // record datasource copyright
    fc::optional<string> copyright_hash = fc::optional<string>();
    for (auto& ext : op.extensions) {
        if (ext.which() == future_extensions::tag<operation_ext_copyright_hash_t>::value) {
            copyright_hash = ext.get<operation_ext_copyright_hash_t>().copyright_hash;
            string hash = *copyright_hash;
            break;
        }
    }
    if (copyright_hash.valid()) {
        // query copyright hash
        const datasource_copyright_index& dco_idx = _db.get_index_type<datasource_copyright_index>();
        auto maybe_dco = dco_idx.indices().get<by_copyright_hash>().find(*copyright_hash);
        // find copyright_object
        if (maybe_dco != dco_idx.indices().get<by_copyright_hash>().end()) {
            if (maybe_dco->datasource_account_id != op.datasource) {
                // second hand datasource
                const auto& new_object = db().create<second_hand_data_object>([&](second_hand_data_object& obj) {
                    obj.second_hand_datasource_id = op.datasource;
                    obj.copyright_hash = *copyright_hash;
                    obj.datasource_account_id = maybe_dco->datasource_account_id;
                    obj.data_transaction_request_id = op.request_id;
                    obj.create_date_time = dto.create_date_time;
                });
                dlog("second_hand_data_object id ${id}", ("id", new_object.id));
            }
        } else { // create datasource copyright object
            const auto& new_object = db().create<datasource_copyright_object>([&](datasource_copyright_object& obj) {
                obj.copyright_hash = *copyright_hash;
                obj.datasource_account_id= op.datasource;
                obj.data_transaction_request_id= op.request_id;
                obj.create_date_time= dto.create_date_time;
            });
            dlog("datasource_copyright_object id ${id}", ("id", new_object.id));
        }
    }

    return void_result();
} FC_CAPTURE_AND_RETHROW((op)) }

void_result data_transaction_datasource_validate_error_evaluator::do_evaluate(const data_transaction_datasource_validate_error_operation& op)
{ try {
    // check object by request_id
    database& _db = db();
    account_object account = _db.get(op.datasource);
    FC_ASSERT(account.is_datasource_member());

    // get data_transaction_object by request_id
    const data_transaction_index& dt_index = _db.get_index_type<data_transaction_index>();
    auto dto = dt_index.indices().get<by_request_id>().find(op.request_id);
    FC_ASSERT(dto != dt_index.indices().get<by_request_id>().end());

    // check status, must be 1
    FC_ASSERT(data_transaction_status_confirmed == dto->status,
              "data_transaction status ${status} != 1", ("status", dto->status));

    // check datasources_status, must be 0
    for (auto& status_obj : dto->datasources_status) {
        if (status_obj.datasource == op.datasource) {
            FC_ASSERT(data_transaction_datasource_status_init == status_obj.status, "datasources_status status must be 0");
        }
    }

    return void_result();
} FC_CAPTURE_AND_RETHROW((op)) }

void_result data_transaction_datasource_validate_error_evaluator::do_apply(const data_transaction_datasource_validate_error_operation& op, uint32_t billed_cpu_time_us)
{ try {
    database& _db = db();
    // get data_transaction_object by request_id
    const auto& data_transaction_idx = _db.get_index_type<data_transaction_index>().indices().get<by_request_id>();
    auto maybe_found = data_transaction_idx.find(op.request_id);
    if (maybe_found == data_transaction_idx.end()) {
        elog("data_transaction_object not found, request_id: ${request_id}", ("request_id", op.request_id));
        return void_result();
    }

    const data_transaction_object &data_transaction_obj = *maybe_found;
    _db.modify(
       data_transaction_obj,
       [&]( data_transaction_object& obj ) {
        for (auto& status_obj : obj.datasources_status) {
            if (status_obj.datasource == op.datasource) {
                status_obj.status = data_transaction_datasource_status_error;
            }
        }
       });
    return void_result();

} FC_CAPTURE_AND_RETHROW( (op) ) }

void_result data_transaction_complain_evaluator::do_evaluate( const data_transaction_complain_operation& op)
{ try {
    database& _db = db();
    account_object datasource_account = _db.get(op.datasource);
    FC_ASSERT(datasource_account.is_datasource_member());
    FC_ASSERT(op.merchant_copyright_hash != op.datasource_copyright_hash);

    // get data_transaction_object
    const auto& data_transaction_idx = _db.get_index_type<data_transaction_index>().indices().get<by_request_id>();
    auto dto = data_transaction_idx.find(op.request_id);
    FC_ASSERT(dto != data_transaction_idx.end(), "data_transaction_object not found, request_id: ${request_id}", ("request_id", op.request_id));

    // get data_transaction_complain_object
    const auto& dtc_idx = _db.get_index_type<data_transaction_complain_index>().indices().get<by_request_id>();
    auto dtc = dtc_idx.find(op.request_id);
    FC_ASSERT(dtc == dtc_idx.end(), "data_transaction_complain_object exists!");

    // check requester
    FC_ASSERT(dto->requester == op.requester, "requester not in data_transaction");
    // check datasource and pay status
    auto iter = std::find_if(
        dto->datasources_status.begin(), dto->datasources_status.end(),
        [&](const data_transaction_datasource_status_object& s) -> bool {
            return s.datasource == op.datasource && s.status == data_transaction_datasource_status_payed;
        });
    FC_ASSERT(iter != dto->datasources_status.end(), "datasource not in datasource_status list, or datasource_status not payed");

    return void_result();
} FC_CAPTURE_AND_RETHROW((op)) }

object_id_type data_transaction_complain_evaluator::do_apply( const data_transaction_complain_operation& op, uint32_t billed_cpu_time_us)
{ try {
    database& _db = db();
    const auto& new_object = _db.create<data_transaction_complain_object>([&](data_transaction_complain_object& obj) {
        obj.data_transaction_request_id = op.request_id;
        obj.requester = op.requester;
        obj.datasource = op.datasource;
        obj.merchant_copyright_hash = op.merchant_copyright_hash;
        obj.datasource_copyright_hash = op.datasource_copyright_hash;
        obj.create_date_time = op.create_date_time;
    });
    dlog("new data_transaction_complain_object ${id}", ("id", new_object.id));
    return new_object.id;
} FC_CAPTURE_AND_RETHROW((op)) }

} } // graphene::chain
