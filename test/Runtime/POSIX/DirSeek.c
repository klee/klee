// RUN: %clang %s -emit-llvm %O0opt -c -g -o %t2.bc
// RUN: rm -rf %t.klee-out %t.klee-out-tmp
// RUN: %gentmp %t.klee-out-tmp
// RUN: %klee --output-dir=%t.klee-out --run-in-dir=%t.klee-out-tmp --libc=uclibc --posix-runtime --exit-on-error %t2.bc --sym-files 2 2
// RUN: rm -rf %t.klee-out %t.klee-out-tmp
// RUN: %gentmp %t.klee-out-tmp
// RUN: %klee --output-dir=%t.klee-out --run-in-dir=%t.klee-out-tmp --libc=uclibc --posix-runtime --exit-on-error %t2.bc --sym-files 1 2
// RUN: rm -rf %t.klee-out %t.klee-out-tmp
// RUN: %gentmp %t.klee-out-tmp
// RUN: %klee --output-dir=%t.klee-out --run-in-dir=%t.klee-out-tmp --libc=uclibc --posix-runtime --exit-on-error %t2.bc --sym-files 1 2 --sym-stdin 2

// For this test really to work as intended it needs to be run in a
// directory large enough to cause uclibc to do multiple getdents
// calls (otherwise uclibc will handle the seeks itself).
// Therefore gentmp generates a directory with a specific amount of entries

#include <assert.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <string.h>

int main(int argc, char **argv) {
  char first[256], second[256];
  DIR *d = opendir(".");
  struct dirent *de = readdir(d);
  assert(de);
  strcpy(first, de->d_name);
  off_t pos = telldir(d);
  de = readdir(d);
  assert(de);
  strcpy(second, de->d_name);

  // Go back to second and check
  seekdir(d, pos);
  de = readdir(d);
  assert(de);
  assert(strcmp(de->d_name, second) == 0);

  // Go to end, then back to 2nd
  while (de) {
    de = readdir(d);
    assert(!errno);
  }
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
