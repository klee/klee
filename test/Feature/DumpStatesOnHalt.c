// RUN: %clang %s -g -emit-llvm %O0opt -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --max-instructions=1 --dump-states-on-halt=true %t1.bc 2>&1 | FileCheck %s
// RUN: test -f %t.klee-out/test000001.ktest

int main(int argc, char** argv) {
  int x = 1;
  if (argc == 1)
    x = 0;
  return x;
}
// CHECK: halting execution, dumping remaining states
