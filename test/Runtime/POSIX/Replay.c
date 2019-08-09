// RUN: %clang -DKLEE_EXECUTION %s -emit-llvm %O0opt -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --posix-runtime %t.bc --sym-files 1 3
// RUN: %klee-replay --create-files-only %t.klee-out/test000001.ktest

// RUN: %cc %s -O0 -o %t2
// RUN: %klee-replay %t2 %t.klee-out/test000001.ktest | FileCheck --check-prefix=REPLAY %s
// REPLAY: Yes

#ifdef KLEE_EXECUTION
#define EXIT klee_silent_exit
#else
#include <stdlib.h>
#define EXIT exit
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include <stdio.h>

int main(int argc, char** argv) {
  int fd, n;
  char buf[1024];
  
  fd = open("A", O_RDONLY);
  assert(fd != -1);
  n = read(fd, buf, 3);
  assert(n == 3);

  /* Generate a single test, with the first three chars 
     in the file 'abc' */
  if (buf[0] == 'a' && buf[1] == 'b' && buf[2] == 'c')
    printf("Yes\n");
  else
    EXIT(0);

  return 0;
}

    
