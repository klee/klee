/* This test checks that when opening a symbolic file in R/W mode, we return exactly twice: 
   once successfully, and the other time with a permission error */

// RUN: %clang %s -emit-llvm %O0opt -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --posix-runtime %t.bc --sym-files 1 10 | FileCheck %s
// RUN: test -f %t.klee-out/test000001.ktest
// RUN: test -f %t.klee-out/test000002.ktest
// RUN: not test -f %t.klee-out/test000003.ktest

#include <stdio.h>       
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

int main(int argc, char** argv) {
  int fd = open("A", O_RDWR);
  if (fd != -1)
    printf("File 'A' opened successfully\n");
  // CHECK-DAG: File 'A' opened successfully
  else printf("Cannot open file 'A'\n");
  // CHECK-DAG: Cannot open file 'A'
  if (fd != -1)
    close(fd);
}
