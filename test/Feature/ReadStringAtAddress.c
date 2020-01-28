extern void klee_warning(const char *);

// RUN: %clang %s -g -emit-llvm %O0opt -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out %t.bc 2>&1 | FileCheck %s

int main(void) {
	char strings[] = {'s', 't', 'r', '1', '\0',
			  's', 't', 'r', '2', '\0',
			  's', 't', 'r', '3', '\0'};

	klee_warning(strings);
	// CHECK: WARNING: main: str1
	klee_warning(strings+5);
	// CHECK: WARNING: main: str2
	klee_warning(strings+2);
	// CHECK: WARNING: main: r1
	klee_warning(strings+10);
	// CHECK: WARNING: main: str3
	klee_warning(strings+6);
	// CHECK: WARNING: main: tr2

	char bytes[] = {'a', 'b', 'c', 'd'};
	// We should not crash, since it reads
	// only up to the size of the object
	klee_warning(bytes);
	// CHECK: WARNING ONCE: String not terminated by \0 passed to one of the klee_ functions
	// CHECK: WARNING: main: abcd

	return 0;
}
