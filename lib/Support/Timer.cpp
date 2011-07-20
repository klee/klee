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

#if LLVM_VERSION_CODE < LLVM_VERSION(2, 9)
#include "llvm/System/Process.h"
#else
#include "llvm/Support/Process.h"
#endif

using namespace klee;
using namespace llvm;

WallTimer::WallTimer() {
  sys::TimeValue now(0,0),user(0,0),sys(0,0);
  sys::Process::GetTimeUsage(now,user,sys);
  startMicroseconds = now.usec();
}

uint64_t WallTimer::check() {
  sys::TimeValue now(0,0),user(0,0),sys(0,0);
  sys::Process::GetTimeUsage(now,user,sys);
  return now.usec() - startMicroseconds;
}
