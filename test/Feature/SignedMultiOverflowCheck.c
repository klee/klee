/*
 * These tests ensure that KLEE correctly identifies
 * overflow conditions in the following tests that
 * utilize the addition, subtraction and multiplication
 * operations.
 */

// RUN: %clang %s -DTEST_CASE=1  -emit-llvm -g %O0opt -c -o %t.case1.bc
// RUN: rm -rf %t.case1.klee-out
// RUN: %klee --output-dir=%t.case1.klee-out --check-signed-overflow %t.case1.bc 2> %t.case1.log
// RUN: FileCheck %s --check-prefix=CASE1 --input-file=%t.case1.log

// RUN: %clang %s -DTEST_CASE=2  -emit-llvm -g %O0opt -c -o %t.case2.bc
// RUN: rm -rf %t.case2.klee-out
// RUN: %klee --output-dir=%t.case2.klee-out --check-signed-overflow %t.case2.bc 2> %t.case2.log
// RUN: FileCheck %s --check-prefix=CASE2 --input-file=%t.case2.log

// RUN: %clang %s -DTEST_CASE=3  -emit-llvm -g %O0opt -c -o %t.case3.bc
// RUN: rm -rf %t.case3.klee-out
// RUN: %klee --output-dir=%t.case3.klee-out --check-signed-overflow %t.case3.bc 2> %t.case3.log
// RUN: FileCheck %s --check-prefix=CASE3 --input-file=%t.case3.log

// RUN: %clang %s -DTEST_CASE=4  -emit-llvm -g %O0opt -c -o %t.case4.bc
// RUN: rm -rf %t.case4.klee-out
// RUN: %klee --output-dir=%t.case4.klee-out --check-signed-overflow %t.case4.bc 2> %t.case4.log
// RUN: FileCheck %s --check-prefix=CASE4 --input-file=%t.case4.log

// RUN: %clang %s -DTEST_CASE=5  -emit-llvm -g %O0opt -c -o %t.case5.bc
// RUN: rm -rf %t.case5.klee-out
// RUN: %klee --output-dir=%t.case5.klee-out --check-signed-overflow %t.case5.bc 2> %t.case5.log
// RUN: FileCheck %s --check-prefix=CASE5 --input-file=%t.case5.log

// RUN: %clang %s -DTEST_CASE=6  -emit-llvm -g %O0opt -c -o %t.case6.bc
// RUN: rm -rf %t.case6.klee-out
// RUN: %klee --output-dir=%t.case6.klee-out --check-signed-overflow %t.case6.bc 2> %t.case6.log
// RUN: FileCheck %s --check-prefix=CASE6 --input-file=%t.case6.log

// RUN: %clang %s -DTEST_CASE=7  -emit-llvm -g %O0opt -c -o %t.case7.bc
// RUN: rm -rf %t.case7.klee-out
// RUN: %klee --output-dir=%t.case7.klee-out --check-signed-overflow %t.case7.bc 2> %t.case7.log
// RUN: FileCheck %s --check-prefix=CASE7 --input-file=%t.case7.log

// RUN: %clang %s -DTEST_CASE=8  -emit-llvm -g %O0opt -c -o %t.case8.bc
// RUN: rm -rf %t.case8.klee-out
// RUN: %klee --output-dir=%t.case8.klee-out --check-signed-overflow %t.case8.bc 2> %t.case8.log
// RUN: FileCheck %s --check-prefix=CASE8 --input-file=%t.case8.log

// RUN: %clang %s -DTEST_CASE=9  -emit-llvm -g %O0opt -c -o %t.case9.bc
// RUN: rm -rf %t.case9.klee-out
// RUN: %klee --output-dir=%t.case9.klee-out --check-signed-overflow %t.case9.bc 2> %t.case9.log
// RUN: FileCheck %s --check-prefix=CASE9 --input-file=%t.case9.log

// RUN: %clang %s -DTEST_CASE=10 -emit-llvm -g %O0opt -c -o %t.case10.bc
// RUN: rm -rf %t.case10.klee-out
// RUN: %klee --output-dir=%t.case10.klee-out --check-signed-overflow %t.case10.bc 2> %t.case10.log
// RUN: FileCheck %s --check-prefix=CASE10 --input-file=%t.case10.log

