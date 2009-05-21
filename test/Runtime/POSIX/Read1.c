// RUN: %llvmgcc %s -emit-llvm -O0 -c -o %t.bc
// RUN: %klee --exit-on-error --posix-runtime --init-env %t.bc --sym-files 1 8 >%t.log

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <assert.h>

int main(int argc, char** argv) {
  char buf[32];

  // If count is zero, read() returns zero and has  no  other  results. (man page)
  int x = read(0, 0, 0);
  assert(x == 0);
  
  int fd = open("A", O_RDONLY);
  assert(fd != -1);

  // EFAULT buf is outside your accessible address space. (man page)
  x = read(fd, 0, 1);
  assert(x == -1 && errno == EFAULT);
 
  // EBADF  fd is not a valid file descriptor (man page)
  x = read(-1, buf, 1);
  assert(x == -1 && errno == EBADF);

  fd = open("A", O_RDONLY);
  assert(fd != -1);
  x = read(fd, buf, 1);
  assert(x == 1);  
}

