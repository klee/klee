//===-- Time.h --------------------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef KLEE_UTIL_TIME_H
#define KLEE_UTIL_TIME_H

#if LLVM_VERSION_CODE >= LLVM_VERSION(4, 0)
#include <chrono>

#include "llvm/Support/Chrono.h"
#else
#include "llvm/Support/TimeValue.h"
#endif

namespace klee {
  namespace util {

    /// Seconds spent by this process in user mode.
    double getUserTime();

    /// Wall time in seconds.
    double getWallTime();

    /// Wall time as TimeValue object.
#if LLVM_VERSION_CODE >= LLVM_VERSION(4, 0)
    double durationToDouble(std::chrono::nanoseconds dur);
    llvm::sys::TimePoint<> getWallTimeVal();
#else
    llvm::sys::TimeValue getWallTimeVal();
#endif
  }
}

#endif
