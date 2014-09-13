// RUN: %llvmgcc %s -emit-llvm -g -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --exit-on-error %t1.bc

#include <assert.h>

int main(int argc, char **argv, char **envp) {
  unsigned i;
  assert(argv[argc] == 0);
  printf("argc: %d, argv: %p, envp: %p\n", argc, argv, envp);
  printf("--environ--\n");
  int haspwd = 0;
  for (i=0; envp[i]; i++) {
    printf("%d: %s\n", i, envp[i]);
    haspwd |= strncmp(envp[i], "PWD=", 4)==0;
  }
  assert(haspwd);
  return 0;
}
