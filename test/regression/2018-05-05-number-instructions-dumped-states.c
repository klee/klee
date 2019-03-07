// RUN: %clang %s -emit-llvm %O0opt -g -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee -max-instructions=1 --output-dir=%t.klee-out %t1.bc 2>&1 | FileCheck %s

// CHECK: KLEE: done: total instructions = 1

#include "klee/klee.h"

int main(int argc, char * argv[]) {}
