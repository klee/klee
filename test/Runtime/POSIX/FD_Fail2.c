// RUN: %llvmgcc %s -emit-llvm -O0 -c -o %t1.bc
// RUN: %klee --libc=uclibc --posix-runtime --search=dfs %t1.bc --sym-files 1 10 --max-fail 1
// RUN: test -f klee-last/test000001.ktest
// RUN: test -f klee-last/test000002.ktest
// RUN: test -f klee-last/test000003.ktest
// RUN: test -f klee-last/test000004.ktest
// RUN: test -f klee-last/test000005.ktest
// RUN: test -f klee-last/test000006.ktest
// RUN: test -f klee-last/test000007.ktest

#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>

int main(int argc, char** argv) {
  char buf[1024];  
  int fd = open("A", O_RDONLY);
  assert(fd != -1);

  int r;

  r = read(fd, buf, 1, 5);
  if (r != -1)
    printf("read() succeeded\n");
  else printf("read() failed with error '%s'\n", strerror(errno));

  r = close(fd);
  if (r != -1)
    printf("close() succeeded\n");
  else printf("close() failed with error '%s'\n", strerror(errno));

  return 0;
}
