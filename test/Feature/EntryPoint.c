// RUN: %clang -emit-llvm -g -c %s -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --entry-point=other_main %t.bc | FileCheck -check-prefix=CHECK-OTHER_MAIN %s

// RUN: rm -rf %t.klee-out
// RUN: %clang -emit-llvm -g -c -DMAIN %s -o %t.bc
// RUN: %klee --output-dir=%t.klee-out --entry-point=other_main %t.bc | FileCheck -check-prefix=CHECK-OTHER_MAIN %s

// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --entry-point="" %t.bc 2>%t.stderr.log || echo "Exit status must be 0"
// RUN: FileCheck -check-prefix=CHECK-EMPTY --input-file=%t.stderr.log %s

#include <stdio.h>

int other_main() {
  printf("Hello World\n");
  // CHECK-OTHER_MAIN: Hello World
  return 0;
}

#ifdef MAIN
int main() {
  return 0;
}
#endif

// CHECK-EMPTY: KLEE: ERROR: entry-point cannot be empty
