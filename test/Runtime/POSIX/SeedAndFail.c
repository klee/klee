// RUN: %llvmgcc %s -emit-llvm -g -O0 -c -o %t.bc
// RUN: rm -rf tmp-123
// RUN: %klee --libc=klee --output-dir=tmp-123 --posix-runtime --init-env %t.bc --sym-files 1 10  2>%t.log
// RUN: %klee --seed-out-dir=tmp-123 --zero-seed-extension --libc=uclibc --posix-runtime --init-env %t.bc --sym-files 1 10 --max-fail 1
// RUN: ls klee-last | grep -c assert | grep 4



#include <stdio.h>
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
