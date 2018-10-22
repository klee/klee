//===--- unittests/TestMain.cpp - unittest driver -------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "gtest/gtest.h"

// WARNING: If LLVM's gtest_main target is reused
//          or is built from LLVM's source tree,
//          this file is ignored. Instead, LLVM's
//          utils/unittest/UnitTestMain/TestMain.cpp
//          is used.

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
