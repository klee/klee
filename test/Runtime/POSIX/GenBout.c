// -- Core testing commands
// RUN: rm -rf %t.out
// RUN: rm -f %t.bout
// RUN: mkdir -p %t.out
// RUN: echo -n aaaa > %t.out/aaaa.txt
// RUN: echo -n bbbb > %t.out/bbbb.txt
// RUN: echo -n cccc > %t.out/cccc.txt
// RUN: %gen-bout -o -p -q file1 --bout-file %t.bout --sym-stdin %t.out/aaaa.txt --sym-file %t.out/bbbb.txt --sym-stdout %t.out/cccc.txt
// RUN: %cc %s -O0 -o %t
// RUN: %klee-replay %t %t.bout 2> %t.out/out.txt
// RUN: FileCheck --input-file=%t.out/out.txt %s

// CHECK: KLEE-REPLAY: NOTE: EXIT STATUS: NORMAL

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int check_fd(int fd, const int file_size) {
  struct stat fs;

  if (fstat(fd, &fs) < 0)
    return -1;

  if (fs.st_size != file_size)
    return -1;

  return 0;
}

int check_file(const char *file_name, const int file_size) {
  int fd;

  if ((fd = open(file_name, O_RDONLY)) < 0)
    return -1;

  if (check_fd(fd, file_size) < 0)
    return -1;

  close(fd);
  return 0;
}

int main(int argc, char **argv) {
  if (argc != 5)
    return 1;

  if (strcmp(argv[1], "-o"))
    return 1;

  if (strcmp(argv[2], "-p"))
    return 1;

  if (strcmp(argv[3], "-q"))
    return 1;

  if (strcmp(argv[4], "file1"))
    return 1;

  if (check_file("A", 4))
    return 1;

  if (check_fd(0, 4))
    return 1;

  if (check_fd(1, 1024))
    return 1;

  printf("tests passed\n");

  return 0;
}

