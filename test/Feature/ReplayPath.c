// RUN: echo "1" > %t1.path
// RUN: echo "0" >> %t1.path
// RUN: echo "1" >> %t1.path
// RUN: echo "0" >> %t1.path
// RUN: echo "1" >> %t1.path
// RUN: %llvmgcc %s -emit-llvm -O0 -c -o %t2.bc
// RUN: %klee --replay-path %t1.path %t2.bc > %t3.log
// RUN: echo "res: 110" > %t3.good
// RUN: diff %t3.log %t3.good

int main() {
  int res = 1;
  int x;

  klee_make_symbolic(&x, sizeof x);

  if (x&1) res *= 2;
  if (x&2) res *= 3;
  if (x&4) res *= 5;

  // get forced branch coverage
  if (x&2) res *= 7;
  if (!(x&2)) res *= 11;
  printf("res: %d\n", res);
 
  return 0;
}
