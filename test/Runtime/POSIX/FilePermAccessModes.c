// RUN: %clang %s -emit-llvm %O0opt -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --exit-on-error --posix-runtime %t.bc --sym-files 1 10

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

void assert_success(const char *path, int flags) {
  int fd = open(path, flags);
  assert(fd != -1);
  assert(close(fd) == 0);
}

void assert_eacces(const char *path, int flags) {
  errno = 0;
  int fd = open(path, flags);
  assert(fd == -1);
  assert(errno == EACCES);
}

int main(int argc, char **argv) {
  chmod("A", 0200);
  assert_eacces("A", O_RDONLY);
  assert_success("A", O_WRONLY);
  assert_eacces("A", O_RDWR);

  chmod("A", 0404);
  assert_success("A", O_RDONLY);
  assert_eacces("A", O_WRONLY);
  assert_eacces("A", O_RDWR);

  chmod("A", 0660);
  assert_success("A", O_RDONLY);
  assert_success("A", O_WRONLY);
  assert_success("A", O_RDWR);
}
