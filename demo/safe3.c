/*
 * Copyright 2016 National University of Singapore
 */
#include <klee/klee.h>
#include <assert.h>

int main() {
  int p1, p2, p3, x, y;
  klee_make_symbolic(&p1, sizeof(p1), "p1");
  klee_make_symbolic(&p2, sizeof(p2), "p2");
  klee_make_symbolic(&p3, sizeof(p3), "p3");
  klee_make_symbolic(&x, sizeof(x), "x");

  if (x <= 0) {
    y = x;
    if (p1 > 8)
      x = x + 1;
    else
      x = y + 1;

    y = x;
    if (p2 > 8)
      x = x + 2;
    else
      x = y + 2;

    y = x;
    if (p3 > 8)
      x = x + 3;
    else
      x = y + 3;

    assert(x <= 4);
  }

  return 0;
}
