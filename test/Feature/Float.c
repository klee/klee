// RUN: %llvmgcc -emit-llvm -g -c %s -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out %t.bc > %t.log
// RUN: grep "3.30* -1.10* 2.420*" %t.log

#include <stdio.h>

float fadd(float a, float b) {
  return a + b;
}

float fsub(float a, float b) {
  return a - b;
}

float fmul(float a, float b) {
  return a * b;
}

int main() {
  printf("%f %f %f\n", fadd(1.1, 2.2), fsub(1.1, 2.2), fmul(1.1, 2.2));
  return 0;
}
