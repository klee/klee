//===-- Time.cpp ----------------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "klee/Config/Version.h"
#include "klee/Internal/System/Time.h"

#if LLVM_VERSION_CODE < LLVM_VERSION(2, 9)
#include "llvm/System/Process.h"
#else
#include "llvm/Support/Process.h"
#endif

using namespace llvm;
using namespace klee;

double util::getUserTime() {
  sys::TimeValue now(0,0),user(0,0),sys(0,0);
  sys::Process::GetTimeUsage(now,user,sys);
  return (user.seconds() + (double) user.nanoseconds() * 1e-9);
}

double util::getWallTime() {
  sys::TimeValue now(0,0),user(0,0),sys(0,0);
  sys::Process::GetTimeUsage(now,user,sys);
  return (now.seconds() + (double) now.nanoseconds() * 1e-9);
}
