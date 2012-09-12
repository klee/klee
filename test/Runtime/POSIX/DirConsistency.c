// RUN: %llvmgcc %s -emit-llvm -O0 -c -o %t.bc
// RUN: %klee --run-in=/tmp --search=random-state --libc=uclibc --posix-runtime --exit-on-error %t.bc --sym-files 1 1 > %t1.log
// RUN: %llvmgcc -D_FILE_OFFSET_BITS=64 %s -emit-llvm -O0 -c -o %t.bc
// RUN: %klee --run-in=/tmp --search=random-state --libc=uclibc --posix-runtime --exit-on-error %t.bc --sym-files 1 1 > %t2.log
// RUN: sort %t1.log %t2.log | uniq -c > %t3.log
// RUN: grep -q "4 COUNT" %t3.log

// For this test really to work as intended it needs to be run in a
// directory large enough to cause uclibc to do multiple getdents
// calls (otherwise uclibc will handle the seeks itself). We should
// create a bunch of files or something.
//
// It is even more important for this test because it requires the
// directory not to change while running, which might be a lot to
// demand of /tmp.

#define _LARGEFILE64_SOURCE
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>

int main(int argc, char **argv) {
  struct stat s;
  int res = stat("A", &s);
  int hasA = !(res!=0 && errno==ENOENT);

  //printf("sizeof(dirent) = %d\n", sizeof(struct dirent));
  //printf("sizeof(dirent64) = %d\n", sizeof(struct dirent64));

  //printf("\"A\" exists: %d\n", hasA);

  DIR *d = opendir(".");
  assert(d);

  int snum = 1;
  if (klee_range(0,2,"range")) {
    snum = 2;
    printf("state%d\n", snum);
  }

  int foundA = 0, count = 0;
  struct dirent *de;
  while ((de = readdir(d))) {
    //    printf("state%d: dirent: %s\n", snum, de->d_name);
    if (strcmp(de->d_name, "A") == 0)
      foundA = 1;
    count++;
  }
  
  closedir(d);

  //printf("found A: %d\n", foundA);

  // Ensure atomic write
  char buf[64];
  sprintf(buf, "COUNT: %d\n", count);
  fputs(buf, stdout);
  assert(hasA == foundA);

  return 0;
}
