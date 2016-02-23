// RUN: %llvmgcc %s -g -emit-llvm -O0 -c -o %t2.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --link-libraries %S/Inputs/libprintint.a --output-dir=%t.klee-out --emit-all-errors %t2.bc 2>&1 | FileCheck %s
extern void printint(int d);

int main(int argc, char * argv[]) {
	printint(5);
	// CHECK: KLEE: WARNING ONCE: calling external: printf
	return 0;
}

/*
// library libprintint.a was generated from the following code:
#include <stdio.h>

void printint(int d) {
	printf("%d\n", d);
}
// And commands:
//   clang -emit-llvm -c -o printint.bc printint.c
//   llvm-ar r libprintint.a printint.bc
//   ar s --plugin lib/LLVMgold.so libprintint.a
*/
