// REQUIRES: posix-runtime
// RUN: %clang %s -emit-llvm -g %O0opt -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: not %klee --output-dir=%t.klee-out %t.bc >%t1.log 2>&1
// RUN: FileCheck -input-file=%t1.log -check-prefix=CHECK-MAIN-NOT-FOUND %s
// RUN: rm -rf %t.klee-out
// RUN: rm -rf %t1.log
// RUN: not %klee --output-dir=%t.klee-out --posix-runtime %t.bc >%t1.log 2>&1
// RUN: FileCheck -input-file=%t1.log -check-prefix=CHECK-MAIN-NOT-FOUND %s

// CHECK-MAIN-NOT-FOUND: Entry function 'main' not found in module.
