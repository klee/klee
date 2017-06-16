// RUN: %llvmgcc -emit-llvm -g -c -o %t.bc %s
// RUN: rm -rf %t.klee-out
// RUN: klee --output-dir=%t.klee-out --use-bounded-merge --search=random-path %t.bc

#include "klee/klee.h"
int fun_1 (in_1) {
  int result = 87;
  if (in_1 % 12 == 0) {
    result = result;
    result = in_1;
    if (result > 2000) {
      result = result - in_1;
    }
    else {
      in_1 = result;
    }
  }
  else if (result < 5000) {
    in_1 = 107;
    if (result > 10) {
      result = result;
      in_1 = in_1;
      result = result * in_1;
      result = result - in_1;
      in_1 = in_1 - result;
      result = in_1;
    }
    else {
      in_1 = in_1 * result;
      in_1 = 7;
    }
  }
  else {
    result = result;
    in_1 = result;
  }
  return result;
}

int fun_2 (in_1, in_2) {
  int result = 187;
  if (in_2 < 30) {
    result = result;
    if (in_2 < 80) {
      result = in_2;
    }
    else {
      in_1 = in_2;
    }
  }
  else if (result > 700) {
    in_2 = in_1;
  }
  else {
    result = in_1;
    result = in_2 - in_1 * result;
    if (in_1 > 400) {
      result = result + in_1;
    }
    else {
      result = 147;
      result = result + in_1 + in_2;
      in_1 = in_1;
      result = in_2;
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
  if (var_3 < 9000) {
    var_4 = var_4 - var_2 * var_3;
    if (var_1 > 5000) {
      var_1 = var_3 - var_2;
      var_2 = fun_2(var_4, var_3);
      var_1 = var_2 + var_1;
      var_1 = var_2 * var_4 - var_1 * var_3;
      if (var_1 < 80) {
        var_2 = var_4 * var_3 * var_2;
        var_3 = var_3;
        var_4 = var_2;
        var_3 = var_3 * var_4 * var_1;
        var_3 = fun_2(var_3, var_1);
        var_4 = var_2;
        var_3 = var_4 - var_1 + var_2 + var_3;
        if (var_3 > 300) {
          var_3 = fun_2(var_2, var_3);
        }
        else {
          var_2 = var_3;
          var_4 = var_4 * var_2;
          var_1 = var_1;
          var_1 = var_4;
          var_2 = var_2 + var_1;
          var_2 = 113;
          var_1 = 169;
          if (var_2 % 27 == 0) {
            var_1 = var_4;
            var_2 = var_2 * var_3 - var_1;
            var_4 = fun_1(var_2);
            if (var_4 > 800) {
              var_1 = 20;
              var_4 = var_4 * var_1 * var_3;
            }
            else {
              var_1 = 126;
            }
          }
          else if (var_2 < 100) {
            var_4 = fun_1(var_4);
          }
          else {
            var_4 = var_2 * var_4;
          }
        }
      }
      else {
        var_2 = var_4 - var_2 + var_3;
      }
    }
    else if (var_4 % 25 == 0) {
      var_4 = var_2;
      var_1 = var_3;
    }
    else {
      var_3 = 20;
      var_4 = 66;
    }
  }
  else {
    var_2 = var_4;
    var_4 = fun_1(var_4);
    var_4 = fun_1(var_2);
    if (var_3 > 300) {
      var_2 = var_3;
    }
    else {
      var_3 = var_1 + var_3;
      var_2 = fun_2(var_2, var_4);
    }
  }
  klee_close_merge();
}

