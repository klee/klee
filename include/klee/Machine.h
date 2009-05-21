//===-- Machine.h -----------------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef __KLEE_MACHINE_H__
#define __KLEE_MACHINE_H__

#include "klee/Expr.h"

namespace klee {
  namespace machine {
    enum ByteOrder {
      LSB = 0,
      MSB = 1
    };
  }
}

#define kMachineByteOrder      klee::machine::LSB
#define kMachinePointerType    Expr::Int32
#define kMachinePointerSize    4

#endif /* __KLEE_MACHINE_H__ */
