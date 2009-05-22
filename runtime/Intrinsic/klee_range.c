//===-- klee_range.c ------------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include <assert.h>
#include <klee/klee.h>

int klee_range(int start, int end, const char* name) {
  int x;

  if (start >= end)
    klee_report_error(__FILE__, __LINE__, "invalid range", "user");

  if (start+1==end) {
    return start;
  } else {
    klee_make_symbolic(&x, sizeof x, name); 

    /* Make nicer constraint when simple... */
    if (start==0) {
      klee_assume((unsigned) x < (unsigned) end);
    } else {
      klee_assume(start <= x);
      klee_assume(x < end);
    }

    return x;
  }
}
