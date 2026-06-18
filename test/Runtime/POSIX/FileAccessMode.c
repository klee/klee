// RUN: %clang %s -emit-llvm %O0opt -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --exit-on-error --libc=uclibc --posix-runtime %t.bc --sym-files 1 10

#include "klee/klee.h"
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

int main(int argc, char **argv) {
  char buf[1];

  int rfd = open("A", O_RDONLY);
  if (rfd < 0)
    klee_silent_exit(0); // open will be denied on some symbolic paths so it doesn't count as a test failure
  assert(write(rfd, "x", 1) == -1 && errno == EBADF);
  close(rfd);

  int wfd = open("A", O_WRONLY);
  if (wfd < 0)
    klee_silent_exit(0); // file not writable on this path so we skip so it doesn't count as a test failure
  assert(read(wfd, buf, 1) == -1 && errno == EBADF);
  close(wfd);

  return 0;
}
