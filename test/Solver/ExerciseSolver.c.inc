#include "klee/klee.h"

#include <stddef.h>
#include <stdio.h>

// just play around with a few symbolic values to exercise the solver a bit
int main(int argc, char **argv) {
  unsigned arr[4];
  klee_make_symbolic(arr, sizeof(arr), "arr");

  arr[0] ^= arr[1];
  arr[2] += arr[3];
  arr[3] %= 8191;

  {
    size_t i;
    klee_make_symbolic(&i, sizeof(i), "i");
    if (i < sizeof(arr) / sizeof(*arr)) {
      arr[i] = 0;
    } else {
      arr[0] = 0;
    }
  }

  if (arr[0] + 8192 == arr[1]) {
    arr[3] *= 2;
  }

  unsigned minimum = arr[0];
  for (size_t i = 1; i < sizeof(arr) / sizeof(*arr); ++i) {
    if (arr[i] < minimum) {
      minimum = arr[i];
    }
  }
}
