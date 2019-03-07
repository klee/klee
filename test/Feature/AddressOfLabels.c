// RUN: %clang -emit-llvm -g -c %s -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out %t.bc > %t.log
// RUN: FileCheck --input-file=%t.log %s

#include <stdio.h>

int main(int argc, char * argv[]) {
	// prevent CFGSimplificationPass optimisation
	void * dummy = &&one;
	switch(argc) {
		case 1: break;
		case 2:
			dummy = &&three;
			goto *dummy;
		default:
			goto *dummy;
	}

	// the real test
	void * lptr = &&two;
	goto *lptr;

one:
	puts("Label one");
// CHECK-NOT: Label one
	return 1;

two:
	puts("Label two");
// CHECK: Label two
	return 0;

three:
	puts("Label three");
// CHECK-NOT: Label three
	return 1;
}
