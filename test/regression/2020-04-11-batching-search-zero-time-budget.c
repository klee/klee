// RUN: %clang %s -emit-llvm %O0opt -g -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --use-batching-search --batch-time=0 --batch-instructions=1 %t.bc 2>&1 | FileCheck %s

#include <time.h>

#include "klee/klee.h"

int main(void) {
  for (int i = 0; i < 10; ++i) {
    struct timespec ts = {.tv_sec = 0, .tv_nsec = 10000000}; // 10 ms
    nanosleep(&ts, NULL);
  }
  // CHECK-NOT: increased time budget
  return 0;
}
