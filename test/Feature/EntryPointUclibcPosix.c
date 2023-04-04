// REQUIRES: uclibc
// REQUIRES: posix-runtime

/* Similar test as EntryPoint.c, but with POSIX and uclibc.  This
   checks that the various wrappers and renames introduced when
   linking the uclibc and the POSIX runtime are correctly handled. */

// RUN: %clang -emit-llvm -g -c %s -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --libc=uclibc --posix-runtime --output-dir=%t.klee-out --entry-point=other_main %t.bc | FileCheck %s

// RUN: rm -rf %t.klee-out
// RUN: %clang -emit-llvm -g -c -DMAIN %s -o %t.bc
// RUN: %klee --libc=uclibc --posix-runtime --output-dir=%t.klee-out --entry-point=other_main %t.bc | FileCheck %s

// RUN: %clang -emit-llvm -g -c %s -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --libc=uclibc --output-dir=%t.klee-out --entry-point=other_main %t.bc | FileCheck %s

// RUN: rm -rf %t.klee-out
// RUN: %clang -emit-llvm -g -c -DMAIN %s -o %t.bc
// RUN: %klee --libc=uclibc  --output-dir=%t.klee-out --entry-point=other_main %t.bc | FileCheck %s

// RUN: %clang -emit-llvm -g -c %s -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --posix-runtime --output-dir=%t.klee-out --entry-point=other_main %t.bc | FileCheck %s

// RUN: rm -rf %t.klee-out
// RUN: %clang -emit-llvm -g -c -DMAIN %s -o %t.bc
// RUN: %klee --posix-runtime --output-dir=%t.klee-out --entry-point=other_main %t.bc | FileCheck %s


#include <stdio.h>

int other_main() {
  printf("Hello World\n");
  // CHECK: Hello World
  return 0;
}

#ifdef MAIN
int main() {
  return 0;
}
#endif
