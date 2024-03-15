// RUN: %clang %s -emit-llvm -g %O0opt -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out %t.bc 2> %t.log
// RUN: %klee-stats --print-more --table-format=csv %t.klee-out > %t.stats
// RUN: FileCheck -check-prefix=CHECK-STATS -input-file=%t.stats %s
// test --print-abs-times
// RUN: %klee-stats --print-abs-times --table-format=csv %t.klee-out > %t2.stats
// RUN: FileCheck -check-prefix=CHECK-STATS-ABS-TIMES -input-file=%t2.stats %s
// test --print-rel-times
// RUN: %klee-stats --print-rel-times --table-format=csv %t.klee-out > %t3.stats
// RUN: FileCheck -check-prefix=CHECK-STATS-REL-TIMES -input-file=%t3.stats %s
// test user-provided columns and ordering
// RUN: %klee-stats --print-columns 'Time(s),Path,ICov(%)' --table-format=csv %t.klee-out > %t4.stats
// RUN: FileCheck -check-prefix=CHECK-STATS-COL -input-file=%t4.stats %s
// test wrong column name and empty columns
// RUN: not %klee-stats --print-columns 'Path,Foo,ICov(%)' --table-format=csv %t.klee-out
// RUN: not %klee-stats --print-columns '' --table-format=csv %t.klee-out
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
// CHECK-STATS: Path,Instrs,Time(s),ICov(%),BCov(%),ICount,TSolver(%),ActiveStates,MaxActiveStates,Mem(MiB),MaxMem(MiB)
// Check there is a line with .klee-out dir, non zero instruction, less than 1 second execution time and 100 ICov.
// CHECK-STATS: {{.*\.klee-out,[1-9][0-9]+,[0-9]+\.([0-9]+),100\.00}}

// Check other formats
// CHECK-STATS-ABS-TIMES: Path,Time(s),TUser(s),TResolve(s),TCex(s),TSolver(s),TFork(s)
// CHECK-STATS-REL-TIMES: Path,Time(s),TSolver(%),TResolve(%),TCex(%),TFork(%),TUser(%)
// CHECK-STATS-COL: Time(s),Path,ICov(%)

// No Summary row for csv
// CHECK-STATS-COL-NOT: {{^}}Total{{.*}}{{$}}