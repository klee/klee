//===-- MemoryUsage.h -------------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef KLEE_MEMORYUSAGE_H
#define KLEE_MEMORYUSAGE_H

#include <cstddef>

namespace klee {
  namespace util {
    size_t GetTotalMallocUsage();
  }
}

#endif /* KLEE_MEMORYUSAGE_H */
