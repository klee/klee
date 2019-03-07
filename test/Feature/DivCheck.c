// Check if div-instructions are correctly instrumented:
// * unoptimized code will contain a call to klee_div_zero_check
// * optimized code will have this check inlined
// In both cases, the `div` instruction should have been marked with meta-data: klee.check.div
//
// RUN: %clang %s -emit-llvm -g -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --check-div-zero=true %t.bc >%t.div_enabled.log
// RUN: FileCheck %s -input-file=%t.klee-out/assembly.ll -check-prefix=DIV-ENABLED
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --check-div-zero=true --optimize %t.bc >%t.div_enabled.log
// RUN: FileCheck %s -input-file=%t.klee-out/assembly.ll -check-prefix=DIV-ENABLED-OPT
// Without debug information
// RUN: %clang %s -emit-llvm -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --check-div-zero=true %t.bc >%t.div_enabled.log
// RUN: FileCheck %s -input-file=%t.klee-out/assembly.ll -check-prefix=DIV-ENABLED
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --check-div-zero=true --optimize %t.bc >%t.div_enabled.log
// RUN: FileCheck %s -input-file=%t.klee-out/assembly.ll -check-prefix=DIV-ENABLED-OPT
#include "klee/klee.h"
#include <stdio.h>

int main(int argc, char **argv) {
  char c;

  klee_make_symbolic(&c, sizeof(c), "index");

  // Validate
  if (argc / c == 5)
    return 1;
  // Check that div has been instrumented once
  // DIV-ENABLED: call {{.*}}void @klee_div_zero_check({{.+}} %{{.+}})
  // DIV-ENABLED-NOT: call {{.*}}void @klee_div_zero_check
  // Check that div has the proper metadata
  // DIV-ENABLED: sdiv {{.*}} !klee.check.div
  // DIV-ENABLED-OPT: sdiv {{.*}} !klee.check.div

  // Validate
  if (argc / 5 == 5)
    return 1;
  // Check that div has not been instrumented
  // DIV-ENABLED-NOT: call {{.*}}void @klee_div_zero_check(i64 5)
  // DIV-ENABLED-NOT: sdiv {{.*}} !klee.check.div
  // DIV-ENABLED-OPT-NOT: sdiv {{.*}} !klee.check.div

  return 0;
}
