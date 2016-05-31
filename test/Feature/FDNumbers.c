// RUN: %llvmgcc %s -emit-llvm -O0 -c -o %t2.bc
// RUN: touch A
// RUN: %klee --exit-on-error --reserve-fds %t2.bc
// XFAIL: *

#include <assert.h>
#include <fcntl.h>
#include <errno.h>

int main(int argc, char **argv) {
  int fd = open("A", O_TRUNC);
  assert(fd == 3);
  assert(!close(0));
  assert(!close(1));
  assert(close(0) == -1);
  assert(close(1) == -1);
  assert(open("A", O_TRUNC) == 0);
  assert(dup(0) == 1);
  assert(open("A", O_TRUNC) == 4);
  assert(!close(1));
  assert(open("A", O_TRUNC) == 1);
  assert(dup(0) != 1);
  assert(dup2(0, 1) == 1);

  return 0;
}
