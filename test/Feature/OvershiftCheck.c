// RUN: %llvmgcc %s -emit-llvm -g -O0 -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out -check-overshift %t.bc 2> %t.log
// RUN: grep -c "overshift error" %t.log
// RUN: grep -c "OvershiftCheck.c:20: overshift error" %t.log
// RUN: grep -c "OvershiftCheck.c:24: overshift error" %t.log

/* This test checks that two consecutive potential overshifts
 * are reported as errors.
 */
int main()
{
  unsigned int x=15;
  unsigned int y;
  unsigned int z;
  volatile unsigned int result;

  /* Overshift if y>= sizeof(x) */
  klee_make_symbolic(&y,sizeof(y),"shift_amount1");
  result = x << y;

  /* Overshift is z>= sizeof(x) */
  klee_make_symbolic(&z,sizeof(z),"shift_amount2");
  result = x >> z;

  return 0;
}
