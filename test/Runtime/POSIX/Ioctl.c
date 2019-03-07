// RUN: %clang %s -emit-llvm %O0opt -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --posix-runtime --exit-on-error %t.bc --sym-stdin 4

#include <assert.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <errno.h>
#include <stdio.h>

int main() {
  struct stat s;
  struct termios ts;

  assert(fstat(0, &s) == 0);

  assert(ioctl(10, FIONREAD, &ts) == -1 && errno == EBADF);

  if (S_ISCHR(s.st_mode)) {
    printf("is chr\n");
    assert(ioctl(0, TIOCGWINSZ, &ts) == 0);
  } else {
    printf("not chr\n");
    // I think TC* ioctls basically always fail on nonchr?
    assert(ioctl(0, TIOCGWINSZ, &ts) == -1);
    assert(errno == ENOTTY);
  }

  return 0;
}
