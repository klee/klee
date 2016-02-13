/*
 * Copyright 2016 National University of Singapore
 *
 * This is an example where Z3 would be strong enough to solve some of
 * the quantified formulas obtaining some subsumptions.
 */
#include <klee/klee.h>
#include <assert.h>

int main() {
  int p1, p2, p3, x;
  klee_make_symbolic(&p1, sizeof(p1), "p1");
  klee_make_symbolic(&p2, sizeof(p2), "p2");
  klee_make_symbolic(&p3, sizeof(p3), "p3");
  klee_make_symbolic(&x, sizeof(x), "x");

  if (x <= 0) {
    if (p1 > 8)
      x = x + 1;
    else
      x = x + 2;
    if (p2 > 8)
      x = x + 1;
    else
      x = x + 2;
    if (p3 > 8)
      x = x + 1;
    else
      x = x + 2;

    assert(x <= 6);
  }

  return 0;
}
