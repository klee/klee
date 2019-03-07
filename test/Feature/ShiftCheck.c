// Check if shift-instructions are correctly instrumented:
// * unoptimized code will contain a call to klee_overshift_check
// * optimized code will have this check inlined
// In both cases, the `ashr` instruction should have been marked with meta-data: klee.check.shift
//
// RUN: %clang %s -emit-llvm -g -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --check-overshift=true %t.bc >%t.shift_enabled.log
// RUN: FileCheck %s -input-file=%t.klee-out/assembly.ll -check-prefix=SHIFT-ENABLED
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --check-overshift=true --optimize %t.bc >%t.shift_enabled.log
// RUN: FileCheck %s -input-file=%t.klee-out/assembly.ll -check-prefix=SHIFT-ENABLED-OPT
// Same test without debug information
// RUN: %clang %s -emit-llvm -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --check-overshift=true %t.bc >%t.shift_enabled.log
// RUN: FileCheck %s -input-file=%t.klee-out/assembly.ll -check-prefix=SHIFT-ENABLED
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --check-overshift=true --optimize %t.bc >%t.shift_enabled.log
// RUN: FileCheck %s -input-file=%t.klee-out/assembly.ll -check-prefix=SHIFT-ENABLED-OPT

#include "klee/klee.h"
#include <stdio.h>

int main(int argc, char **argv) {
  char c;

  klee_make_symbolic(&c, sizeof(c), "index");

  // Validate
  if (argc >> c == 5)
    return 1;
  // Check for klee_overshift_check call
  // SHIFT-ENABLED: call {{.*}}void @klee_overshift_check
  // Check that double-instrumentation does not happen
  // SHIFT-ENABLED-NOT: call {{.*}}void @klee_overshift_check
  // SHIFT-ENABLED: ashr {{.*}} !klee.check.shift
  // SHIFT-ENABLED-OPT: ashr {{.*}} !klee.check.shift

  // Validate
  uint32_t value = (uint32_t)argc;
  if (value >> 3 == 5)
    return 1;
  // Check that the second shift was not instrumented
  // SHIFT-ENABLED-NOT: call {{.*}}void @klee_overshift_check(i32 i{{.+.+}} 3)
  // SHIFT-ENABLED-NOT: ashr {{.*}} !klee.check.shift
  // SHIFT-ENABLED-OPT-NOT: ashr {{.*}} !klee.check.shift

  return 0;
}
