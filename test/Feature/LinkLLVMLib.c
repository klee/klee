// RUN: %clang %s -g -emit-llvm %O0opt -c -o %t1.bc -DLINK_LLVM_LIB_TEST_LIB
// RUN: %llvmar r %t1.a %t1.bc
//
// RUN: %clang %s -g -emit-llvm %O0opt -c -o %t2.bc -DLINK_LLVM_LIB_TEST_EXEC
// RUN: rm -rf %t.klee-out
// RUN: %klee --link-llvm-lib %t1.a --output-dir=%t.klee-out --emit-all-errors --warnings-only-to-file=false %t2.bc 2>&1 | FileCheck %s

#ifdef LINK_LLVM_LIB_TEST_EXEC
extern void printint(int d);

int main(int argc, char * argv[]) {
	printint(5);
	// CHECK: KLEE: WARNING ONCE: calling external: printf
	return 0;
}
#endif

#ifdef LINK_LLVM_LIB_TEST_LIB
#include <stdio.h>

void printint(int d) {
	printf("%d\n", d);
}
#endif
