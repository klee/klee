// RUN: %clang %s -emit-llvm -g -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out %t1.bc

#include "klee/klee.h"

int main(int argc, char * argv[]) {
	int a = klee_int((void*)0);
}
