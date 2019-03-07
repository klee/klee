// RUN: %clang -emit-llvm -c -g %O0opt %s -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out -check-div-zero -emit-all-errors=0 %t.bc 2> %t.log
// RUN: FileCheck --input-file=%t.log %s

/* This test case captures a bug where two distinct division
*  by zero errors are treated as the same error and so
*  only one test case is generated EVEN IF THERE ARE MULTIPLE 
*  DISTINCT ERRORS!
*/
int main() {
  unsigned int a = 15;
  unsigned int b = 15;
  volatile unsigned int d1;
  volatile unsigned int d2;

  klee_make_symbolic(&d1, sizeof(d1), "divisor1");
  klee_make_symbolic(&d2, sizeof(d2), "divisor2");

  // deliberate division by zero possible
  // CHECK: consecutive_divide_by_zero.c:[[@LINE+1]]: divide by zero
  unsigned int result1 = a / d1;

  // another deliberate division by zero possible
  // CHECK: consecutive_divide_by_zero.c:[[@LINE+1]]: divide by zero
  unsigned int result2 = b / d2;

  // CHECK: completed paths = 3
  // CHECK: generated tests = 3
  return 0;
}
