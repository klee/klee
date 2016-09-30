//===-- Time.cpp ----------------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "klee/Support/Time.h"

#include "klee/Config/Version.h"
#include "llvm/Support/Process.h"
#include "llvm/Support/TimeValue.h"

using namespace llvm;
using namespace klee;

double util::getUserTime() {
  sys::TimeValue now(0,0),user(0,0),sys(0,0);
  sys::Process::GetTimeUsage(now,user,sys);
  return (user.seconds() + (double) user.nanoseconds() * 1e-9);
}

double util::getWallTime() {
  sys::TimeValue now = getWallTimeVal();
  return (now.seconds() + ((double) now.nanoseconds() * 1e-9));
}

sys::TimeValue util::getWallTimeVal() {
  return sys::TimeValue::now();
}
