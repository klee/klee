// RUN: %llvmgcc -emit-llvm -c -g -o %t1.bc %s
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --zest --output-level=error --emit-all-errors --patch-check-before=1 %t1.bc aa
// RUN: ls %t.klee-out/ | grep .ptr.err | wc -l | grep 2
// RUN: rm -rf %t.klee-out-2
// RUN: %klee --output-dir=%t.klee-out-2 --zest --output-level=error --emit-all-errors --patch-check-before=1 %t1.bc yy
// RUN: ls %t.klee-out-2/ | grep .ptr.err | wc -l | grep 6
// corresponding to x?, x1, b?, a?, a1, a0

int main(int argc, char **argv) {
  int a[10] = {0};

  char opt = argv[1][0];
  char opt2 = argv[1][1];

  int index = 9;

  if (opt == 'a')
    index += 3;
  else {
    klee_patch_begin();
    if (opt == 'b')
      a[10] = 0;
    klee_patch_end(1);
  }

  if (opt2 == '0')
    a[index] = 0;

  if (opt2 == '1') {
    index += 1;
  }

  klee_patch_begin();
  switch (opt) {
  case 'x':
    index += 10;
    break;
  default:
    index -= 1;
  }
  klee_patch_end(1);
  return a[index];
}
