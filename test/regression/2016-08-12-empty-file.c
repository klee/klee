// RUN: %llvmgcc %s -emit-llvm -g -O0 -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: not %klee --output-dir=%t.klee-out %t.bc >%t1.log 2>&1
// RUN: FileCheck -input-file=%t1.log -check-prefix=CHECK-MAIN-NOT-FOUND %s
// RUN: rm -rf %t.klee-out
// RUN: rm -rf %t1.log
// RUN: not %klee --output-dir=%t.klee-out --posix-runtime %t.bc >%t1.log 2>&1
// RUN: FileCheck -input-file=%t1.log -check-prefix=CHECK-MAIN-NOT-FOUND %s

// CHECK-MAIN-NOT-FOUND: 'main' function not found in module.
