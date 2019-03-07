// RUN: %clang %s -emit-llvm %O0opt -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --libc=uclibc --posix-runtime --search=dfs %t1.bc --sym-files 1 10 --max-fail 2
//
// Check that generated assembly doesn't use puts to output strings
// RUN: FileCheck -input-file=%t.klee-out/assembly.ll %s
// CHECK-NOT: call i32 @puts(
//
// RUN: test -f %t.klee-out/test000001.ktest
// RUN: test -f %t.klee-out/test000002.ktest
// RUN: test -f %t.klee-out/test000003.ktest
// RUN: test -f %t.klee-out/test000004.ktest
// RUN: not test -f %t.klee-out/test000005.ktest

#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char** argv) {
  char buf[1024];
  // Open the symbolic file "A"
  int fd = open("A", O_RDONLY);
  assert(fd != -1);

  int r;

  r = read(fd, buf, 5);
  if (r != -1)
    printf("read() succeeded %u\n", fd);
  else printf("read() failed with error '%s'\n", strerror(errno));

  r = close(fd);
  if (r != -1)
    printf("close() succeeded %u\n", fd);
  else printf("close() failed with error '%s'\n", strerror(errno));

  return 0;
}
