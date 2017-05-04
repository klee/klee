// RUN: %llvmgcc -emit-llvm -g -c -o %t.bc %s
// RUN: rm -rf %t.klee-out
// RUN: klee --output-dir=%t.klee-out --use-bounded-merge --incomplete-bounded-merge --debug-log-incomplete-merge --search=dfs %t.bc 2>&1 | FileCheck %s

// CHECK: Preemptively releasing states

// This might fail, the generated test count fluctuates between 8 and 10
// CHECK-NOT: KLEE: done: generated tests = 73
// CHECK-NOT: KLEE: done: generated tests = 1 

#include "klee/klee.h"

int fun_1 (in_1, in_2) {
  int result = 153;
  if (result < 5000) {
    in_1 = in_2 + in_1 - result;
    in_2 = in_1;
    result = in_2;
    if (in_2 < 6000) {
      in_1 = in_2 - in_1;
      in_2 = in_2 * in_1;
      in_2 = in_1 - result;
      in_1 = in_1 + in_2;
      in_1 = result;
    }
    else if (in_2 < 800) {
      result = in_1 + in_2 + result;
      in_1 = in_2 - result;
      in_1 = result * in_1;
    }
    else {
      in_2 = in_2 - result * in_1;
      result = in_2 * in_1 + result;
      result = in_1 * result * in_2;
    }
  }
  else {
    in_1 = in_2 + in_1;
    in_1 = result * in_2;
    in_2 = result + in_2 + in_1;
    if (in_1 % 29 == 0) {
      in_1 = in_2;
      in_2 = in_2 * result;
      in_1 = in_1 * in_2 * result;
      in_2 = 87;
      in_1 = result + in_2 * in_1;
      in_2 = in_1 - in_2 - result;
      in_1 = in_1 + result;
    }
    else {
      result = result * in_1 * in_2;
      in_1 = result * in_2;
      result = in_1 * in_2 * result;
    }
  }
  return result;
}

int fun_2 (in_1, in_2, in_3) {
  int result = 142;
  if (result < 10) {
    in_2 = in_1 + result;
    in_3 = in_3;
    in_1 = in_2 * result;
    in_1 = in_2 + in_1;
  }
  else if (in_3 % 14 == 0) {
    in_3 = in_2 + in_1;
    result = result * in_3;
    in_2 = in_2 + result;
    in_3 = result - in_2 + in_1;
    if (in_1 % 18 == 0) {
      in_2 = in_3 - in_1 + result + in_2;
      in_2 = result * in_2 - in_1 - in_3;
      in_2 = in_1 + result;
      in_2 = in_1 - result;
      in_2 = in_3 * in_2;
    }
    else {
      in_2 = result + in_1;
      in_3 = in_3;
      in_3 = result + in_2;
    }
  }
  else {
    in_1 = in_3 - in_2 * in_1;
    in_2 = result - in_1;
    in_1 = 13;
    if (in_2 < 8000) {
      in_1 = in_2 + result * in_3;
      in_3 = in_1;
      in_1 = in_2 - result;
      in_2 = result - in_1;
      in_1 = in_3 - result * in_1;
    }
    else {
      in_2 = in_2 - result;
      in_2 = in_2;
      in_2 = in_1;
    }
  }
  return result;
}

int main(int argc, char** args) {
  int var_1;   klee_make_symbolic(&var_1, sizeof(var_1), "var_1");
  int var_2;   klee_make_symbolic(&var_2, sizeof(var_2), "var_2");
  int var_3;   klee_make_symbolic(&var_3, sizeof(var_3), "var_3");
  int var_4;   klee_make_symbolic(&var_4, sizeof(var_4), "var_4");
  klee_open_merge();
  if (var_2 > 5000) {
    var_1 = var_3 * var_1 + var_4 + var_2;
    var_3 = var_3 * var_1 - var_4;
    var_2 = var_1 + var_4 * var_2 + var_3;

    klee_open_merge();

    if (var_3 > 30) {
      var_1 = fun_2(var_2, var_3, var_1);
      var_3 = var_4;
      var_2 = var_1 * var_3 + var_4 + var_2;
      var_3 = var_4;
      if (var_2 > 80) {
        var_3 = var_2 * var_3;
        var_1 = var_3 - var_2;
        var_3 = var_4 - var_2;
        var_1 = var_3 + var_2;
        var_2 = var_4 * var_1;
      }
      else {
        var_4 = 20;
        var_1 = var_2 + var_1;
        var_3 = fun_1(var_1, var_2);
        var_3 = var_1 - var_3 * var_2;
        var_1 = var_1 * var_4 - var_3 + var_2;
      }
    }
    else if (var_1 > 1000) {
      var_4 = var_2 * var_4;
    }
    else if (var_1 < 100) {
      var_2 = 130;
      var_1 = 47;
      var_1 = 99;
    }
    else {
      var_1 = 168;
      var_1 = var_1 + var_2;
      var_2 = var_4 + var_2;
      var_3 = fun_2(var_1, var_2, var_4);
      var_2 = fun_2(var_1, var_2, var_4);
      var_2 = var_4 - var_2 - var_3;
      if (var_2 % 30 == 0) {
        var_3 = var_1 - var_4;
        var_1 = var_2 + var_4;
        var_4 = var_2 * var_1 * var_4 * var_3;
      }
      else {
        var_3 = var_2 + var_4 - var_3;
        var_2 = fun_1(var_2, var_1);
        var_2 = var_3;
      }
    }

    klee_close_merge();
  }
  else if (var_2 > 3000) {
    var_3 = var_4 * var_3 - var_1;
    var_2 = fun_2(var_3, var_1, var_2);
    var_2 = var_2;
    if (var_4 % 3 == 0) {
      var_1 = var_2;
      var_4 = var_4 - var_3;
      var_2 = var_1 + var_4;
      var_3 = fun_2(var_1, var_2, var_4);
      var_2 = fun_1(var_3, var_2);
    }
    else if (var_1 % 13 == 0) {
      var_1 = fun_2(var_3, var_4, var_1);
      var_2 = var_3;
      var_3 = var_1;
    }
    else {
      var_1 = var_3 + var_2;
      var_1 = var_4 + var_2;
      var_2 = var_3 - var_1;
    }
  }
  else {
    var_3 = var_1 + var_2;
    var_3 = var_4 - var_2;
    var_2 = var_3 * var_4 - var_2;
    var_2 = var_1;
    var_3 = var_3 + var_1;
  }
  klee_close_merge();
}

