// RUN: %clang %s -emit-llvm %O0opt -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --libc=klee --posix-runtime %t.bc

#include <assert.h>
#include <errno.h>
#include <unistd.h>

int main(void) {
  // non-null path with 0 size should set EINVAL and return NULL
  char *path[42];
  errno = 0;
  char *result = getcwd(path, 0);
  assert(errno == EINVAL);
  assert(result == NULL);
}
