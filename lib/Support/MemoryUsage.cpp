//===-- MemoryUsage.cpp ---------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "klee/Internal/System/MemoryUsage.h"
#include <malloc.h>

using namespace klee;

size_t util::GetTotalMemoryUsage() {
  // This is linux platform specific
  struct mallinfo mi = ::mallinfo();
  return mi.uordblks + mi.hblkhd;
}
