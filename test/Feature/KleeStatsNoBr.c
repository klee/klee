// RUN: %clang %s -emit-llvm -g %O0opt -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out %t.bc 2> %t.log
// RUN: %klee-stats --print-columns 'BCov(%),Branches,FullBranches,PartialBranches' --table-format=csv %t.klee-out > %t.stats
// RUN: FileCheck -check-prefix=CHECK-STATS -input-file=%t.stats %s

int main(){
  int a = 42;
  a -= 42;
  return a;
}

// Check that there are no branches in stats but 100% coverage
// CHECK-STATS: BCov(%),Branches,FullBranches,PartialBranches
// CHECK-STATS: 100.00,0,0,0
