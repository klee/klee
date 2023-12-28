int main(int x) {
  int *p = 0;
  if (x) {
    p = &x;
  } else {
    klee_sleep();
  }
  return *p;
}

// RUN: %clang %s -emit-llvm -c -g -O0 -Xclang -disable-O0-optnone -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --use-guided-search=error --mock-external-calls --skip-not-symbolic-objects --skip-not-lazy-initialized --check-out-of-memory --search=bfs --max-instructions=45 --max-cycles-before-stuck=0 --use-lazy-initialization=only --analysis-reproduce=%s.json %t1.bc
// RUN: FileCheck -input-file=%t.klee-out/warnings.txt %s -check-prefix=CHECK-NONE
// CHECK-NONE: KLEE: WARNING: 50.00% NullPointerException False Positive at trace 1
// RUN: FileCheck -input-file=%t.klee-out/messages.txt %s -check-prefix=CHECK-DISTANCE
// CHECK-DISTANCE: KLEE: (0, 1, 0) for Target 1: error in function main (lines 8 to 8) 
