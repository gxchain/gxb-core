#!/bin/bash
set -x

export LLVM_DIR=/usr/local/Cellar/llvm@4/4.0.1/lib/cmake/llvm
export WASM_LLVM_CONFIG=~/wasm-compiler/llvm/lib/cmake/llv
export WASM_ROOT=~/wasm-compiler/llvm

export DISTCC_HOSTS="192.168.100.158 192.168.100.169"
export set CCACHE_PREFIX=distcc
export CC='ccache gcc'
export CXX='ccache g++'

if [ "$(pgrep distcc)" ] ; then
    echo "distcc already run"
else
    distccd --daemon --allow 0.0.0.0/0
fi

rm -rf build &&  mkdir -p build &&  cd build
cmake -DLLVM_DIR=$LLVM_DIR -DWASM_ROOT=$WASM_ROOT -DOPENSSL_ROOT_DIR=/usr/local/opt/openssl -DOPENSSL_INCLUDE_DIR=/usr/local/opt/openssl/include -DOPENSSL_LIBRARIES=/usr/local/opt/openssl/lib -DCMAKE_BUILD_TYPE=Release ..
make -j8
