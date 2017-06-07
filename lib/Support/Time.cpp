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

#if LLVM_VERSION_CODE >= LLVM_VERSION(4, 0)
#include <chrono>

#include <llvm/Support/Chrono.h>
#else
#include "llvm/Support/TimeValue.h"
#endif

#include "llvm/Support/Process.h"

using namespace llvm;
using namespace klee;

#if LLVM_VERSION_CODE >= LLVM_VERSION(4, 0)
double util::durationToDouble(std::chrono::nanoseconds dur)
{
  return dur.count() / 1e9;
}

double util::getUserTime() {
  sys::TimePoint<> now;
  std::chrono::nanoseconds user, sys;

  sys::Process::GetTimeUsage(now, user, sys);

  return durationToDouble(user);
}

double util::getWallTime() {
  return durationToDouble(getWallTimeVal().time_since_epoch());
}

sys::TimePoint<> util::getWallTimeVal() {
  return std::chrono::system_clock::now();
}

#else
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
#endif