// RUN: %clang %s -DTEST_CASE=11 -emit-llvm -g %O0opt -c -o %t.case11.bc
// RUN: rm -rf %t.case11.klee-out
// RUN: %klee --output-dir=%t.case11.klee-out --check-signed-overflow %t.case11.bc 2> %t.case11.log
// RUN: FileCheck %s --check-prefix=CASE11 --input-file=%t.case11.log

// RUN: %clang %s -DTEST_CASE=12 -emit-llvm -g %O0opt -c -o %t.case12.bc
// RUN: rm -rf %t.case12.klee-out
// RUN: %klee --output-dir=%t.case12.klee-out --check-signed-overflow %t.case12.bc 2> %t.case12.log
// RUN: FileCheck %s --check-prefix=CASE12 --input-file=%t.case12.log

// RUN: %clang %s -DTEST_CASE=13 -emit-llvm -g %O0opt -c -o %t.case13.bc
// RUN: rm -rf %t.case13.klee-out
// RUN: %klee --output-dir=%t.case13.klee-out --check-signed-overflow %t.case13.bc 2> %t.case13.log
// RUN: FileCheck %s --check-prefix=CASE13 --input-file=%t.case13.log

// RUN: %clang %s -DTEST_CASE=14 -emit-llvm -g %O0opt -c -o %t.case14.bc
// RUN: rm -rf %t.case14.klee-out
// RUN: %klee --output-dir=%t.case14.klee-out --check-signed-overflow %t.case14.bc 2> %t.case14.log
// RUN: FileCheck %s --check-prefix=CASE14 --input-file=%t.case14.log

// RUN: %clang %s -DTEST_CASE=15 -emit-llvm -g %O0opt -c -o %t.case15.bc
// RUN: rm -rf %t.case15.klee-out
// RUN: %klee --output-dir=%t.case15.klee-out --check-signed-overflow %t.case15.bc 2> %t.case15.log
// RUN: FileCheck %s --check-prefix=CASE15 --input-file=%t.case15.log

// RUN: %clang %s -DTEST_CASE=16 -emit-llvm -g %O0opt -c -o %t.case16.bc
// RUN: rm -rf %t.case16.klee-out
// RUN: %klee --output-dir=%t.case16.klee-out --check-signed-overflow %t.case16.bc 2> %t.case16.log
// RUN: FileCheck %s --check-prefix=CASE16 --input-file=%t.case16.log

// RUN: %clang %s -DTEST_CASE=17 -emit-llvm -g %O0opt -c -o %t.case17.bc
// RUN: rm -rf %t.case17.klee-out
// RUN: %klee --output-dir=%t.case17.klee-out --check-signed-overflow %t.case17.bc 2> %t.case17.log
// RUN: FileCheck %s --check-prefix=CASE17 --input-file=%t.case17.log

// RUN: %clang %s -DTEST_CASE=18 -emit-llvm -g %O0opt -c -o %t.case18.bc
// RUN: rm -rf %t.case18.klee-out
// RUN: %klee --output-dir=%t.case18.klee-out --check-signed-overflow %t.case18.bc 2> %t.case18.log
// RUN: FileCheck %s --check-prefix=CASE18 --input-file=%t.case18.log

#include "klee/klee.h"
#include <limits.h>

#define CONSTTESTNUM 49
#define CONSTTESTOTHER 7
#define CONSTTESTFACTOR 2

