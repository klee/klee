// RUN: %clang %s -emit-llvm %O0opt -c -o %t.bc
// RUN: rm -f %t.log
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --max-instructions=100 --libc=none %t.bc 2> %t.log
// RUN: cat %t.log | FileCheck %s

// This will only finish within 100 instructions if the
// SpecialFunctionHandler handles the mem.* calls
// CHECK-NOT: halting execution, dumping remaining states

#include <string.h>

#define SIZE 4000
int main() {
  char a[SIZE];
  char b[SIZE];
  memset(a, 0, 3000);
  memmove(a + 1000, a, 3000);
  memcpy(b, a, SIZE);
  return 0;
}
