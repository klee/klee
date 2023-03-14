// REQUIRES: posix-runtime
// REQUIRES: uclibc

// RUN: %clang -emit-llvm -g -c %s -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: not %klee --output-dir=%t.klee-out --entry-point=missing %t.bc 2>&1 | FileCheck -check-prefix=CHECK-MISSING %s

// RUN: %clang -emit-llvm -g -c %s -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: not %klee --output-dir=%t.klee-out --entry-point=missing --libc=uclibc %t.bc 2>&1 | FileCheck -check-prefix=CHECK-MISSING %s

// RUN: %clang -emit-llvm -g -c %s -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: not %klee --output-dir=%t.klee-out --entry-point=missing %t.bc --posix-runtime 2>&1 | FileCheck -check-prefix=CHECK-MISSING %s

// RUN: %clang -emit-llvm -g -c %s -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: not %klee --output-dir=%t.klee-out --entry-point=missing %t.bc --libc=uclibc --posix-runtime 2>&1 | FileCheck -check-prefix=CHECK-MISSING %s

// RUN: %clang -emit-llvm -g -c %s -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: not %klee --output-dir=%t.klee-out --entry-point=missing --libc=klee %t.bc 2>&1 | FileCheck -check-prefix=CHECK-MISSING %s

// RUN: %clang -emit-llvm -g -c %s -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: not %klee --output-dir=%t.klee-out --entry-point=missing --libc=klee --posix-runtime %t.bc 2>&1 | FileCheck -check-prefix=CHECK-MISSING %s


/* Missing main */

// RUN: %clang -emit-llvm -g -c %s -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: not %klee --output-dir=%t.klee-out --entry-point=main %t.bc 2>&1 | FileCheck -check-prefix=CHECK-MAIN %s

// RUN: %clang -emit-llvm -g -c %s -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: not %klee --output-dir=%t.klee-out --entry-point=main --libc=uclibc %t.bc 2>&1 | FileCheck -check-prefix=CHECK-MAIN %s

// RUN: %clang -emit-llvm -g -c %s -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: not %klee --output-dir=%t.klee-out --entry-point=main --posix-runtime %t.bc 2>&1 | FileCheck -check-prefix=CHECK-MAIN %s

// RUN: %clang -emit-llvm -g -c %s -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: not %klee --output-dir=%t.klee-out --entry-point=main --libc=uclibc --posix-runtime %t.bc 2>&1 | FileCheck -check-prefix=CHECK-MAIN %s

// RUN: %clang -emit-llvm -g -c %s -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: not %klee --output-dir=%t.klee-out --entry-point=main --libc=klee %t.bc 2>&1 | FileCheck -check-prefix=CHECK-MAIN %s

// RUN: %clang -emit-llvm -g -c %s -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: not %klee --output-dir=%t.klee-out --entry-point=main --libc=klee --posix-runtime %t.bc 2>&1 | FileCheck -check-prefix=CHECK-MAIN %s

#include <stdio.h>

int hello() {
  printf("Hello World\n");
  return 0;
}

// CHECK-MISSING: Entry function 'missing' not found in module
// CHECK-MAIN: Entry function 'main' not found in module
