#!/bin/bash
set -x

export LLVM_DIR=/usr/local/Cellar/llvm@4/4.0.1/lib/cmake/llvm
export WASM_LLVM_CONFIG=~/wasm-compiler/llvm/lib/cmake/llv
export WASM_ROOT=~/wasm-compiler/llvm

export CC='ccache gcc'
export CXX='ccache g++'

rm -rf build &&  mkdir -p build &&  cd build
cmake -DLLVM_DIR=$LLVM_DIR -DWASM_ROOT=$WASM_ROOT -DOPENSSL_ROOT_DIR=/usr/local/opt/openssl -DOPENSSL_INCLUDE_DIR=/usr/local/opt/openssl/include -DOPENSSL_LIBRARIES=/usr/local/opt/openssl/lib -DCMAKE_BUILD_TYPE=Release ..
make -j4
