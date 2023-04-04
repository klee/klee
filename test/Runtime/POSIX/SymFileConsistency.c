// REQUIRES: posix-runtime
// RUN: %clang %s -emit-llvm %O0opt -c -g -o %t.bc
// RUN: rm -rf %t.klee-out-tmp
// RUN: %klee --output-dir=%t.klee-out-tmp --libc=uclibc --posix-runtime --exit-on-error %t.bc --sym-files 1 1 > %t1.log

// This test checks that symbolic files can be resolved both with a relative path
// ie. 'A' or by its full path ie. '/full/path/to/cwd/A'

#include "klee/klee.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

int main(int argc, char **argv) {
  struct stat s, s1, s2;
  int res = stat("A", &s);
  char cwd[1024];
  getcwd(cwd, 1024);
  char *smallest_cwd = malloc(strlen(cwd) + 1);
  strcpy(smallest_cwd, cwd);
  stat(smallest_cwd, &s2);
  free(smallest_cwd);

  char fullName[1024];
  snprintf(fullName, 1024, "%s/%s", cwd, "A");

  res = stat(fullName, &s1);
  assert(s.st_size == s1.st_size);

  return 0;
}
