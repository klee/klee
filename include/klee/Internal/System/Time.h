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

    /// Seconds spent by this process in user mode.
    double getUserTime();

    /// Wall time in seconds.
    double getWallTime();

    /// Wall time as TimeValue object.
    llvm::sys::TimeValue getWallTimeVal();
  }
}

#endif
