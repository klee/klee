// RUN: %llvmgcc %s -emit-llvm -O0 -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --exit-on-error --libc=uclibc --posix-runtime %t.bc --sym-files 1 10 --sym-stdout 2>%t.log
//
// Disable this test on LLVM 3.4 for now, it seems to hang indefinitely when run
// in our Travis CI config.
//
// FIXME: Investigate.
// REQUIRES: not-llvm-3.4

#include <stdio.h>
#include <assert.h>

int main(int argc, char** argv) {
  const char* msg = "This will eventually overflow stdout. ";
  char buf[32];
  int i;
  
  FILE* f = stdout;//fopen("A", "w");
  if (!f)
    klee_silent_exit(0);

  for (i = 0; i < 300; i++)
    fwrite(msg, sizeof(msg), 1, f);
  fclose(f);

  return 0;
}
