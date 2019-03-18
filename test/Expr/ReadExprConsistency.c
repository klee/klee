// RUN: %clang %s -emit-llvm -g -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --exit-on-error %t.bc 2>%t.log
// RUN: cat %t.log | FileCheck %s

#include <assert.h>

/*
This tests checks ensures that only relevant updates are present when doing
concrete reads. If they are not, there can be situations where ReadExpr are
in inconcistent state and depend on ordering of other operations.

See
https://github.com/klee/klee/issues/921
https://github.com/klee/klee/pull/1061
*/

void klee_print_expr(const char*, char);
int main() {
	char  arr[3];
  char symbolic;
  klee_make_symbolic(&symbolic, sizeof(symbolic), "symbolic");
  klee_assume(symbolic >= 0 & symbolic < 3); 
  klee_make_symbolic(arr, sizeof(arr), "arr");
  
  
  char a = arr[2];  // (ReadExpr 2 arr)
  //CHECK: arr[2]:(Read w8 2 arr)
  klee_print_expr("arr[2]", arr[2]);
  arr[1] = 0;
  char b = arr[symbolic];  // (ReadExpr symbolic [1=0]@arr)
  //CHECK: arr[2]:(Read w8 2 arr)
  //CHECK-NOT: arr[2]:(Read w8 2 [1=0]@arr)
  klee_print_expr("arr[2]", arr[2]);
  
  if(a == b) printf("Equal!\n");
  else printf("Not Equal!\n");
  return 0;
}
