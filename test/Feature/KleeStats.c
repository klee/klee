// RUN: %clang %s -emit-llvm -g %O0opt -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out  %t.bc 2> %t.log
// RUN: klee-stats --print-more %t.klee-out > %t.stats
// RUN: FileCheck -check-prefix=CHECK-STATS -input-file=%t.stats %s
#include "klee/klee.h"
#include <stdlib.h>
int main(){
  int a;
  klee_make_symbolic (&a, sizeof(int), "a");
  if (a) {
    abort();
  }
  return 0;
}
// First check we find a line with the expected format
// CHECK-STATS: | Path | Instrs| Time(s)| ICov(%)| BCov(%)| ICount| TSolver(%)|
//Check there is a line with .klee-out dir, non zero instruction, less than 1 second execution time and 100 ICov.
// CHECK-STATS: {{.*\.klee-out\|[ ]*[1-9]+\|[ ]*0\.([0-9]+)\|[ ]*100\.00}}
