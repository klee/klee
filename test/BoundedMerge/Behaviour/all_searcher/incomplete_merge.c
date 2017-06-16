// RUN: %S/setup_test.sh "%llvmgcc" %s %t "-incomplete-bounded-merge" | FileCheck %s

// CHECK: open merge:

#include "klee/klee.h"

int fun_1 (in_1, in_2) {
  int result = 75;
  if (in_2 < 9000) {
    in_1 = result;
    in_1 = result * in_1 + in_2;
    result = in_1;
    in_1 = result + in_2;
  }
  else if (in_2 < 5000) {
    in_1 = in_2 + result;
    in_2 = 111;
    in_2 = in_2;
    result = in_1;
  }
  else {
    in_1 = in_1 + result + in_2;
    in_2 = result * in_2;
    in_2 = in_1 + result;
    if (in_1 > 100) {
      in_1 = result * in_1 + in_2;
      in_1 = result + in_1 - in_2;
      in_2 = in_2 + result;
    }
    else if (result > 800) {
      result = in_1 + result;
      result = result;
      in_2 = result + in_2;
      in_2 = in_1 + result - in_2;
      result = in_1 * result;
    }
    else {
      result = in_2;
      result = in_1;
      in_2 = 57;
    }
  }
  return result;
}

int fun_2 (in_1, in_2, in_3) {
  int result = 48;
  if (in_2 % 6 == 0) {
    in_2 = result + in_3 - in_1;
    in_2 = in_1 * in_3;
    result = in_1 * result;
    in_1 = in_3;
    in_1 = in_1;
    in_2 = result;
  }
  else if (result % 2 == 0) {
    in_2 = in_3 + in_1;
    result = in_3 + in_2;
    result = in_1 * in_3 - in_2;
    result = result + in_1;
    if (in_1 < 50) {
      in_2 = in_2 * in_1 - result - in_3;
      in_3 = 12;
      result = in_3;
    }
    else {
      in_1 = in_2 * in_3;
      result = result * in_3 * in_1 - in_2;
      in_2 = in_3 + in_2 * in_1;
      in_1 = in_2 - result * in_3;
    }
  }
  else {
    in_1 = in_2 - in_1;
    in_2 = in_2;
    in_1 = result * in_1 * in_3;
    in_1 = 100;
    if (in_1 > 700) {
      result = result + in_1;
      result = in_2;
      in_2 = result * in_3 * in_2 - in_1;
      in_3 = in_3 - in_1;
    }
    else {
      in_2 = in_2 * result + in_1;
      result = in_1 * in_3;
      in_2 = result + in_2;
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
  if (var_3 > 2000) {
    var_2 = var_3;
    var_4 = var_1;
    var_4 = fun_2(var_4, var_2, var_3);
    if (var_2 < 800) {
      var_4 = var_1 - var_2;
      var_4 = var_4 * var_2;
      var_3 = fun_1(var_3, var_2);
    }
    else if (var_2 < 9000) {
      var_4 = var_4 + var_1 * var_2;
      var_1 = var_3 * var_4;
      var_4 = var_2 + var_3 - var_4 - var_1;
    }
    else {
      var_3 = var_1 - var_2 * var_4 - var_3;
      var_2 = var_3;
      var_1 = var_1 - var_4;
    }
  }
  else if (var_2 < 700) {
    var_1 = var_3 - var_4;
    var_4 = var_3;
    var_4 = var_2 - var_3 - var_4 * var_1;
    var_4 = var_1 * var_3;
    if (var_3 > 90) {
      var_3 = 199;
      var_4 = var_3;
      var_2 = var_2 + var_4;
      var_3 = var_3 - var_4;
    }
    else if (var_1 < 600) {
      var_3 = var_1 + var_2;
      var_2 = var_4 - var_1 - var_2;
      var_3 = var_3 + var_4 + var_1 * var_2;
    }
    else {
      var_1 = var_1;
      var_1 = var_4 + var_1 * var_3 - var_2;
      var_3 = var_2;
    }
  }
  else {
    var_1 = var_2 - var_3 - var_4;
    var_1 = 57;
    var_1 = var_2;
    var_1 = var_1 - var_3 * var_4;
    if (var_3 % 7 == 0) {
      var_2 = var_4 + var_1;
      var_2 = var_4;
      var_1 = 163;
      var_4 = fun_2(var_1, var_2, var_4);
    }
    else if (var_2 > 100) {
      var_2 = fun_1(var_3, var_2);
      var_3 = var_3;
      var_3 = 20;
    }
    else {
      var_4 = var_2;
      var_4 = fun_1(var_4, var_1);
      var_3 = fun_2(var_3, var_4, var_1);
    }
  }
  klee_close_merge();
}

