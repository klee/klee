// RUN: %llvmgcc -emit-llvm -g -c -o %t1.bc %s
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out -max-time=2 --exit-on-error --output-level=error --simple-constraint-management --libc=uclibc --posix-runtime --zest --use-query-log=solver:pc %t1.bc 1
// RUN: grep -v fd_init %t.klee-out/solver-queries.pc >%t2.txt
// RUN: grep -c location %t2.txt >%t3.txt
// RUN: grep 1 %t3.txt

int main(int argc, char **argv) {

  int x = (int)argv[1][0];
  // int x = 1;
  int i;
  if (x == '1') {
    i = argv[x - '1'][0];
  }

  for (i = 0; i < 3000; i++)
    if (x > 10)
      x += i;

  return 0;
}
