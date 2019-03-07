// RUN: %clang -emit-llvm -g -c -o %t.bc %s
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --exit-on-error --silent-klee-assume %t.bc > %t.silent-klee-assume.log 2>&1
// RUN: FileCheck -input-file=%t.silent-klee-assume.log -check-prefix=CHECK-SILENT-KLEE-ASSUME %s
// RUN: rm -rf %t.klee-out
// RUN: not %klee --output-dir=%t.klee-out --exit-on-error %t.bc > %t.default-klee-assume.log 2>&1
// RUN: FileCheck -input-file=%t.default-klee-assume.log -check-prefix=CHECK-DEFAULT-KLEE-ASSUME %s

#include <stdio.h>
#include <assert.h>

int main() {
  int x = klee_int("x");
  int y = klee_int("y");
  klee_assume(x > 10);
  if (y < 42) {
    // CHECK-DEFAULT-KLEE-ASSUME: KLEE: ERROR: {{.*SilentKleeAssume.c:18}}: invalid klee_assume call (provably false)
    klee_assume(x < 10);
    // CHECK-DEFAULT-KLEE-ASSUME: KLEE: NOTE: now ignoring this error at this location
    // CHECK-DEFAULT-KLEE-ASSUME: EXITING ON ERROR:
    // CHECK-DEFAULT-KLEE-ASSUME: Error: invalid klee_assume call (provably false)
    assert(0);
  }
  return 0;
}

// CHECK-SILENT-KLEE-ASSUME: KLEE: output directory is "{{.+}}"
// CHECK-SILENT-KLEE-ASSUME: KLEE: done: total instructions = {{[0-9]+}}
// CHECK-SILENT-KLEE-ASSUME: KLEE: done: completed paths = 2
// CHECK-SILENT-KLEE-ASSUME: KLEE: done: generated tests = 1
