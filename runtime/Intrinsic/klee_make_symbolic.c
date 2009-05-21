//===-- klee_make_symbolic.c ----------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include <klee/klee.h>

void klee_make_symbolic(void *addr, unsigned nbytes) {
  klee_make_symbolic_name(addr, nbytes, "unnamed");
}
