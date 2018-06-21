// RUN: rm -rf file.bout
// RUN: echo -n aaaa > %t.aaaa.txt
// RUN: echo -n bbbb > %t.bbbb.txt
// RUN: echo -n cccc > %t.cccc.txt
// RUN: %gen-bout -o -p -q file1 --sym-stdin %t.aaaa.txt --sym-file %t.bbbb.txt --sym-stdout %t.cccc.txt
// RUN: %cc %s -O0 -o %t
// RUN: %klee-replay %t file.bout 2>&1 | grep "klee-replay: EXIT STATUS: NORMAL"

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
  int i = 0;

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

