// RUN: %clang %s -emit-llvm -O0 -g -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --libc=klee --output-dir=%t.klee-out --exit-on-error %t1.bc > %t-output.txt 2>&1
// RUN: FileCheck -input-file=%t-output.txt %s
// REQUIRES: x86_64
#include "klee/klee.h"
#include <assert.h>
#include <inttypes.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

long double globalLongDouble[4] = {INFINITY, NAN, 1.0l, -1.0l};

void dump_long_double(long double ld) {
  uint64_t data[2] = {0, 0};
  memcpy(data, &ld, 10);
  printf("a: 0x%.4" PRIx16 " %.16" PRIx64 "\n", (uint16_t)data[1], data[0]);
}

int main() {
  long double local[4];
  assert((((size_t)local) % 16) == 0); // 16 byte aligned
  memcpy(local, &globalLongDouble, sizeof(globalLongDouble));
  assert(isinf(local[0]));
  assert(isnan(local[1]));
  assert(local[2] == 1.0l);
  assert(local[3] == -1.0l);
}
// CHECK: KLEE: done: generated tests = 1
