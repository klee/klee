// REQUIRES: not-wsl-1
// Disabling WSL 1 because futimesat syscall may be not implemented
//
// Without the -std=c99 option, linux is an implicitly defined macro on Linux
// systems so all such substrings in the given path will be replaced with 1
// during stringification.  Unfortunately, Fedora's rpmbuild uses
// x86_64-redhat-linux-gnu as a build directory for all CMake projects which
// leads to a test failure due to an unwanted change to x86_64-redhat-1-gnu.
//
// RUN: %clang %s -std=c99 -emit-llvm %O0opt -g -c -DTDIR=%T -o %t2.bc
// RUN: touch %T/futimesat-dummy
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --posix-runtime --exit-on-error %t2.bc --sym-files 1 10

// According to https://man7.org/linux/man-pages/man2/futimesat.2.html
// _GNU_SOURCE should be defined for glibc.
#define _GNU_SOURCE

#include <assert.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#define xstr(s) str(s)
#define str(s) #s

int main(int argc, char **argv) {
  int r;
  struct stat buf;
  struct timeval times[2];

  times[0].tv_sec = time(NULL)-3600;
  times[0].tv_usec = 0;
  times[1].tv_sec = time(NULL)-3600;
  times[1].tv_usec = 0;

  r = futimesat(AT_FDCWD, "A", times);
  assert(r != -1);

  r = fstatat(AT_FDCWD, "A", &buf, 0);
  assert(r != -1);
  assert(buf.st_atime == times[0].tv_sec &&
         buf.st_mtime == times[1].tv_sec);

  /* assumes TDIR exists and is writeable */
  int fd = open( xstr(TDIR) , O_RDONLY);
  assert(fd > 0);
  r = futimesat(fd, "futimesat-dummy", times);
  assert(r != -1);

  r = fstatat(fd, "futimesat-dummy", &buf, 0);
  assert(r != -1);
  assert(buf.st_atime == times[0].tv_sec &&
         buf.st_mtime == times[1].tv_sec);

  close(fd);

  return 0;
}
