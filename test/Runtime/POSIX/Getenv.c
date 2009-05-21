// RUN: %llvmgcc %s -emit-llvm -O0 -c -o %t2.bc
// RUN: %klee --init-env --libc=klee --posix-runtime --exit-on-error %t2.bc --sym-files 1 10

#include <assert.h>

int main(int argc, char **argv) {
  char *g = getenv("PWD");
  if (g) {
    printf("have PWD\n");
  } else {
    printf("have no PWD\n");
  }

  g = getenv("HELLO");
  if (!g || strcmp(g, "nice")==0) {
    printf("getenv(\"HELLO\") = %p\n", g);
    if (g) assert(strcmp(getenv("HELLO"),"nice") == 0);
  }

  return 0;
}
