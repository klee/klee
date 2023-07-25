// REQUIRES: geq-llvm-11.0
// REQUIRES: not-darwin
// RUN: %clang %s -emit-llvm -g %O0opt -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --mock-policy=all %t.bc

// RUN: %llc %t.bc -filetype=obj -o %t.o
// RUN: %llc %t.klee-out/externals.ll -filetype=obj -o %t_externals.o
// RUN: %objcopy --redefine-syms %t.klee-out/redefinitions.txt %t.o
// RUN: %cc -no-pie %t_externals.o %t.o %libkleeruntest -Wl,-rpath %libkleeruntestdir -o %t_runner
// RUN: test -f %t.klee-out/test000001.ktest
// RUN: env KTEST_FILE=%t.klee-out/test000001.ktest %t_runner

// RUN: %runmocks %cc -no-pie %libkleeruntest -Wl,-rpath %libkleeruntestdir -o %t_runner2 %t.klee-out %t.bc
// RUN: test -f %t.klee-out/test000001.ktest
// RUN: env KTEST_FILE=%t.klee-out/test000001.ktest %t_runner2

extern int variable;

extern int foo(int);

int main() {
  int a;
  klee_make_symbolic(&a, sizeof(a), "a");
  a = variable + foo(a);
  return 0;
}
