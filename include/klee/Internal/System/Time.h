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

#include <llvm/Support/TimeValue.h>

namespace klee {
  namespace util {

    /// Seconds spent by this process in user mode (usually at least microsecond precision).
    double getUserTime();

    /// Wall time in seconds (usually at least microsecond precision).
    double getWallTime();

    /// Wall time in seconds (usually millisecond precision).
    /// If possible, uses cheaper, coarse and monotonic clock. Neither is guaranteed.
    double getWallTimeCoarse();

    /// Wall time as TimeValue object (usually at least microsecond precision).
    llvm::sys::TimeValue getWallTimeVal();

    /// Wall time as TimeValue object (usually millisecond precision).
    /// If possible, uses cheaper, coarse and monotonic clock. Neither is guaranteed.
    llvm::sys::TimeValue getWallTimeValCoarse();
  }
}

#endif
