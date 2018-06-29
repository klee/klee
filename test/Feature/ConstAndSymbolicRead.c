// RUN: %llvmgcc %s -g -emit-llvm -O0 -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out -use-query-log=all:kquery %t1.bc  
//FileCheck %s


int main() {
  char  arr[3];
  char symbolic;
  klee_make_symbolic(&symbolic, sizeof(symbolic), "symbolic");
  klee_assume(symbolic >= 0 & symbolic < 3);
  klee_make_symbolic(arr, sizeof(arr), "arr");
  arr[1] = 0;

  char a = arr[2];
  char b = arr[symbolic];
  if(a == b) printf("Equal!\n");
  else printf("Not Equal!\n");
  return 0;
}
