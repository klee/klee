// REQUIRES: stp
// RUN: %clang %s -emit-llvm %O0opt -c -o %t1.bc
//
// RUN: rm -rf %t.klee-out-minisat
// RUN: %klee --output-dir=%t.klee-out-minisat -solver-backend=stp --stp-sat-solver=minisat %t1.bc
// RUN: cat %t.klee-out-minisat/messages.txt %t.klee-out-minisat/warnings.txt > %t.klee-out-minisat/all.txt
// RUN: FileCheck --input-file=%t.klee-out-minisat/all.txt --check-prefix=MINISAT %s
//
// RUN: rm -rf %t.klee-out-riss
// RUN: %klee --output-dir=%t.klee-out-riss -solver-backend=stp --stp-sat-solver=riss %t1.bc
// RUN: cat %t.klee-out-riss/messages.txt %t.klee-out-riss/warnings.txt > %t.klee-out-riss/all.txt
// RUN: FileCheck --input-file=%t.klee-out-riss/all.txt --check-prefix=RISS %s

#include "klee/klee.h"

int main(void) {
	int foo;
	int bar = 42;
	klee_make_symbolic(&foo, sizeof(foo), "foo");

	if (foo) bar -= 17;
	else bar += 5;

	return bar;
}

// MINISAT: KLEE: Using STP solver backend
// MINISAT: {{KLEE: SAT solver: MiniSat|KLEE: Fallback SAT solver}}

// RISS: KLEE: Using STP solver backend
// RISS: {{KLEE: SAT solver: RISS|KLEE: Fallback SAT solver}}
