//===-- Timer.cpp ---------------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "klee/Support/Timer.h"

#include "klee/Config/Version.h"
#include "klee/Support/Time.h"

using namespace klee;
using namespace llvm;

WallTimer::WallTimer() {
  startMicroseconds = util::getWallTimeVal().usec();
}

uint64_t WallTimer::check() {
  return util::getWallTimeVal().usec() - startMicroseconds;
}
