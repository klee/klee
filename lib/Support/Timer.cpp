//===-- Timer.cpp ---------------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "klee/Config/Version.h"
#include "klee/Internal/Support/Timer.h"

#include "klee/Internal/System/Time.h"

using namespace klee;
using namespace llvm;

WallTimer::WallTimer() {
  startMicroseconds = util::getWallTimeMicros();
}

uint64_t WallTimer::check() {
  return util::getWallTimeMicros() - startMicroseconds;
}
