// REQUIRES: not-darwin
// RUN: %clang %s -Wall -emit-llvm -g %O0opt -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --libc=uclibc --posix-runtime --exit-on-error %t.bc

#define _GNU_SOURCE
#include <limits.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char * argv[]) {
  char cwd[PATH_MAX] = {0};

  if (!getcwd(cwd, PATH_MAX)) exit(EXIT_FAILURE);
  if (!canonicalize_file_name(cwd)) exit(EXIT_FAILURE);
}
