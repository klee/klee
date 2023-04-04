// -- Core testing commands
// REQUIRES: uclibc
// REQUIRES: posix-runtime
// RUN: rm -rf %t.out
// RUN: rm -f %t.bc
// RUN: mkdir -p %t.out
// RUN: echo -n aaaa > %t.out/aaaa.txt
// RUN: echo -n bbbb > %t.out/bbbb.txt
// RUN: echo -n ccc > %t.out/cccc.txt
// RUN: %clang %s -emit-llvm %O0opt -c -o %t.bc
// RUN: %klee-zesti -only-replay-seeds -libc=uclibc -posix-runtime %t.bc -o -p -q %t.out/bbbb.txt %t.out/cccc.txt < %t.out/aaaa.txt  &> %t.out/out.txt
// RUN: FileCheck --input-file=%t.out/out.txt %s

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

int check_fd(int fd, const int file_size, const char *file_contents) {
  struct stat fs;
  char contents[10] = {0};

  if (fstat(fd, &fs) < 0)
    return -1;

  if (fs.st_size != file_size)
    return -1;

  read(fd, contents, 10);

  if (strcmp(contents, file_contents) != 0)
    return -1;

  return 0;
}

int check_file(const char *file_name, const int file_size, const char *file_contents) {
  int fd;

  if ((fd = open(file_name, O_RDONLY)) < 0)
    return -1;

  if (check_fd(fd, file_size, file_contents) < 0)
    return -1;

  close(fd);
  return 0;
}

int main(int argc, char **argv) {
  if (argc == 6) {
    // CHECK-DAG: Got 5 args
    printf("Got 5 args\n");
  }

  if (strcmp(argv[1], "-o") == 0) {
    // CHECK-DAG: Got -o flag
    printf("Got -o flag\n");
  }

  if (strcmp(argv[2], "-p") == 0) {
    // CHECK-DAG: Got -p flag
    printf("Got -p flag\n");
  }

  if (strcmp(argv[3], "-q") == 0) {
    // CHECK-DAG: Got -q flag
    printf("Got -q flag\n");
  }

  if (strcmp(argv[4], "A") == 0) {
    // CHECK-DAG: Got A file
    printf("Got A file\n");
  }

  if (check_file(argv[4], 4, "bbbb") == 0) {
    // CHECK-DAG: Got A file size
    printf("Got A file size\n");
  }

  if (strcmp(argv[5], "B") == 0) {
    // CHECK-DAG: Got B file
    printf("Got B file\n");
  }

  // File sizes get increased to the highest among files, so even B has file size 4.
  // This is due to the limitation of posix-runtime API
  if (check_file(argv[5], 4, "ccc") == 0) {
    // CHECK-DAG: Got B file size
    printf("Got B file size\n");
  }

  if (check_fd(0, 4, "aaaa") == 0) {
    // CHECK-DAG: Got stdin file size
    printf("Got stdin file size\n");
  }
  // CHECK-DAG: KLEE: done: completed paths = 1
  // CHECK-DAG: KLEE: done: generated tests = 1

  return 0;
}
