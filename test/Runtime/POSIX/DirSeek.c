// RUN: %llvmgcc %s -emit-llvm -O0 -c -o %t2.bc
// RUN: %klee --run-in=/tmp --init-env --libc=uclibc --posix-runtime --exit-on-error %t2.bc --sym-files 2 2
// RUN: %klee --run-in=/tmp --init-env --libc=uclibc --posix-runtime --exit-on-error %t2.bc --sym-files 1 2
// RUN: %klee --run-in=/tmp --init-env --libc=uclibc --posix-runtime --exit-on-error %t2.bc --sym-files 0 2

// For this test really to work as intended it needs to be run in a
// directory large enough to cause uclibc to do multiple getdents
// calls (otherwise uclibc will handle the seeks itself). We should
// create a bunch of files or something.

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>

int main(int argc, char **argv) {
  struct stat s;

  char first[256], second[256];
  DIR *d = opendir(".");
  struct dirent *de = readdir(d);
  assert(de);
  strcpy(first, de->d_name);
  off_t pos = telldir(d);
  printf("pos: %d\n", telldir(d));
  de = readdir(d);
  assert(de);
  strcpy(second, de->d_name);

  // Go back to second and check
  seekdir(d, pos);
  de = readdir(d);
  assert(de);
  assert(strcmp(de->d_name, second) == 0);

  // Go to end, then back to 2nd
  while (de)
    de = readdir(d);
  assert(!errno);
  seekdir(d, pos);
  assert(telldir(d) == pos);
  de = readdir(d);
  assert(de);
  assert(strcmp(de->d_name, second) == 0);

  // Go to beginning and check
  rewinddir(d);
  de = readdir(d);
  assert(de);
  assert(strcmp(de->d_name, first) == 0);  
  closedir(d);

  return 0;
}
