// RUN: %clang %s -emit-llvm %O0opt -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --exit-on-error --posix-runtime --libc=uclibc %t.bc >%t.log

/* This test checks that main() with 3 arguments is supported, and that the data in envp for $HOME is consistent with getenv("HOME") */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

int main(int argcPtr, char **argvPtr, char** envp) {

  char* home1 = getenv("HOME");
  printf("home1 = %s\n", home1);

  char* home2 = "invalid$home";
  while (*envp) {
    if (strncmp(*envp, "HOME", 4) == 0)
      home2 = *envp + 5;
    envp++;
  }
  printf("home2 = %s\n", home2);
  assert(strcmp(home1, home2) == 0);
  
  return 0;
}
