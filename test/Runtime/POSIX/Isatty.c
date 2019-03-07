// RUN: %clang %s -emit-llvm %O0opt -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --libc=uclibc --posix-runtime %t.bc --sym-stdin 10 --sym-stdout > %t.log 2>&1
// RUN: test -f %t.klee-out/test000001.ktest
// RUN: test -f %t.klee-out/test000002.ktest
// RUN: test -f %t.klee-out/test000003.ktest
// RUN: test -f %t.klee-out/test000004.ktest
// RUN: FileCheck --input-file=%t.log %s

// Depending on how uClibc is compiled (i.e. without -DKLEE_SYM_PRINTF)
// fprintf prints out on stdout even if stderr is provided.
#include <assert.h>
#include <stdio.h>
#include <unistd.h>

int main(int argc, char **argv) {
  int fd0 = 0; // stdin
  int fd1 = 1; // stdout

  int r = isatty(fd0);
  // CHECK-DAG: stdin is a tty
  // CHECK-DAG: stdin is NOT a tty
  if (r)
    fprintf(stderr, "stdin is a tty\n");
  else
    fprintf(stderr, "stdin is NOT a tty\n");

  r = isatty(fd1);
  // CHECK-DAG: stdout is a tty
  // CHECK-DAG: stdout is NOT a tty
  if (r)
    fprintf(stderr, "stdout is a tty\n");
  else
    fprintf(stderr, "stdout is NOT a tty\n");

  return 0;
}
