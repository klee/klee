//===-- klee-choose.c -----------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "klee/klee.h"

uintptr_t klee_choose(uintptr_t n) {
  uintptr_t x;
  klee_make_symbolic(&x, sizeof x, "klee_choose");

  // NB: this will *not* work if they don't compare to n values.
  if(x >= n)
    klee_silent_exit(0);
  return x;
}
