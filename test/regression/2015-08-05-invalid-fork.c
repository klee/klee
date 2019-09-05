/* Reported by @kren1 in #262 
   The test makes sure that the string "Should be printed once" 
   is printed a single time. 
*/
#include "klee/klee.h"

// RUN: %clang %s -emit-llvm -g %O0opt -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out %t.bc | FileCheck %s

static int g_10 = 0x923607A9L;

int main(int argc, char* argv[]) {
  klee_make_symbolic(&g_10,sizeof(g_10), "g_10");
  if (g_10 < (int)0x923607A9L) klee_silent_exit(0);
  if (g_10 > (int)0x923607A9L) klee_silent_exit(0);

  int b = 2;
  int i = g_10 % (1 % g_10);
  i || b;
  printf("Should be printed once\n");
  // CHECK: Should be printed once
  // CHECK-NOT: Should be printed once
  return 0;
}
