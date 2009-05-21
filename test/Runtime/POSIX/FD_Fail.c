// RUN: %llvmgcc %s -emit-llvm -O0 -c -o %t1.bc
// RUN: %klee --libc=uclibc --posix-runtime --init-env %t1.bc --sym-files 0 0 --max-fail 1 > %t.log
// RUN: grep -q "fread(): ok" %t.log
// RUN: grep -q "fread(): fail" %t.log
// RUN: grep -q "fclose(): ok" %t.log
// RUN: grep -q "fclose(): fail" %t.log

#include <stdio.h>
#include <assert.h>

int main(int argc, char** argv) {
  char buf[1024];  
  FILE* f = fopen("/etc/fstab", "r");
  assert(f);
    
  int r = fread(buf, 1, 100, f);
  printf("fread(): %s\n", 
         r ? "ok" : "fail");

  r = fclose(f);
  printf("fclose(): %s\n", 
         r ? "ok" : "fail");

  return 0;
}
