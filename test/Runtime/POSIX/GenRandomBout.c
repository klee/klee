// -- Core testing commands
// RUN: rm -rf %t.out
// RUN: mkdir -p %t.out && cd %t.out
// RUN: %gen-random-bout 100 -sym-arg 4 -sym-files 2 20 -sym-arg 5 -sym-stdin 8 -sym-stdout -sym-arg 6 -sym-args 1 4 5
// RUN: %cc %s -O0 -o %t
// RUN: %klee-replay %t file.bout 2>&1 | grep "klee-replay: EXIT STATUS: NORMAL"
//
// -- Option error handling tests
// RUN: bash -c '%gen-random-bout || :' 2>&1 | grep "Usage"
// RUN: bash -c '%gen-random-bout 0 --unexpected-option || :' 2>&1 | grep "Unexpected"
// RUN: bash -c '%gen-random-bout 100 --sym-args 5 3 || :' 2>&1 | grep "ran out of"
// RUN: bash -c '%gen-random-bout 100 --sym-args 5 3 10 || :' 2>&1 | grep "should be no more"

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

  if (argc < 4 || argc > 7)
    return 1;

  if (strlen(argv[1]) > 4)
    return 1;

  if (strlen(argv[2]) > 5)
    return 1;

  if (strlen(argv[3]) > 6)
    return 1;

  for (i = 4; i < argc; ++i)
    if (strlen(argv[i]) > 5)
      return 1;

  if (check_file("A", 20) < 0)
    return 1;

  if (check_file("B", 20) < 0)
    return 1;

  if (check_fd(0, 8) < 0)
    return 1;

  if (check_fd(1, 1024) < 0)
    return 1;

  printf("All size tests passed\n");

  return 0;
}
