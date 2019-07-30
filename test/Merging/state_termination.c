// RUN: %clang -emit-llvm -g -c -o %t.bc %s
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --use-merge --debug-log-merge --search=dfs  %t.bc 

#include "klee/klee.h"

int main(int argc, char** args){

  int x;

  char str[5];
  klee_make_symbolic(str, sizeof(str), "str");
  char *s = str;

  klee_open_merge();
  while(*s != 's')
      s++;
  klee_close_merge();

  return 0;
}
