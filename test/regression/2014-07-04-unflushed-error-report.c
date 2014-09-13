// RUN: %llvmgcc %s -emit-llvm -g -O0 -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out -check-overshift %t.bc 2> %t.log
// RUN: FileCheck -input-file=%t.klee-out/test000001.overshift.err %s

/* This test checks that the error file isn't empty and contains the
 * right content.
 */
int main()
{
  unsigned int x=15;
  unsigned int y;
  unsigned int z;
  volatile unsigned int result;

  /* Overshift if y>= sizeof(x) */
  klee_make_symbolic(&y,sizeof(y),"shift_amount1");
  // CHECK: Error: overshift error
  // CHECK-NEXT: 2014-07-04-unflushed-error-report.c
  // FIXME: Need newer FileCheck for to do ``Line: [[@LINE+1]]``
  // Just hardcode line number for now
  // CHECK-NEXT: Line: 23
  result = x << y;

  return 0;
}
