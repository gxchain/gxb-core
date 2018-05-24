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

#include <graphene/chain/contract_evaluator.hpp>
#include <graphene/chain/account_object.hpp>
#include <graphene/chain/exceptions.hpp>
#include <graphene/chain/is_authorized_asset.hpp>
#include <graphene/chain/signature_object.hpp>

#include <graphene/chain/apply_context.hpp>
#include <graphene/chain/wasm_interface.hpp>
#include <graphene/chain/wast_to_wasm.hpp>


namespace graphene { namespace chain {

void_result contract_call_evaluator::do_evaluate(const contract_call_operation &op)
{ try {
    dlog("contract_call_evaluator do_evaluator");
    return void_result();
} FC_CAPTURE_AND_RETHROW((op)) }

void_result contract_call_evaluator::do_apply(const contract_call_operation &op)
{ try {
    dlog("call contract, name ${n}, method ${m}, data ${d}", ("n", op.name)("m", op.method)("d", op.data));
    dlog("contract_call_evaluator do_apply");

    static const char wast_code[] = R"=====(
(module
  (import "env" "_fwrite" (func $__fwrite (param i32 i32 i32 i32) (result i32)))
  (import "env" "_stdout" (global $stdoutPtr i32))
  (import "env" "memory" (memory 1))
  (export "main" (func $main))

  (data (i32.const 8) "Hello World!\n")

  (func (export "establishStackSpace") (param i32 i32) (nop))

  (func $main (result i32)
    (local $stdout i32)
    (set_local $stdout (i32.load align=4 (get_global $stdoutPtr)))

    (return (call $__fwrite
       (i32.const 8)         ;; void *ptr    => Address of our string
       (i32.const 1)         ;; size_t size  => Data size
       (i32.const 13)        ;; size_t nmemb => Length of our string
       (get_local $stdout))  ;; stream
    )
  )
)
)=====";

    std::vector<uint8_t> wasm = graphene::chain::wast_to_wasm(wast_code);
    auto code_id = fc::sha256::hash(wast_code, (uint32_t) strlen(wast_code));

    auto wasm_bytes = bytes(wasm.begin(), wasm.end());
    auto wasm_code = shared_string(wasm_bytes.data(), wasm_bytes.data() + wasm_bytes.size());
    dlog("wast code ${c}", ("c", wast_code));
    try {
        action a{1, 1, {}};
        apply_context ap{a};
        wasm_interface(graphene::chain::wasm_interface::vm_type::binaryen).apply(code_id, wasm_code, ap);
        dlog("wasm exec success");
    } catch ( const wasm_exit& ){
        dlog("wasm exec failed");
    }

    return void_result();
} FC_CAPTURE_AND_RETHROW((op)) }

} } // graphene::chain
