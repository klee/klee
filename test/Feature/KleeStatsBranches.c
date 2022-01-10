// RUN: %clang %s -emit-llvm -g %O0opt -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --write-no-tests --output-dir=%t.klee-out %t.bc 2> %t.log
// RUN: %klee-stats --print-columns 'BrConditional,BrIndirect,BrSwitch,BrCall,BrMemOp,BrResolvePointer,BrAlloc,BrRealloc,BrFree,BrGetVal' --table-format=csv %t.klee-out > %t.stats
// RUN: FileCheck -check-prefix=CHECK-STATS -input-file=%t.stats %s

#include "klee/klee.h"

#include <stdlib.h>


void foo(void) {}
void bar(void) {}

int memop(void) {
  int *p;
  klee_make_symbolic(&p, sizeof(p), "p");
  return *p;
}


int main(void) {
  // alloc
  size_t size0;
  klee_make_symbolic(&size0, sizeof(size_t), "size0");
  klee_assume(size0 < 10);
  void *p;

  // realloc
  size_t size1;
  klee_make_symbolic(&size1, sizeof(size_t), "size1");
  klee_assume(size1 < 20);

  // conditional
  int cond;
  klee_make_symbolic(&cond, sizeof(cond), "cond");

  // switch
  int sw_cond;
  klee_make_symbolic(&sw_cond, sizeof(sw_cond), "sw_cond");

  // call
  void (*fptr)(void);
  klee_make_symbolic(&fptr, sizeof(fptr), "fptr");
  klee_assume((fptr == &foo) | (fptr == &bar));

  // indirectbr
  void *lptr;
  klee_make_symbolic(&lptr, sizeof(lptr), "lptr");
  klee_assume((lptr == &&one) | (lptr == &&two));
  goto *lptr;


one:
  p = malloc(size0);
  if (p) {
    p = realloc(p, size1);
    if (p) {
      // free
      klee_make_symbolic(&p, sizeof(p), "p");
      free(p);
    }
  }

  return 1;

two:
  switch (sw_cond) {
  case 8: memop(); break; // memop
  case 15: (*fptr)(); break;
  default: {
    int c = 42;
    // conditional
    if (cond) c++;
    return c;
  }
  }

  return 2;
}

// Check that we create branches
// CHECK-STATS: BrConditional,BrIndirect,BrSwitch,BrCall,BrMemOp,BrResolvePointer,BrAlloc,BrRealloc,BrFree,BrGetVal
// CHECK-STATS: 1,1,2,1,{{[1-9][0-9]*}},{{[1-9][0-9]*}},2,1,1,0
