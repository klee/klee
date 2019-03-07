// RUN: %clang -emit-llvm -g -c %s -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out %t.bc > %t.log 2> %t.stderr.log
// RUN: FileCheck %s -check-prefix=CHECK-MSG --input-file=%t.log
// RUN: FileCheck %s -check-prefix=CHECK-ERR --input-file=%t.stderr.log

#include <stdio.h>

int main(void) {
	void * lptr = &&one;
	lptr = &&two;
	lptr = &&three;

	klee_make_symbolic(&lptr, sizeof(lptr), "lptr");
// CHECK-ERR: KLEE: ERROR: {{.*}} indirectbr: illegal label address
	goto *lptr;

one:
	puts("Label one");
// CHECK-MSG-DAG: Label one
	return 0;

two:
	puts("Label two");
// CHECK-MSG-DAG: Label two
	return 0;

three:
	puts("Label three");
// CHECK-MSG-DAG: Label three
	return 0;

// LLVM should infer only {one, two, three} as valid targets
four:
	puts("Label four");
// CHECK-MSG-NOT: Label four
	return 0;
}
