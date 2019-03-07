// RUN: %clang %s -emit-llvm -g -c -o %t2.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --exit-on-error --debug-print-instructions=all:stderr %t2.bc 2>%t3.txt
// RUN: FileCheck -input-file=%t3.txt -check-prefix=CHECK-FILE-SRC %s
// RUN: FileCheck -input-file=%t3.txt -check-prefix=CHECK-FILE-DBG %s
// RUN: not test -f %t.klee-out/instructions.txt
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --exit-on-error --debug-print-instructions=all:file %t2.bc
// RUN: FileCheck -input-file=%t.klee-out/instructions.txt -check-prefix=CHECK-FILE-DBG %s
// RUN: FileCheck -input-file=%t.klee-out/instructions.txt -check-prefix=CHECK-FILE-SRC %s
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --exit-on-error --debug-print-instructions=src:file %t2.bc
// RUN: FileCheck -input-file=%t.klee-out/instructions.txt -check-prefix=CHECK-FILE-SRC %s
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --exit-on-error --debug-print-instructions=compact:file %t2.bc
// RUN: FileCheck -input-file=%t.klee-out/instructions.txt -check-prefix=CHECK-FILE-COMPACT %s
//
#include "klee/klee.h"
#include <assert.h>
#include <stdio.h>

char array[5] = { 1, 2, 3, -4, 5 };

int main() {
  unsigned k;

  klee_make_symbolic(&k, sizeof(k), "k");
  klee_assume(k < 5);

  if (array[k] == -4)
    printf("Yes\n");

  // CHECK-FILE-SRC: LoggingInstructions.c:27
  // CHECK-FILE-SRC: LoggingInstructions.c:28
  // CHECK-FILE-SRC: LoggingInstructions.c:30
  // CHECK-FILE-SRC: LoggingInstructions.c:31

  // CHECK-FILE-DBG: call void @klee_make_symbolic
  // CHECK-FILE-DBG: load

  // CHECK-FILE-COMPACT-NOT: LoggingInstructions.c:21
  // CHECK-FILE-COMPACT-NOT: LoggingInstructions.c:22
  // CHECK-FILE-COMPACT-NOT: LoggingInstructions.c:24
  // CHECK-FILE-COMPACT-NOT: LoggingInstructions.c:25

  return 0;
}
