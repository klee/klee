// -- Core testing commands
// RUN: rm -f %t.bout
// RUN: %gen-random-bout 100 -sym-arg 4 -sym-files 2 20 -sym-arg 5 -sym-stdin 8 -sym-stdout -sym-arg 6 -sym-args 1 4 5 -bout-file %t.bout
// RUN: %cc %s -O0 -o %t
// RUN: %klee-replay %t %t.bout 2> %t.out
// RUN: FileCheck --input-file=%t.out %s
// CHECK: KLEE-REPLAY: NOTE: EXIT STATUS: NORMAL
//
// -- Option error handling tests
// RUN: not %gen-random-bout 2> %t1
// RUN: FileCheck -check-prefix=CHECK-USAGE -input-file=%t1 %s
// CHECK-USAGE: Usage
//
// RUN: not %gen-random-bout 0 --unexpected-option 2> %t2
// RUN: FileCheck -check-prefix=CHECK-UNEXPECTED -input-file=%t2 %s
// CHECK-UNEXPECTED: Unexpected
//
// RUN: not %gen-random-bout 100 --sym-args 5 3 2> %t3
// RUN: FileCheck -check-prefix=CHECK-RANOUT -input-file=%t3 %s
// CHECK-RANOUT: ran out of
//
// RUN: not %gen-random-bout 100 --sym-args 5 3 10 2> %t4
// RUN: FileCheck -check-prefix=CHECK-NOMORE -input-file=%t4 %s
// CHECK-NOMORE: should be no more

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

  puts("All size tests passed");

  return 0;
}
