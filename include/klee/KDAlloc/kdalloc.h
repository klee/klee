//===-- kdalloc.h -----------------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef KDALLOC_KDALLOC_H
#define KDALLOC_KDALLOC_H

#include "allocator.h"
#include "mapping.h"

namespace klee::kdalloc {
using StackAllocator = Allocator;
using StackAllocatorFactory = AllocatorFactory;
} // namespace klee::kdalloc

#endif
