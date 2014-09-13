// RUN: %llvmgcc %s -emit-llvm -O0 -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --libc=uclibc --posix-runtime --exit-on-error %t.bc --sym-files 0 4 > %t.log
// RUN: grep "mode:file" %t.log
// RUN: grep "mode:dir" %t.log
// RUN: grep "mode:chr" %t.log
// RUN: grep "mode:blk" %t.log
// RUN: grep "mode:fifo" %t.log
// RUN: grep "mode:lnk" %t.log
// RUN: grep "read:sym:yes" %t.log
// RUN: grep "read:sym:no" %t.log

#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <assert.h>

int main(int argc, char **argv) {
  struct stat stats;
  assert(fstat(0, &stats) == 0);

  if (S_ISREG(stats.st_mode)) {
    printf("mode:file\n");
  } else if (S_ISDIR(stats.st_mode)) {
    printf("mode:dir\n");
  } else if (S_ISCHR(stats.st_mode)) {
    printf("mode:chr\n");
  } else if (S_ISBLK(stats.st_mode)) {
    printf("mode:blk\n");
  } else if (S_ISFIFO(stats.st_mode)) {
    printf("mode:fifo\n");
  } else if (S_ISLNK(stats.st_mode)) {
    printf("mode:lnk\n");
  } else if (S_ISSOCK(stats.st_mode)) {
    printf("mode:sock\n");
  } else {
    printf("unknown mode\n");
  }

  assert(stats.st_size==4);

  if (S_ISREG(stats.st_mode)) {
    char buf[10];
    int n = read(0, buf, 5);
    assert(n == 4);
    
    if (strcmp(buf, "HI!")) {
      printf("read:sym:yes\n");
    } else {
      printf("read:sym:no\n");
    }
  }

  fflush(stdout);

  return 0;
}
