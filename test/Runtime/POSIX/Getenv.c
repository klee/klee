// RUN: %clang %s -emit-llvm %O0opt -c -o %t2.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --libc=klee --posix-runtime --exit-on-error %t2.bc --sym-files 1 10

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
