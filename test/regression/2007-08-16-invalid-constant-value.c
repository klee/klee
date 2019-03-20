// RUN: rm -f %t4.out %t4.err %t4.log
// RUN: %clang %s -emit-llvm -O2 -c -o %t1.bc
// RUN: llvm-as -f %p/../Feature/_utils._ll -o %t2.bc
// RUN: llvm-link %t1.bc %t2.bc -o %t3.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out %t3.bc

#include <assert.h>

#include "../Feature/utils.h"

int main() {
  unsigned char a;

  klee_make_symbolic(&a, sizeof a, "a");

  // demand was firing here because an invalid constant
  // value was being created when implied value did not
  // subtract using the proper type (so overflowed into
  // invalid bits)
  if (util_make_concat2(a + 0xCD, 0xCD) == 0xABCD) {
    assert(!klee_is_symbolic(a));
    printf("add constant case: %d\n", a);
  }

  if (util_make_concat2(0x0B - a, 0xCD) == 0xABCD) {
    assert(!klee_is_symbolic(a));
    printf("sub constant case: %d\n", a);
  }

  return 0;
}
