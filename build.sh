#!/bin/bash
#
# Copyright (c) Huawei Technologies Co., Ltd. 2012-2021. All rights reserved.
#

# This script is used to build KLEE as UTBot backend

set -e
set -o pipefail
mkdir -p build
cd build
$UTBOT_CMAKE_BINARY -G Ninja \
  -DCMAKE_PREFIX_PATH=$UTBOT_INSTALL_DIR/lib/cmake/z3 \
  -DCMAKE_LIBRARY_PATH=$UTBOT_INSTALL_DIR/lib \
  -DCMAKE_INCLUDE_PATH=$UTBOT_INSTALL_DIR/include \
  -DENABLE_SOLVER_Z3=ON \
  -DENABLE_POSIX_RUNTIME=ON \
  -DENABLE_FLOATING_POINT=ON \
  -DENABLE_FP_RUNTIME=ON \
  -DLLVM_CONFIG_BINARY=$UTBOT_INSTALL_DIR/bin/llvm-config \
  -DENABLE_UNIT_TESTS=OFF \
  -DENABLE_SYSTEM_TESTS=OFF \
  -DGTEST_SRC_DIR=$UTBOT_ALL/gtest \
  -DGTEST_INCLUDE_DIR=$UTBOT_ALL/gtest/googletest/include \
  -DCMAKE_INSTALL_PREFIX=$UTBOT_ALL/klee/ \
    ..
$UTBOT_CMAKE_BINARY --build .
$UTBOT_CMAKE_BINARY --install .
