// RUN: %llvmgcc %s -emit-llvm -O0 -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --posix-runtime %t.bc --sym-files 1 10 --sym-stdout 2>%t.log 
// RUN: test -f %t.klee-out/test000001.ktest
// RUN: test -f %t.klee-out/test000002.ktest
// RUN: test -f %t.klee-out/test000003.ktest

#include <stdio.h>       
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int main(int argc, char** argv) {
  int fd = open("A", O_RDWR);
  if (fd != -1)
    fprintf(stderr, "File 'A' opened successfully\n");
  else fprintf(stderr, "Cannot open file 'A'\n");

  if (fd != -1)
    close(fd);
}
