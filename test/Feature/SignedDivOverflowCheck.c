/*
 * These tests ensure that the provided division
 * operations, which are known to contain overflow
 * are correctly identified by klee, and that the
 * appropriate metadata is present.
 */

//RUN: %clang %s -DTEST_CASE=1 -emit-llvm -g %O0opt -c -o %t.case1.bc
//RUN: rm -rf %t.case1.klee-out
//RUN: %klee --output-dir=%t.case1.klee-out --check-signed-overflow %t.case1.bc 2> %t.case1.log
//RUN: FileCheck %s --check-prefix=CASE1 --input-file=%t.case1.log
//RUN: %clang %s -DTEST_CASE=2 -emit-llvm -g %O0opt -c -o %t.case2.bc
//RUN: rm -rf %t.case2.klee-out
//RUN: %klee --output-dir=%t.case2.klee-out --check-signed-overflow %t.case2.bc 2> %t.case2.log
//RUN: FileCheck %s --check-prefix=CASE2 --input-file=%t.case2.log
//RUN: %clang %s -DTEST_CASE=3 -emit-llvm -g %O0opt -c -o %t.case3.bc
//RUN: rm -rf %t.case3.klee-out
//RUN: %klee --output-dir=%t.case3.klee-out --check-signed-overflow %t.case3.bc 2> %t.case3.log
//RUN: FileCheck %s --check-prefix=CASE3 --input-file=%t.case3.log
//RUN: %clang %s -DTEST_CASE=4 -emit-llvm -g %O0opt -c -o %t.case4.bc
//RUN: rm -rf %t.case4.klee-out
//RUN: %klee --output-dir=%t.case4.klee-out --check-signed-overflow %t.case4.bc 2> %t.case4.log
//RUN: FileCheck %s --check-prefix=CASE4 --input-file=%t.case4.log
//RUN: %clang %s -DTEST_CASE=5 -emit-llvm -g %O0opt -c -o %t.case5.bc
//RUN: rm -rf %t.case5.klee-out
//RUN: %klee --output-dir=%t.case5.klee-out --check-signed-overflow %t.case5.bc 2> %t.case5.log
//RUN: FileCheck %s --check-prefix=CASE5 --input-file=%t.case5.log
//RUN: %clang %s -DTEST_CASE=6 -emit-llvm -g %O0opt -c -o %t.case6.bc
//RUN: rm -rf %t.case6.klee-out
//RUN: %klee --output-dir=%t.case6.klee-out --check-signed-overflow %t.case6.bc 2> %t.case6.log
//RUN: FileCheck %s --check-prefix=CASE6 --input-file=%t.case6.log

#include "klee/klee.h"
#include <limits.h>

#define CONSTTESTNUM 49
#define CONSTTESTDENOM 7

int main(void) {
  #if TEST_CASE == 1
    //Deliberate instance of concrete overflow (int)
    volatile int test1x, test1y;
    int result1;
    test1x = INT_MIN;
    test1y = -1;
    // CASE1: SignedDivOverflowCheck.c:[[@LINE+1]]: signed overflow error (div)
    result1 = test1x / test1y;
    return 0;

  #elif TEST_CASE == 2
    //Deliberate instance of concrete overflow (long long)
    volatile long long test2x, test2y;
    long long result2;
    test2x = LLONG_MIN;
    test2y = -1;
    // CASE2: SignedDivOverflowCheck.c:[[@LINE+1]]: signed overflow error (div)
    result2 = test2x / test2y;
    return 0;

  #elif TEST_CASE == 3
    //Instance of potential overflow possible (int)
    volatile int test3x, test3y;
    int result3;
    klee_make_symbolic((void *)&test3x, sizeof(test3x), "test3x");
    klee_make_symbolic((void *)&test3y, sizeof(test3y), "test3y");
    klee_assume(test3y != 0);
    // CASE3: SignedDivOverflowCheck.c:[[@LINE+1]]: signed overflow error (div)
    result3 = test3x / test3y;
    return 0;

  #elif TEST_CASE == 4
    //Instance of potential overflow possible (int)
    volatile long long test4x, test4y;
    long long result4;
    klee_make_symbolic((void *)&test4x, sizeof(test4x), "test4x");
    klee_make_symbolic((void *)&test4y, sizeof(test4y), "test4y");
    klee_assume(test4y != 0);
    // CASE4: SignedDivOverflowCheck.c:[[@LINE+1]]: signed overflow error (div)
    result4 = test4x / test4y;
    return 0;

  #elif TEST_CASE == 5
    //Additional symbolic conditional test case
    volatile unsigned short cond1, cond2;
    volatile int test5x, test5y;
    int result5;

    klee_make_symbolic((void *)&cond1, sizeof(cond1), "cond1");
    klee_make_symbolic((void *)&cond2, sizeof(cond2), "cond2");
    klee_assume(cond1 < 2);
    klee_assume(cond2 < 2);

    test5x = cond1 ? INT_MIN : CONSTTESTNUM;
    test5y = cond2 ? CONSTTESTDENOM : -1;
    // CASE5: SignedDivOverflowCheck.c:[[@LINE+1]]: signed overflow error (div)
    result5 = test5x / test5y;
    return 0;

  #elif TEST_CASE == 6
    //Control case
    volatile int test6x, test6y;
    int result6;
    test6x = CONSTTESTNUM;
    test6y = CONSTTESTDENOM;
    // CASE6-NOT: SignedDivOverflowCheck.c:[[@LINE+1]]: signed overflow error (div)
    // CASE6: KLEE: done:
    result6 = test6x / test6y;
    return 0;
  #endif
}