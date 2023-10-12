// RUN: %clang  -Wno-everything %s -emit-llvm %O0opt -g -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --entry-point=klee_entry --skip-not-lazy-initialized --min-number-elements-li=1 %t1.bc 2>&1
// RUN: %ktest-tool %t.klee-out/test000003.ktest | FileCheck %s

#include "klee/klee.h"

struct StructWithPointer {
  int x;
  int *y;
};

struct StructWithDoublePointer {
  int x;
  int **y;
};

struct StructWithArrayOfPointer {
  int x;
  int *y[2];
};

struct StructWithStructWithPointer {
  struct StructWithPointer swp;
  struct StructWithDoublePointer *swdp;
};

struct StructManyPointers {
  int a;
  int *b;
  int **c;
  int ***d;
};

struct StructComplex {
  int x;
  int *y;
  int **z;
  struct StructWithPointer *swp;
  struct StructWithDoublePointer **swdp;
  struct StructManyPointers smp;
};

int sumStructWithPointer(struct StructWithPointer par) {
  return par.x + *par.y;
}

int sumStructWithPointerAsPointer(struct StructWithPointer *par) {
  return par->x + *par->y;
}

int sumStructWithDoublePointer(struct StructWithDoublePointer par) {
  return par.x + **par.y;
}

int sumStructWithArrayOfPointer(struct StructWithArrayOfPointer par) {
  return par.x + *(par.y[0]) + *(par.y[1]);
}

int sumStructWithStructWithPointer(struct StructWithStructWithPointer par) {
  int sswp = sumStructWithPointer(par.swp);
  int sswdp = sumStructWithDoublePointer(*par.swdp);
  return sswp + sswdp;
}

int sumStructManyPointers(struct StructManyPointers par) {
  return par.a + *par.b + **par.c + ***par.d;
}

int sumStructComplex(struct StructComplex par) {
  int sswp = sumStructWithPointer(*par.swp);
  int sswdp = sumStructWithDoublePointer(**par.swdp);
  int ssmp = sumStructManyPointers(par.smp);
  return par.x + *par.y + **par.z + sswp + sswdp + ssmp;
}

// CHECK: object 2: pointers: [(8, 3, 0)]
int klee_entry(int utbot_argc, char **utbot_argv, char **utbot_envp) {
  struct StructComplex par;
  klee_make_symbolic(&par, sizeof(par), "par");
  klee_prefer_cex(&par, par.x >= -10 & par.x <= 10);
  klee_prefer_cex(&par, par.smp.a >= -10 & par.smp.a <= 10);
  ////////////////////////////////////////////
  int utbot_result;
  klee_make_symbolic(&utbot_result, sizeof(utbot_result), "utbot_result");
  int utbot_tmp = sumStructComplex(par);
  klee_assume(utbot_tmp == utbot_result);
  return 0;
}