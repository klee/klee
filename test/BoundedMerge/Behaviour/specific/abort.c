// RUN: %llvmgcc -emit-llvm -g -c -o %t.bc %s
// RUN: rm -rf %t.klee-out
// RUN: klee --output-dir=%t.klee-out --use-bounded-merge --search=random-path --max-time=5 %t.bc

#include "klee/klee.h"

int fun_1 (in_1, in_2) {
  int result = 155;
  if (in_1 % 4 == 0) {
    in_2 = result;
    if (result % 12 == 0) {
      in_2 = in_1;
      result = in_2;
      in_2 = in_2;
    }
    else {
      in_1 = in_2;
    }
  }
  else {
    in_2 = in_2;
    if (in_2 % 7 == 0) {
      result = result + in_2 - in_1;
    }
    else if (in_1 > 1000) {
      in_2 = result;
    }
    else {
      in_1 = in_2 * in_1;
      result = in_2;
    }
  }
  return result;
}

int fun_2 (in_1) {
  int result = 73;
  if (in_1 % 14 == 0) {
    in_1 = in_1;
    in_1 = result * in_1;
    if (in_1 % 29 == 0) {
      result = in_1;
    }
    else if (result > 1000) {
      result = result;
      result = in_1;
      result = result;
      result = result + in_1;
      result = in_1 + result;
      result = result;
    }
    else {
      in_1 = result;
      in_1 = result;
    }
  }
  else {
    in_1 = in_1;
    if (in_1 > 100) {
      result = in_1;
      result = 69;
      result = in_1;
      in_1 = 95;
    }
    else {
      in_1 = 61;
    }
  }
  return result;
}

