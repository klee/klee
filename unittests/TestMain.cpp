//===--- unittests/TestMain.cpp - unittest driver -------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "klee/Support/CompilerWarning.h"
DISABLE_WARNING_PUSH
DISABLE_WARNING_DEPRECATED_DECLARATIONS
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/Signals.h"
DISABLE_WARNING_POP

#include "gtest/gtest.h"

int main(int argc, char **argv) {
  llvm::sys::PrintStackTraceOnErrorSignal(argv[0], true);
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
