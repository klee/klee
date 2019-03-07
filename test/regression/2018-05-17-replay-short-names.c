// RUN: rm -rf a
// RUN: mkdir a
// RUN: touch a/b
// RUN: %clang %s -emit-llvm %O0opt -g -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee -replay-ktest-dir=a --output-dir=%t.klee-out %t1.bc 2>&1
//

#include "klee/klee.h"

int main(int argc, char * argv[]) {}
