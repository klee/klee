//===--- unittests/TestMain.cpp - unittest driver -------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "klee/Config/Version.h"

#include "llvm/ADT/StringRef.h"
#include "llvm/Support/Signals.h"

#include "gtest/gtest.h"

int main(int argc, char **argv) {

#if LLVM_VERSION_CODE >= LLVM_VERSION(3, 9)
  llvm::sys::PrintStackTraceOnErrorSignal(argv[0], true);
#else
  llvm::sys::PrintStackTraceOnErrorSignal(true);
#endif

  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
