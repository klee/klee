// RUN: %clang  -Wno-everything %s -emit-llvm %O0opt -g -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --optimize-aggressive=false --track-coverage=branches --optimize=true --emit-all-errors --only-output-states-covering-new=true --dump-states-on-halt=true --search=dfs %t1.bc 2>&1 | FileCheck -check-prefix=CHECK %s

#include "klee-test-comp.c"

// This file is part of the SV-Benchmarks collection of verification tasks:
// https://github.com/sosy-lab/sv-benchmarks
//
// SPDX-FileCopyrightText: 2016 Gilles Audemard
// SPDX-FileCopyrightText: 2020 Dirk Beyer <https://www.sosy-lab.org>
// SPDX-FileCopyrightText: 2020 The SV-Benchmarks Community
//
// SPDX-License-Identifier: MIT

extern void abort(void) __attribute__((__nothrow__, __leaf__))
__attribute__((__noreturn__));
extern void __assert_fail(const char *, const char *, unsigned int,
                          const char *) __attribute__((__nothrow__, __leaf__))
__attribute__((__noreturn__));
#ifdef GCOV
extern void __gcov_dump(void);
#endif

void abort_prog() {
#ifdef GCOV
  __gcov_dump();
#endif
  abort();
}
int __VERIFIER_nondet_int();
void reach_error() {
  abort_prog();
  __assert_fail("0", "Dubois-015.c", 5, "reach_error");
}
void assume(int cond) {
  if (!cond)
    abort_prog();
}
int main() {
  int cond0;
  int dummy = 0;
  int N;
  int var0;
  var0 = __VERIFIER_nondet_int();
  assume(var0 >= 0);
  assume(var0 <= 1);
  int var1;
  var1 = __VERIFIER_nondet_int();
  assume(var1 >= 0);
  assume(var1 <= 1);
  int var2;
  var2 = __VERIFIER_nondet_int();
  assume(var2 >= 0);
  assume(var2 <= 1);
  int var3;
  var3 = __VERIFIER_nondet_int();
  assume(var3 >= 0);
  assume(var3 <= 1);
  int var4;
  var4 = __VERIFIER_nondet_int();
  assume(var4 >= 0);
  assume(var4 <= 1);
  int var5;
  var5 = __VERIFIER_nondet_int();
  assume(var5 >= 0);
  assume(var5 <= 1);
  int var6;
  var6 = __VERIFIER_nondet_int();
  assume(var6 >= 0);
  assume(var6 <= 1);
  int var7;
  var7 = __VERIFIER_nondet_int();
  assume(var7 >= 0);
  assume(var7 <= 1);
  int var8;
  var8 = __VERIFIER_nondet_int();
  assume(var8 >= 0);
  assume(var8 <= 1);
  int var9;
  var9 = __VERIFIER_nondet_int();
  assume(var9 >= 0);
  assume(var9 <= 1);
  int var10;
  var10 = __VERIFIER_nondet_int();
  assume(var10 >= 0);
  assume(var10 <= 1);
  int var11;
  var11 = __VERIFIER_nondet_int();
  assume(var11 >= 0);
  assume(var11 <= 1);
  int var12;
  var12 = __VERIFIER_nondet_int();
  assume(var12 >= 0);
  assume(var12 <= 1);
  int var13;
  var13 = __VERIFIER_nondet_int();
  assume(var13 >= 0);
  assume(var13 <= 1);
  int var14;
  var14 = __VERIFIER_nondet_int();
  assume(var14 >= 0);
  assume(var14 <= 1);
  int var15;
  var15 = __VERIFIER_nondet_int();
  assume(var15 >= 0);
  assume(var15 <= 1);
  int var16;
  var16 = __VERIFIER_nondet_int();
  assume(var16 >= 0);
  assume(var16 <= 1);
  int var17;
  var17 = __VERIFIER_nondet_int();
  assume(var17 >= 0);
  assume(var17 <= 1);
  int var18;
  var18 = __VERIFIER_nondet_int();
  assume(var18 >= 0);
  assume(var18 <= 1);
  int var19;
  var19 = __VERIFIER_nondet_int();
  assume(var19 >= 0);
  assume(var19 <= 1);
  int var20;
  var20 = __VERIFIER_nondet_int();
  assume(var20 >= 0);
  assume(var20 <= 1);
  int var21;
  var21 = __VERIFIER_nondet_int();
  assume(var21 >= 0);
  assume(var21 <= 1);
  int var22;
  var22 = __VERIFIER_nondet_int();
  assume(var22 >= 0);
  assume(var22 <= 1);
  int var23;
  var23 = __VERIFIER_nondet_int();
  assume(var23 >= 0);
  assume(var23 <= 1);
  int var24;
  var24 = __VERIFIER_nondet_int();
  assume(var24 >= 0);
  assume(var24 <= 1);
  int var25;
  var25 = __VERIFIER_nondet_int();
  assume(var25 >= 0);
  assume(var25 <= 1);
  int var26;
  var26 = __VERIFIER_nondet_int();
  assume(var26 >= 0);
  assume(var26 <= 1);
  int var27;
  var27 = __VERIFIER_nondet_int();
  assume(var27 >= 0);
  assume(var27 <= 1);
  int var28;
  var28 = __VERIFIER_nondet_int();
  assume(var28 >= 0);
  assume(var28 <= 1);
  int var29;
  var29 = __VERIFIER_nondet_int();
  assume(var29 >= 0);
  assume(var29 <= 1);
  int var30;
  var30 = __VERIFIER_nondet_int();
  assume(var30 >= 0);
  assume(var30 <= 1);
  int var31;
  var31 = __VERIFIER_nondet_int();
  assume(var31 >= 0);
  assume(var31 <= 1);
  int var32;
  var32 = __VERIFIER_nondet_int();
  assume(var32 >= 0);
  assume(var32 <= 1);
  int var33;
  var33 = __VERIFIER_nondet_int();
  assume(var33 >= 0);
  assume(var33 <= 1);
  int var34;
  var34 = __VERIFIER_nondet_int();
  assume(var34 >= 0);
  assume(var34 <= 1);
  int var35;
  var35 = __VERIFIER_nondet_int();
  assume(var35 >= 0);
  assume(var35 <= 1);
  int var36;
  var36 = __VERIFIER_nondet_int();
  assume(var36 >= 0);
  assume(var36 <= 1);
  int var37;
  var37 = __VERIFIER_nondet_int();
  assume(var37 >= 0);
  assume(var37 <= 1);
  int var38;
  var38 = __VERIFIER_nondet_int();
  assume(var38 >= 0);
  assume(var38 <= 1);
  int var39;
  var39 = __VERIFIER_nondet_int();
  assume(var39 >= 0);
  assume(var39 <= 1);
  int var40;
  var40 = __VERIFIER_nondet_int();
  assume(var40 >= 0);
  assume(var40 <= 1);
  int var41;
  var41 = __VERIFIER_nondet_int();
  assume(var41 >= 0);
  assume(var41 <= 1);
  int var42;
  var42 = __VERIFIER_nondet_int();
  assume(var42 >= 0);
  assume(var42 <= 1);
  int var43;
  var43 = __VERIFIER_nondet_int();
  assume(var43 >= 0);
  assume(var43 <= 1);
  int var44;
  var44 = __VERIFIER_nondet_int();
  assume(var44 >= 0);
  assume(var44 <= 1);
  int myvar0 = 1;
  assume((var28 == 0 & var29 == 0 & var0 == 1) |
         (var28 == 0 & var29 == 1 & var0 == 0) |
         (var28 == 1 & var29 == 0 & var0 == 0) |
         (var28 == 1 & var29 == 1 & var0 == 1) | 0);
  assume((var1 == 0 & var31 == 0 & var2 == 1) |
         (var1 == 0 & var31 == 1 & var2 == 0) |
         (var1 == 1 & var31 == 0 & var2 == 0) |
         (var1 == 1 & var31 == 1 & var2 == 1) | 0);
  assume((var2 == 0 & var32 == 0 & var3 == 1) |
         (var2 == 0 & var32 == 1 & var3 == 0) |
         (var2 == 1 & var32 == 0 & var3 == 0) |
         (var2 == 1 & var32 == 1 & var3 == 1) | 0);
  assume((var3 == 0 & var33 == 0 & var4 == 1) |
         (var3 == 0 & var33 == 1 & var4 == 0) |
         (var3 == 1 & var33 == 0 & var4 == 0) |
         (var3 == 1 & var33 == 1 & var4 == 1) | 0);
  assume((var4 == 0 & var34 == 0 & var5 == 1) |
         (var4 == 0 & var34 == 1 & var5 == 0) |
         (var4 == 1 & var34 == 0 & var5 == 0) |
         (var4 == 1 & var34 == 1 & var5 == 1) | 0);
  assume((var5 == 0 & var35 == 0 & var6 == 1) |
         (var5 == 0 & var35 == 1 & var6 == 0) |
         (var5 == 1 & var35 == 0 & var6 == 0) |
         (var5 == 1 & var35 == 1 & var6 == 1) | 0);
  assume((var6 == 0 & var36 == 0 & var7 == 1) |
         (var6 == 0 & var36 == 1 & var7 == 0) |
         (var6 == 1 & var36 == 0 & var7 == 0) |
         (var6 == 1 & var36 == 1 & var7 == 1) | 0);
  assume((var7 == 0 & var37 == 0 & var8 == 1) |
         (var7 == 0 & var37 == 1 & var8 == 0) |
         (var7 == 1 & var37 == 0 & var8 == 0) |
         (var7 == 1 & var37 == 1 & var8 == 1) | 0);
  assume((var8 == 0 & var38 == 0 & var9 == 1) |
         (var8 == 0 & var38 == 1 & var9 == 0) |
         (var8 == 1 & var38 == 0 & var9 == 0) |
         (var8 == 1 & var38 == 1 & var9 == 1) | 0);
  assume((var9 == 0 & var39 == 0 & var10 == 1) |
         (var9 == 0 & var39 == 1 & var10 == 0) |
         (var9 == 1 & var39 == 0 & var10 == 0) |
         (var9 == 1 & var39 == 1 & var10 == 1) | 0);
  assume((var10 == 0 & var40 == 0 & var11 == 1) |
         (var10 == 0 & var40 == 1 & var11 == 0) |
         (var10 == 1 & var40 == 0 & var11 == 0) |
         (var10 == 1 & var40 == 1 & var11 == 1) | 0);
  assume((var11 == 0 & var41 == 0 & var12 == 1) |
         (var11 == 0 & var41 == 1 & var12 == 0) |
         (var11 == 1 & var41 == 0 & var12 == 0) |
         (var11 == 1 & var41 == 1 & var12 == 1) | 0);
  assume((var12 == 0 & var42 == 0 & var13 == 1) |
         (var12 == 0 & var42 == 1 & var13 == 0) |
         (var12 == 1 & var42 == 0 & var13 == 0) |
         (var12 == 1 & var42 == 1 & var13 == 1) | 0);
  assume((var0 == 0 & var30 == 0 & var1 == 1) |
         (var0 == 0 & var30 == 1 & var1 == 0) |
         (var0 == 1 & var30 == 0 & var1 == 0) |
         (var0 == 1 & var30 == 1 & var1 == 1) | 0);
  assume((var14 == 0 & var43 == 0 & var44 == 1) |
         (var14 == 0 & var43 == 1 & var44 == 0) |
         (var14 == 1 & var43 == 0 & var44 == 0) |
         (var14 == 1 & var43 == 1 & var44 == 1) | 0);
  assume((var13 == 0 & var43 == 0 & var44 == 1) |
         (var13 == 0 & var43 == 1 & var44 == 0) |
         (var13 == 1 & var43 == 0 & var44 == 0) |
         (var13 == 1 & var43 == 1 & var44 == 1) | 0);
  assume((var16 == 0 & var41 == 0 & var15 == 1) |
         (var16 == 0 & var41 == 1 & var15 == 0) |
         (var16 == 1 & var41 == 0 & var15 == 0) |
         (var16 == 1 & var41 == 1 & var15 == 1) | 0);
  assume((var17 == 0 & var40 == 0 & var16 == 1) |
         (var17 == 0 & var40 == 1 & var16 == 0) |
         (var17 == 1 & var40 == 0 & var16 == 0) |
         (var17 == 1 & var40 == 1 & var16 == 1) | 0);
  assume((var18 == 0 & var39 == 0 & var17 == 1) |
         (var18 == 0 & var39 == 1 & var17 == 0) |
         (var18 == 1 & var39 == 0 & var17 == 0) |
         (var18 == 1 & var39 == 1 & var17 == 1) | 0);
  assume((var19 == 0 & var38 == 0 & var18 == 1) |
         (var19 == 0 & var38 == 1 & var18 == 0) |
         (var19 == 1 & var38 == 0 & var18 == 0) |
         (var19 == 1 & var38 == 1 & var18 == 1) | 0);
  assume((var20 == 0 & var37 == 0 & var19 == 1) |
         (var20 == 0 & var37 == 1 & var19 == 0) |
         (var20 == 1 & var37 == 0 & var19 == 0) |
         (var20 == 1 & var37 == 1 & var19 == 1) | 0);
  assume((var21 == 0 & var36 == 0 & var20 == 1) |
         (var21 == 0 & var36 == 1 & var20 == 0) |
         (var21 == 1 & var36 == 0 & var20 == 0) |
         (var21 == 1 & var36 == 1 & var20 == 1) | 0);
  assume((var22 == 0 & var35 == 0 & var21 == 1) |
         (var22 == 0 & var35 == 1 & var21 == 0) |
         (var22 == 1 & var35 == 0 & var21 == 0) |
         (var22 == 1 & var35 == 1 & var21 == 1) | 0);
  assume((var23 == 0 & var34 == 0 & var22 == 1) |
         (var23 == 0 & var34 == 1 & var22 == 0) |
         (var23 == 1 & var34 == 0 & var22 == 0) |
         (var23 == 1 & var34 == 1 & var22 == 1) | 0);
  assume((var24 == 0 & var33 == 0 & var23 == 1) |
         (var24 == 0 & var33 == 1 & var23 == 0) |
         (var24 == 1 & var33 == 0 & var23 == 0) |
         (var24 == 1 & var33 == 1 & var23 == 1) | 0);
  assume((var25 == 0 & var32 == 0 & var24 == 1) |
         (var25 == 0 & var32 == 1 & var24 == 0) |
         (var25 == 1 & var32 == 0 & var24 == 0) |
         (var25 == 1 & var32 == 1 & var24 == 1) | 0);
  assume((var26 == 0 & var31 == 0 & var25 == 1) |
         (var26 == 0 & var31 == 1 & var25 == 0) |
         (var26 == 1 & var31 == 0 & var25 == 0) |
         (var26 == 1 & var31 == 1 & var25 == 1) | 0);
  assume((var27 == 0 & var30 == 0 & var26 == 1) |
         (var27 == 0 & var30 == 1 & var26 == 0) |
         (var27 == 1 & var30 == 0 & var26 == 0) |
         (var27 == 1 & var30 == 1 & var26 == 1) | 0);
  assume((var15 == 0 & var42 == 0 & var14 == 1) |
         (var15 == 0 & var42 == 1 & var14 == 0) |
         (var15 == 1 & var42 == 0 & var14 == 0) |
         (var15 == 1 & var42 == 1 & var14 == 1) | 0);
  assume((var28 == 0 & var29 == 0 & var27 == 0) |
         (var28 == 0 & var29 == 1 & var27 == 1) |
         (var28 == 1 & var29 == 0 & var27 == 1) |
         (var28 == 1 & var29 == 1 & var27 == 0) | 0);
  reach_error();
  return 0; /* 0 x[0]1 x[1]2 x[2]3 x[3]4 x[4]5 x[5]6 x[6]7 x[7]8 x[8]9 x[9]10
               x[10]11 x[11]12 x[12]13 x[13]14 x[14]15 x[15]16 x[16]17 x[17]18
               x[18]19 x[19]20 x[20]21 x[21]22 x[22]23 x[23]24 x[24]25 x[25]26
               x[26]27 x[27]28 x[28]29 x[29]30 x[30]31 x[31]32 x[32]33 x[33]34
               x[34]35 x[35]36 x[36]37 x[37]38 x[38]39 x[39]40 x[40]41 x[41]42
               x[42]43 x[43]44 x[44] */
}

// CHECK: generated tests = 2