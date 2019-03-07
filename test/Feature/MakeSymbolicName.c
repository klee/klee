// RUN: %clang %s -emit-llvm -g -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --search=random-state --exit-on-error %t1.bc

#include <assert.h>

int main() {
  int a[4] = {1, 2, 3, 4};
  unsigned i;

  klee_make_symbolic(&i, sizeof(i), "index");  
  if (i > 3)
    klee_silent_exit(0);

  assert(a[i] << 1 != 5);
  if (a[i] << 1 == 6)
    assert(i == 2);
}
