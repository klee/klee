// RUN: %clang %s -emit-llvm %O0opt -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --libc=uclibc --posix-runtime %t1.bc --max-fail 1 | FileCheck %s

#include <stdio.h>
#include <assert.h>

int main(int argc, char** argv) {
  char buf[1024];  
  FILE* f = fopen("/etc/mtab", "r");
  assert(f);
    
  int r = fread(buf, 1, 100, f);
  printf("fread(): %s\n", 
         r ? "ok" : "fail");
  // CHECK-DAG: fread(): ok
  // CHECK-DAG: fread(): fail

  r = fclose(f);
  printf("fclose(): %s\n", 
         r ? "ok" : "fail");
  // CHECK-DAG: fclose(): ok
  // CHECK-DAG: fclose(): fail

  return 0;
}
