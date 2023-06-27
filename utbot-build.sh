#!/bin/bash
# This script is used to build KLEE as UTBot backend

set -e
set -o pipefail
mkdir -p build
cd build

$UTBOT_CMAKE_BINARY -G Ninja \
  -DCMAKE_PREFIX_PATH=$UTBOT_INSTALL_DIR/lib/cmake/z3 \
  -DCMAKE_LIBRARY_PATH=$UTBOT_INSTALL_DIR/lib \
  -DCMAKE_INCLUDE_PATH=$UTBOT_INSTALL_DIR/include \
  -DENABLE_SOLVER_Z3=TRUE \
  -DENABLE_POSIX_RUNTIME=TRUE \
  -DENABLE_KLEE_UCLIBC=ON \
  -DKLEE_UCLIBC_PATH=$UTBOT_ALL/klee-uclibc \
  -DENABLE_FLOATING_POINT=TRUE \
  -DENABLE_FP_RUNTIME=TRUE \
  -DLLVM_CONFIG_BINARY=$UTBOT_INSTALL_DIR/bin/llvm-config \
  -DLLVMCC=/utbot_distr/install/bin/clang \
  -DLLVMCXX=/utbot_distr/install/bin/clang++ \
  -DENABLE_UNIT_TESTS=TRUE \
  -DENABLE_SYSTEM_TESTS=TRUE \
  -DGTEST_SRC_DIR=$UTBOT_ALL/gtest \
  -DGTEST_INCLUDE_DIR=$UTBOT_ALL/gtest/googletest/include \
  -DCMAKE_INSTALL_PREFIX=$UTBOT_ALL/klee \
  -DENABLE_KLEE_LIBCXX=TRUE \
  -DKLEE_LIBCXX_DIR=$UTBOT_ALL/libcxx/install \
  -DKLEE_LIBCXX_INCLUDE_DIR=$UTBOT_ALL/libcxx/install/include/c++/v1 \
  -DENABLE_KLEE_EH_CXX=TRUE \
  -DKLEE_LIBCXXABI_SRC_DIR=$UTBOT_ALL/libcxx/libcxxabi \
   ..

$UTBOT_CMAKE_BINARY --build .
$UTBOT_CMAKE_BINARY --install .