int main(int argc, char** args) {
  int var_1;   klee_make_symbolic(&var_1, sizeof(var_1), "var_1");
  int var_2;   klee_make_symbolic(&var_2, sizeof(var_2), "var_2");
  int var_3;   klee_make_symbolic(&var_3, sizeof(var_3), "var_3");
  int var_4;   klee_make_symbolic(&var_4, sizeof(var_4), "var_4");
  if (var_2 < 50) {
    var_4 = var_4 - var_2 + var_3;
    if (var_1 < 300) {
      var_2 = fun_2(var_2);
      if (var_1 % 5 == 0) {
        var_3 = var_1;
        if (var_1 > 50) {
          var_3 = var_1;
          if (var_4 < 80) {
            var_2 = var_2;
          }
          else {
            var_2 = var_4 * var_1 * var_2;
          }
        }
        else if (var_1 > 5000) {
          var_2 = var_4;
          if (var_4 < 300) {
            var_4 = var_1 + var_4 * var_2;
          }
          else {
            var_1 = var_4 * var_1;
            var_1 = var_3 * var_2;
            var_1 = var_2;
          }
        }
        else if (var_2 % 30 == 0) {
          var_2 = var_4 * var_2 * var_3;
          var_2 = fun_2(var_1);
        }
        else if (var_2 < 9000) {
          var_4 = fun_2(var_3);
          var_3 = var_4 - var_2;
          var_1 = var_2;
          var_1 = var_3;
          var_3 = var_4 - var_1 * var_2;
          var_4 = var_4;
          if (var_1 > 500) {
            var_2 = var_4 * var_2;
          }
          else {
            var_2 = var_1;
          }
        }
        else {
          var_1 = var_2 - var_1;
          var_4 = var_3 + var_4 + var_1;
          if (var_3 < 9000) {
            var_2 = var_4;
            if (var_1 % 4 == 0) {
              var_3 = var_2;
              if (var_4 > 80) {
                var_4 = var_2 - var_1 * var_3;
                var_3 = var_2;
                var_2 = fun_1(var_2, var_3);
                var_2 = var_2 - var_3 * var_1 - var_4;
              }
              else {
                var_1 = var_4 * var_2 - var_3;
                var_2 = fun_1(var_1, var_3);
                var_4 = var_3 + var_1;
                var_1 = 24;
                var_3 = 115;
                var_4 = fun_1(var_3, var_2);
              }
            }
            else {
              var_2 = var_1;
            }
          }
          else if (var_1 > 1000) {
            var_2 = var_1;
            var_4 = var_4;
            var_1 = var_1;
            var_3 = fun_2(var_2);
            var_2 = var_1 + var_2;
            var_3 = var_4 - var_1 * var_2;
          }
          else {
            var_3 = fun_1(var_1, var_3);
          }
        }
      }
      else if (var_4 > 7000) {
        var_4 = fun_2(var_3);
        var_3 = var_2 * var_4;
        var_1 = var_4 - var_3;
        var_1 = var_2 + var_4 - var_3;
      }
      else {
        var_3 = var_3;
        var_4 = 25;
        var_3 = var_2;
        var_2 = var_1;
        var_1 = fun_2(var_2);
        var_2 = var_4;
        var_4 = var_1;
        var_2 = var_1 + var_3 + var_2;
      }
    }
    else if (var_3 % 23 == 0) {
      var_1 = var_4;
      if (var_1 < 60) {
        var_3 = var_4 * var_2 * var_3 - var_1;
      }
      else if (var_4 % 8 == 0) {
        var_4 = fun_1(var_4, var_1);
        var_1 = var_4;
        if (var_1 > 30) {
          var_1 = var_3 - var_1;
          if (var_4 % 18 == 0) {
            var_2 = var_3;
            var_3 = 125;
          }
          else {
            var_4 = var_2;
          }
        }
        else if (var_2 > 200) {
          var_3 = var_2;
          var_1 = fun_2(var_4);
        }
        else if (var_2 < 5000) {
          var_1 = 8;
        }
        else {
          var_3 = fun_1(var_4, var_1);
          if (var_4 > 1000) {
            var_3 = var_2 - var_1;
          }
          else if (var_3 % 28 == 0) {
            var_4 = fun_2(var_3);
          }
          else {
            var_2 = var_1 - var_4 * var_3;
            var_1 = fun_2(var_1);
            var_3 = var_4;
          }
        }
      }
      else {
        var_2 = var_1;
        var_3 = fun_1(var_3, var_1);
        if (var_2 > 6000) {
          var_2 = fun_1(var_3, var_4);
          var_2 = fun_2(var_2);
        }
        else {
          var_1 = var_4;
          var_3 = fun_1(var_4, var_3);
          if (var_4 > 9000) {
            var_4 = var_1;
          }
          else {
            var_3 = var_4;
          }
        }
      }
    }
    else {
      var_2 = var_2;
      if (var_1 > 600) {
        var_2 = var_2;
        var_3 = var_1 - var_4;
      }
      else {
        var_3 = var_3;
        var_3 = fun_1(var_2, var_3);
      }
    }
  }
  else if (var_4 > 400) {
    var_1 = var_4;
    if (var_1 % 15 == 0) {
      var_2 = var_2;
      var_4 = fun_1(var_1, var_4);
      var_4 = var_3 + var_4 + var_1;
      var_1 = var_3;
      if (var_3 < 900) {
        var_3 = fun_2(var_2);
        var_3 = var_4 - var_3 - var_1 + var_2;
        var_1 = var_2;
        if (var_4 % 21 == 0) {
          var_2 = var_1 * var_2;
          var_2 = var_4;
        }
        else if (var_2 < 70) {
          var_3 = var_1 * var_4 - var_3;
        }
        else {
          var_4 = var_4;
          var_4 = var_2;
        }
      }
      else {
        var_1 = var_2;
        var_3 = var_2;
      }
    }
    else if (var_1 > 6000) {
      var_2 = fun_2(var_4);
      var_1 = var_1 * var_3 + var_2 * var_4;
      if (var_3 > 30) {
        var_2 = var_1 + var_4;
        if (var_2 < 40) {
          var_4 = var_3 * var_1 - var_2 + var_4;
        }
        else {
          var_4 = var_4 - var_1;
        }
      }
      else if (var_2 > 20) {
        var_2 = 124;
        var_3 = var_3;
        if (var_4 % 30 == 0) {
          var_2 = var_1;
          var_4 = fun_2(var_4);
          var_4 = var_2;
          if (var_4 % 10 == 0) {
            var_2 = var_4;
            var_2 = var_2;
            if (var_4 > 30) {
              var_4 = fun_2(var_3);
              var_3 = var_1;
              var_3 = fun_1(var_4, var_1);
            }
            else {
              var_1 = 163;
              var_1 = fun_2(var_4);
              if (var_2 > 800) {
                var_2 = var_2;
              }
              else if (var_2 > 10) {
                var_1 = var_3;
              }
              else {
                var_1 = var_4 * var_2 + var_1;
                var_3 = 154;
              }
            }
          }
          else if (var_1 < 800) {
            var_3 = var_1;
            if (var_4 % 23 == 0) {
              var_1 = var_1 - var_4;
              if (var_4 % 6 == 0) {
                var_3 = var_3 - var_1;
                var_1 = fun_1(var_1, var_2);
              }
              else {
                var_3 = var_2 * var_4 * var_1;
              }
            }
            else {
              var_3 = var_3 * var_1;
            }
          }
          else if (var_3 % 24 == 0) {
            var_3 = fun_1(var_1, var_2);
            var_4 = var_2 * var_3;
            if (var_2 < 900) {
              var_2 = var_1 * var_4 * var_2;
              var_2 = var_4 * var_3;
              var_1 = fun_1(var_2, var_3);
            }
            else {
              var_1 = fun_2(var_2);
            }
          }
          else {
            var_1 = 48;
            if (var_3 > 7000) {
              var_3 = var_1 - var_4 * var_2;
            }
            else {
              var_2 = var_4;
              var_4 = var_1;
              var_4 = var_2;
              if (var_1 > 90) {
                var_1 = var_2 * var_4;
              }
              else if (var_3 < 7000) {
                var_2 = var_2 - var_1;
              }
              else {
                var_2 = 179;
                var_3 = var_3 * var_1;
                var_2 = 73;
                var_2 = fun_2(var_2);
              }
            }
          }
        }
        else if (var_2 % 12 == 0) {
          var_2 = var_3;
          var_3 = var_3;
          if (var_2 > 8000) {
            var_4 = var_3;
            if (var_1 < 10) {
              var_3 = var_2 * var_1 - var_3;
              var_4 = var_2;
              var_4 = 139;
              if (var_2 > 700) {
                var_1 = var_1;
              }
              else if (var_3 % 21 == 0) {
                var_4 = var_3 - var_4;
              }
              else {
                var_3 = var_3 - var_1 * var_2;
                var_2 = var_3 * var_2 - var_1 + var_4;
                var_3 = fun_1(var_3, var_1);
              }
            }
            else if (var_4 % 30 == 0) {
              var_4 = var_3;
            }
            else {
              var_4 = fun_1(var_2, var_4);
              if (var_1 % 22 == 0) {
                var_4 = var_3 - var_2 * var_1 + var_4;
              }
              else {
                var_2 = var_1;
              }
            }
          }
          else if (var_2 > 90) {
            var_3 = fun_2(var_2);
            var_1 = var_4 * var_1;
            if (var_3 % 16 == 0) {
              var_1 = var_1 - var_3 * var_4;
              var_1 = var_1;
              var_1 = var_4;
              var_3 = fun_1(var_4, var_3);
              var_3 = 138;
              if (var_1 < 8000) {
                var_1 = var_2;
                var_1 = var_2 * var_4 + var_3 - var_1;
              }
              else {
                var_3 = var_4;
                var_1 = fun_2(var_2);
              }
            }
            else {
              var_2 = var_3;
              var_2 = var_3;
            }
          }
          else {
            var_2 = fun_1(var_1, var_2);
          }
        }
        else {
          var_2 = var_4 + var_1 - var_2;
          var_2 = var_2;
          var_2 = fun_1(var_4, var_1);
          if (var_1 < 4000) {
            var_2 = var_2 + var_3;
          }
          else if (var_4 < 50) {
            var_2 = var_4;
          }
          else if (var_1 % 20 == 0) {
            var_1 = var_1;
            var_4 = fun_1(var_1, var_4);
            var_2 = fun_2(var_4);
            var_1 = 166;
          }
          else if (var_2 % 26 == 0) {
            var_2 = var_2;
            if (var_1 > 700) {
              var_4 = var_1 - var_2;
              var_4 = var_1;
              var_3 = var_2 - var_1 - var_4;
              var_1 = var_4;
            }
            else {
              var_4 = fun_2(var_2);
            }
          }
          else {
            var_2 = 35;
            if (var_2 > 3000) {
              var_2 = var_2;
              var_2 = var_2 - var_3;
              var_2 = fun_2(var_4);
              var_1 = var_1 - var_4 - var_3;
              var_2 = fun_1(var_3, var_2);
              var_3 = fun_1(var_2, var_1);
            }
            else {
              var_3 = var_2;
            }
          }
        }
      }
      else if (var_1 % 19 == 0) {
        var_2 = var_2;
        var_2 = fun_2(var_4);
        var_3 = 102;
        var_3 = 130;
        var_2 = var_2;
        var_3 = var_2;
      }
      else if (var_3 % 18 == 0) {
        var_1 = var_1 - var_2 + var_4;
        var_3 = var_4;
        var_3 = var_2;
        if (var_4 % 18 == 0) {
          var_3 = var_2;
        }
        else if (var_3 > 8000) {
          var_1 = var_4 - var_1 + var_3;
          var_1 = var_4 - var_3;
        }
        else {
          var_1 = var_1 + var_2 * var_4;
        }
      }
      else {
        var_3 = var_1 * var_3;
        var_3 = var_4 * var_3 * var_1;
        var_4 = fun_2(var_2);
        if (var_4 < 200) {
          var_2 = var_2;
          if (var_1 < 400) {
            var_1 = 153;
            if (var_2 % 28 == 0) {
              var_3 = fun_1(var_3, var_4);
            }
            else if (var_2 < 400) {
              var_1 = var_3 + var_1;
              if (var_3 > 600) {
                var_3 = 128;
                var_3 = 21;
                var_3 = var_3 - var_1;
                var_1 = fun_2(var_2);
                var_1 = var_2;
                var_4 = fun_2(var_3);
              }
              else {
                var_4 = 11;
              }
            }
            else {
              var_3 = var_4;
              var_2 = var_1;
              if (var_2 < 7000) {
                var_1 = var_4;
              }
              else {
                var_3 = var_2;
                var_2 = var_4 * var_1;
              }
            }
          }
          else {
            var_3 = var_3;
            if (var_3 % 13 == 0) {
              var_2 = fun_1(var_2, var_4);
              var_3 = var_3 - var_2;
              var_2 = var_1 + var_4;
              var_1 = var_4;
              if (var_3 < 70) {
                var_2 = fun_2(var_4);
              }
              else {
                var_1 = var_2 - var_3 * var_4;
                var_3 = var_3;
                var_4 = fun_2(var_1);
                var_3 = var_3;
              }
            }
            else {
              var_4 = fun_1(var_3, var_4);
            }
          }
        }
        else {
          var_3 = var_4;
          var_2 = var_4;
          if (var_4 % 2 == 0) {
            var_2 = 91;
            var_3 = 187;
            var_3 = var_4 * var_1 - var_2;
            if (var_4 < 700) {
              var_1 = var_2 * var_3 - var_1;
            }
            else {
              var_3 = var_2;
              var_2 = var_3;
            }
          }
          else if (var_2 < 2000) {
            var_3 = var_4 - var_1 - var_3;
            if (var_2 % 5 == 0) {
              var_3 = fun_2(var_3);
            }
            else if (var_3 < 2000) {
              var_2 = var_1 - var_3 - var_4 - var_2;
            }
            else {
              var_2 = var_1 * var_3;
            }
          }
          else {
            var_2 = var_1;
          }
        }
      }
    }
    else if (var_3 < 10) {
      var_3 = fun_2(var_2);
      var_4 = var_2;
      var_3 = var_2;
      if (var_1 > 400) {
        var_4 = var_1;
      }
      else if (var_2 % 24 == 0) {
        var_4 = var_2 - var_3 + var_4;
        var_4 = 192;
      }
      else {
        var_3 = var_1;
      }
    }
    else {
      var_3 = var_4 + var_3;
      var_4 = var_3 + var_2;
      var_4 = var_2 + var_3;
      var_2 = var_2;
      var_3 = var_3 * var_2 * var_4;
      var_1 = var_3;
      var_4 = var_2 + var_3 + var_1;
      if (var_2 < 40) {
        var_3 = var_3 + var_2;
        if (var_4 > 20) {
          var_3 = var_3 + var_1;
          if (var_4 > 90) {
            var_1 = var_3 + var_1 - var_4 - var_2;
            var_3 = 175;
            var_1 = var_3;
            var_2 = var_2;
            var_2 = fun_2(var_1);
          }
          else if (var_2 < 4000) {
            var_4 = var_4;
            var_3 = 27;
          }
          else {
            var_1 = 12;
            var_3 = var_2 - var_1 + var_3;
            var_3 = var_1 + var_3 + var_2;
            var_2 = var_4 - var_2;
          }
        }
        else {
          var_1 = fun_1(var_3, var_1);
          var_2 = var_2;
          var_3 = var_2;
          if (var_4 < 5000) {
            var_4 = var_2 - var_1;
            var_2 = 136;
          }
          else {
            var_4 = var_3 + var_4;
            if (var_1 < 4000) {
              var_1 = var_2 + var_3 - var_1;
              if (var_4 < 6000) {
                var_3 = fun_1(var_2, var_1);
              }
              else if (var_1 < 9000) {
                var_1 = 19;
                var_2 = var_4 * var_3 * var_2;
                var_1 = fun_2(var_1);
                var_1 = var_1;
              }
              else {
                var_2 = fun_1(var_4, var_3);
                var_2 = var_2 * var_1 - var_3;
                var_2 = fun_1(var_2, var_1);
                var_1 = var_2 * var_4;
                var_1 = var_4 + var_1 + var_2;
              }
            }
            else {
              var_4 = var_3;
            }
          }
        }
      }
      else if (var_3 % 5 == 0) {
        var_1 = var_3 - var_4;
      }
      else {
        var_4 = var_3;
        var_4 = fun_2(var_2);
        var_2 = 127;
        if (var_2 < 500) {
          var_3 = var_4;
        }
        else if (var_2 > 300) {
          var_2 = var_4;
          var_4 = var_4 - var_2;
        }
        else if (var_2 % 14 == 0) {
          var_4 = var_4;
        }
        else {
          var_4 = var_2 - var_3 + var_1 * var_4;
          var_3 = var_4 - var_2;
          var_4 = fun_1(var_4, var_1);
          var_1 = var_1;
          var_4 = var_3;
        }
      }
    }
  }
  else {
    var_1 = var_3;
    if (var_2 % 18 == 0) {
      var_3 = fun_2(var_1);
    }
    else {
      var_1 = var_4;
      if (var_2 % 15 == 0) {
        var_1 = fun_2(var_1);
      }
      else {
        var_2 = var_1 * var_3;
        var_2 = var_3;
      }
    }
  }
}

