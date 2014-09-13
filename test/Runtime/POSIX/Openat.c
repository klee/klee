// RUN: %llvmgcc %s -emit-llvm -O0 -c -o %t2.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --posix-runtime --exit-on-error %t2.bc --sym-files 1 10
// RUN: test -f %t.klee-out/test000001.ktest

#include <assert.h>
#include <fcntl.h>

int main(int argc, char **argv) {
  int fd = openat(AT_FDCWD, "A", O_RDWR|O_TRUNC);
  if (fd != -1) {
    char buf[10];
    assert(read(fd, buf, 10) == 10);
    assert(klee_is_symbolic(buf[0]));
  } else {
    klee_silent_exit(0);
  }
  return 0;
}
