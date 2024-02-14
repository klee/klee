// RUN: %clang -emit-llvm -c -g %s -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --write-bin-tests %t.bc

// RUN: test -f %t.klee-out/test000001.ktest.buffer
// RUN: %ktest-tool %t.klee-out/test000001.ktest | grep hex | awk '{print $NF}' | tr -d '\n' &> %t.klee-out/oracle1.bin
// RUN  diff %t.klee-out/oracle1.bin %t.klee-out/test000001.ktest.buffer

// RUN: test -f %t.klee-out/test000002.ktest.buffer
// RUN: %ktest-tool %t.klee-out/test000002.ktest | grep hex | awk '{print $NF}' | tr -d '\n' &> %t.klee-out/oracle2.bin
// RUN  diff %t.klee-out/oracle2.bin %t.klee-out/test000002.ktest.buffer

// RUN: test -f %t.klee-out/test000003.ktest.buffer
// RUN: %ktest-tool %t.klee-out/test000003.ktest | grep hex | awk '{print $NF}' | tr -d '\n' &> %t.klee-out/oracle3.bin
// RUN  diff %t.klee-out/oracle3.bin %t.klee-out/test000003.ktest.buffer

// RUN: test -f %t.klee-out/test000004.ktest.buffer
// RUN: %ktest-tool %t.klee-out/test000004.ktest | grep hex | awk '{print $NF}' | tr -d '\n' &> %t.klee-out/oracle4.bin
// RUN  diff %t.klee-out/oracle4.bin %t.klee-out/test000004.ktest.buffer

int main(int argc, char *argv[]) {

  int x;
  klee_make_symbolic(&x, sizeof(x), "x");

  int y;
  klee_make_symbolic(&y, sizeof(y), "y");

  if (x > 0) {
    if (y > 0)
      return 1;
    else
      return 2;
  } else {
    if (y > 0)
      return 3;
    else
      return 4;
  }

  return 0;
}
