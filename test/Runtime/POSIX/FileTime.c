// Tests the functionality of setting and getting file access and modification times
// RUN: %clang %s -emit-llvm %O0opt -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --libc=uclibc --posix-runtime %t1.bc --sym-files 1 1

#include <stdio.h>
#include <assert.h>
#include <sys/stat.h>
#include <sys/time.h>

const char filePath[] = "A";

int main(int argc, char** argv) {

  // Create the file
  FILE* const f = fopen(filePath, "w");
  assert(f);
  const int r = fclose(f);
  assert(r == 0);

  struct timeval now;
  gettimeofday(&now, NULL);

  struct timeval times[2] = {now, now};
  times[1].tv_sec += 100; // Introduce difference between access time and modification time
  utimes(filePath, times);

  struct stat sb;
  stat(filePath, &sb);

  assert(sb.st_atim.tv_sec == times[0].tv_sec);
  assert(sb.st_mtim.tv_sec == times[1].tv_sec);

  gettimeofday(&now, NULL);
  // Test with setting access time and modification time to current time
  utimes(filePath, NULL);

  struct timeval someTimeAfter;
  gettimeofday(&someTimeAfter, NULL);

  stat(filePath, &sb);

  assert(sb.st_atim.tv_sec >= now.tv_sec && sb.st_atim.tv_sec <= someTimeAfter.tv_sec);
  assert(sb.st_mtim.tv_sec >= now.tv_sec && sb.st_mtim.tv_sec <= someTimeAfter.tv_sec);
  
  return 0;
}
