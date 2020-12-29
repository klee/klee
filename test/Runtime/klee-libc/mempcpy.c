// validate mempcpy implementation
// RUN: %clang %s -emit-llvm %O0opt -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --libc=klee --exit-on-error %t.bc

#include "klee/klee.h"

// do not #define _GNU_SOURCE - just declare mempcpy for all systems

#include <assert.h>
#include <string.h>

void *mempcpy(void *destaddr, void const *srcaddr, size_t len);

int main() {
  // char arrays
  char s1[] = "foofoo";
  char s2[] = "barbar";
  char *spos = (char *)mempcpy(s2, s1, 3);

  assert(spos == &s2[3]);
  assert(!strcmp(s2, "foobar"));
  assert(!strcmp(s1, "foofoo"));

  // int arrays
  int arr1[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
  int arr2[] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1};
  int *pos = (int *)mempcpy(arr2, arr1, sizeof(int) * 5);

  assert(pos == &arr2[5]);
  for (int i = 0; i < 10; ++i) assert(arr1[i] == i);
  for (int i = 0; i <  5; ++i) assert(arr2[i] == i);
  for (int i = 5; i < 10; ++i) assert(arr2[i] == -1);

  // symbolic int arrays
  int sarr1[10];
  int sarr2[10];
  klee_make_symbolic(sarr1, sizeof(int) * 10, "sarr1");
  klee_make_symbolic(sarr2, sizeof(int) * 10, "sarr2");
  pos = (int *)mempcpy(sarr2, sarr1, sizeof(int) * 5);

  assert(pos == &sarr2[5]);
  assert(!memcmp(sarr2, sarr1, sizeof(int) * 5));
}
