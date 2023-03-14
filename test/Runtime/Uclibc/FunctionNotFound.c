// RUN: %clang %s -emit-llvm %O0opt -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: not %klee --entry-point=main --output-dir=%t.klee-out --libc=uclibc --exit-on-error %t1.bc 2>&1 | FileCheck %s

int foo(int argc, char *argv[]) {
  return 0;
}

// CHECK: KLEE: ERROR: Entry function 'main' not found in module.