int main(void) {
#if TEST_CASE == 1
  /* Addition: deliberate concrete overflow (int) */
  volatile int test1x, test1y;
  int result1;
  test1x = INT_MAX;
  test1y = 1;
  // CASE1: SignedMultiOverflowCheck.c:[[@LINE+1]]: signed overflow error (add)
  result1 = test1x + test1y;
  return 0;

#elif TEST_CASE == 2
  /* Addition: deliberate concrete overflow (long long) */
  volatile long long test2x, test2y;
  long long result2;
  test2x = LLONG_MAX;
  test2y = 1;
  // CASE2: SignedMultiOverflowCheck.c:[[@LINE+1]]: signed overflow error (add)
  result2 = test2x + test2y;
  return 0;

#elif TEST_CASE == 3
  /* Addition: symbolic overflow possible (int) */
  volatile int test3x, test3y;
  int result3;
  klee_make_symbolic((void *)&test3x, sizeof(test3x), "test3x");
  klee_make_symbolic((void *)&test3y, sizeof(test3y), "test3y");
  // CASE3: SignedMultiOverflowCheck.c:[[@LINE+1]]: signed overflow error (add)
  result3 = test3x + test3y;
  return 0;

#elif TEST_CASE == 4
  /* Addition: symbolic overflow possible (long long) */
  volatile long long test4x, test4y;
  long long result4;
  klee_make_symbolic((void *)&test4x, sizeof(test4x), "test4x");
  klee_make_symbolic((void *)&test4y, sizeof(test4y), "test4y");
  // CASE4: SignedMultiOverflowCheck.c:[[@LINE+1]]: signed overflow error (add)
  result4 = test4x + test4y;
  return 0;

#elif TEST_CASE == 5
  /* Addition: symbolic conditional test */
  volatile unsigned short cond1, cond2;
  volatile int test5x, test5y;
  int result5;

  klee_make_symbolic((void *)&cond1, sizeof(cond1), "cond1");
  klee_make_symbolic((void *)&cond2, sizeof(cond2), "cond2");
  klee_assume(cond1 < 2);
  klee_assume(cond2 < 2);

  test5x = cond1 ? INT_MAX : CONSTTESTNUM;
  test5y = cond2 ? 1 : CONSTTESTOTHER;
  // CASE5: SignedMultiOverflowCheck.c:[[@LINE+1]]: signed overflow error (add)
  result5 = test5x + test5y;
  return 0;

#elif TEST_CASE == 6
  /* Addition: control case */
  volatile int test6x, test6y;
  int result6;
  test6x = CONSTTESTNUM;
  test6y = CONSTTESTOTHER;
  // CASE6-NOT: SignedMultiOverflowCheck.c:[[@LINE+1]]: signed overflow error
  // CASE6: KLEE: done:
  result6 = test6x + test6y;
  return 0;

#elif TEST_CASE == 7
  /* Subtraction: deliberate concrete overflow (int) */
  volatile int test7x, test7y;
  int result7;
  test7x = INT_MIN;
  test7y = 1;
  // CASE7: SignedMultiOverflowCheck.c:[[@LINE+1]]: signed overflow error (sub)
  result7 = test7x - test7y;
  return 0;

#elif TEST_CASE == 8
  /* Subtraction: deliberate concrete overflow (long long) */
  volatile long long test8x, test8y;
  long long result8;
  test8x = LLONG_MIN;
  test8y = 1;
  // CASE8: SignedMultiOverflowCheck.c:[[@LINE+1]]: signed overflow error (sub)
  result8 = test8x - test8y;
  return 0;

#elif TEST_CASE == 9
  /* Subtraction: symbolic overflow possible (int) */
  volatile int test9x, test9y;
  int result9;
  klee_make_symbolic((void *)&test9x, sizeof(test9x), "test9x");
  klee_make_symbolic((void *)&test9y, sizeof(test9y), "test9y");
  // CASE9: SignedMultiOverflowCheck.c:[[@LINE+1]]: signed overflow error (sub)
  result9 = test9x - test9y;
  return 0;

#elif TEST_CASE == 10
  /* Subtraction: symbolic overflow possible (long long) */
  volatile long long test10x, test10y;
  long long result10;
  klee_make_symbolic((void *)&test10x, sizeof(test10x), "test10x");
  klee_make_symbolic((void *)&test10y, sizeof(test10y), "test10y");
  // CASE10: SignedMultiOverflowCheck.c:[[@LINE+1]]: signed overflow error (sub)
  result10 = test10x - test10y;
  return 0;

#elif TEST_CASE == 11
  /* Subtraction: symbolic conditional test */
  volatile unsigned short cond1, cond2;
  volatile int test11x, test11y;
  int result11;

  klee_make_symbolic((void *)&cond1, sizeof(cond1), "cond1");
  klee_make_symbolic((void *)&cond2, sizeof(cond2), "cond2");
  klee_assume(cond1 < 2);
  klee_assume(cond2 < 2);

  test11x = cond1 ? INT_MIN : CONSTTESTNUM;
  test11y = cond2 ? 1 : CONSTTESTOTHER;
  // CASE11: SignedMultiOverflowCheck.c:[[@LINE+1]]: signed overflow error (sub)
  result11 = test11x - test11y;
  return 0;

#elif TEST_CASE == 12
  /* Subtraction: control case */
  volatile int test12x, test12y;
  int result12;
  test12x = CONSTTESTNUM;
  test12y = CONSTTESTOTHER;
  // CASE12-NOT: SignedMultiOverflowCheck.c:[[@LINE+1]]: signed overflow error
  // CASE12: KLEE: done:
  result12 = test12x - test12y;
  return 0;

#elif TEST_CASE == 13
  /* Multiplication: deliberate concrete overflow (int) */
  volatile int test13x, test13y;
  int result13;
  test13x = INT_MAX;
  test13y = CONSTTESTFACTOR;
  // CASE13: SignedMultiOverflowCheck.c:[[@LINE+1]]: signed overflow error (mul)
  result13 = test13x * test13y;
  return 0;

#elif TEST_CASE == 14
  /* Multiplication: deliberate concrete overflow (long long) */
  volatile long long test14x, test14y;
  long long result14;
  test14x = LLONG_MAX;
  test14y = CONSTTESTFACTOR;
  // CASE14: SignedMultiOverflowCheck.c:[[@LINE+1]]: signed overflow error (mul)
  result14 = test14x * test14y;
  return 0;

#elif TEST_CASE == 15
  /* Multiplication: symbolic overflow possible (int) */
  volatile int test15x, test15y;
  int result15;
  klee_make_symbolic((void *)&test15x, sizeof(test15x), "test15x");
  klee_make_symbolic((void *)&test15y, sizeof(test15y), "test15y");
  // CASE15: SignedMultiOverflowCheck.c:[[@LINE+1]]: signed overflow error (mul)
  result15 = test15x * test15y;
  return 0;

#elif TEST_CASE == 16
  /* Multiplication: symbolic overflow possible (long long) */
  volatile long long test16x, test16y;
  long long result16;
  klee_make_symbolic((void *)&test16x, sizeof(test16x), "test16x");
  klee_make_symbolic((void *)&test16y, sizeof(test16y), "test16y");
  // CASE16: SignedMultiOverflowCheck.c:[[@LINE+1]]: signed overflow error (mul)
  result16 = test16x * test16y;
  return 0;

#elif TEST_CASE == 17
  /* Multiplication: symbolic conditional test */
  volatile unsigned short cond1, cond2;
  volatile int test17x, test17y;
  int result17;

  klee_make_symbolic((void *)&cond1, sizeof(cond1), "cond1");
  klee_make_symbolic((void *)&cond2, sizeof(cond2), "cond2");
  klee_assume(cond1 < 2);
  klee_assume(cond2 < 2);

  test17x = cond1 ? INT_MAX : CONSTTESTNUM;
  test17y = cond2 ? CONSTTESTFACTOR : CONSTTESTOTHER;
  // CASE17: SignedMultiOverflowCheck.c:[[@LINE+1]]: signed overflow error (mul)
  result17 = test17x * test17y;
  return 0;

#elif TEST_CASE == 18
  /* Multiplication: control case */
  volatile int test18x, test18y;
  int result18;
  test18x = CONSTTESTNUM;
  test18y = CONSTTESTOTHER;
  // CASE18-NOT: SignedMultiOverflowCheck.c:[[@LINE+1]]: signed overflow error
  // CASE18: KLEE: done:
  result18 = test18x * test18y;
  return 0;
#endif
}