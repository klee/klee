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

namespace klee {
  namespace util {
    // Returns the current time spent by the process in userland in seconds
    double getUserTime();
    // Returns the current wall time in seconds
    double getWallTime();
  }
}

#endif
