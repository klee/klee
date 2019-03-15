// RUN: %clang %s -emit-llvm -g %O0opt -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --libc=uclibc --posix-runtime %t.bc --sym-files 1 10  2>%t.log
// RUN: rm -rf %t.klee-out-2
// RUN: %klee --output-dir=%t.klee-out-2 --seed-dir=%t.klee-out --zero-seed-extension --libc=uclibc --posix-runtime %t.bc --sym-files 1 10 --max-fail 1
// RUN: ls %t.klee-out-2 | grep -c assert | grep 4

#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


int main(int argc, char** argv) {
  char buf[32];
  
  int fd = open("A", O_RDWR | O_CREAT, S_IRWXU);
  assert(fd != -1);
  int nbytes = write(fd, "Hello", sizeof("Hello"));
  assert(nbytes == sizeof("Hello"));

  off_t off = lseek(fd, 0, SEEK_SET);
  assert(off != (off_t) -1);

  nbytes = read(fd, buf, sizeof("Hello"));
  assert(nbytes == sizeof("Hello"));
  
  int r = close(fd);
  assert(r == 0);

  r = memcmp(buf, "Hello", sizeof("Hello"));
  assert(r == 0);

  return 0;
}
