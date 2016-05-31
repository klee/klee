// RUN: %llvmgcc -emit-llvm -c -g -o %t1.bc %s
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --zest --use-symbex=1 --use-zest-search %t1.bc z
// RUN: ls %t.klee-out/ | grep .ptr.err | wc -l | grep 1
// RUN: rm -rf %t.klee-out-2
// RUN: %klee --output-dir=%t.klee-out-2 --zest --use-symbex=1 --use-zest-search %t1.bc b
// RUN: ls %t.klee-out-2/ | grep .ptr.err | wc -l | grep 1

#include <stdio.h>

int main(int argc, char **argv) {
  int a[10] = {0};

  char opt = argv[1][0];

  int index = 7;

  klee_make_symbolic(&index, sizeof(index), "index");
  klee_assume(index == 7);

  if (opt == 'c')
    index += 3;

  if (opt == 'b')
    index += 2;

  if (opt == 'a')
    index++;

  return a[index];
}
