//===-- klee_int.c --------------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include <assert.h>
#include "klee/klee.h"

int klee_int(const char *name) {
  int x;
  klee_make_symbolic(&x, sizeof x, name);
  return x;
}
