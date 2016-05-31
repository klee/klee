// RUN: %llvmgcc -emit-llvm -c -g -o %t1.bc %s
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --zest --output-level=error --emit-all-errors --patch-check-before=1 %t1.bc xb
// RUN: ls %t.klee-out/ | grep .ptr.err | wc -l | grep 2

#include <assert.h>

int main(int argc, char **argv) {
  int a[10] = {0};

  char opt = argv[1][0];
  char opt2 = argv[1][1];

  int index = 9;

  if (opt == 'a')
    index += 0;
  else {
    klee_patch_begin();
    if (opt2 == 'x')
      index += 3;
    if (opt2 == 'y')
      index += 9;
    // patch ends here but continue to check until the next symbolic fork
    klee_patch_end(1);
  }

  return a[index];
}
