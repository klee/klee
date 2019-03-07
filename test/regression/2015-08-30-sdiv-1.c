// RUN: %clang %s -emit-llvm -g %O0opt -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out -exit-on-error -solver-optimize-divides=true %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out -exit-on-error -solver-optimize-divides=false %t.bc

/* Division by constant can be optimized.using mul/shift
 * For signed division, div by 1 or -1 cannot be optimized like that.
 */
#include <stdint.h>
int main() {
  int32_t dividend;
  klee_make_symbolic(&dividend, sizeof dividend, "Dividend");
  if ((3 ^ (dividend & 2)) / 1)
    return 1;
  if ((3 ^ (dividend & 2)) / -1)
    return 1;
  return 0;
}
