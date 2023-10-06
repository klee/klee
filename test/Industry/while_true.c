int main() {
  int *p = 0;
  klee_sleep();
  int s = 0;
  while (s < 10) {
    s = (s + 1) % 10;
  }
  return *p;
}

// RUN: %clang %s -emit-llvm -c -g -O0 -Xclang -disable-O0-optnone -o %t1.bc
// RUN: rm -rf %t.klee-out-1
// RUN: %klee --output-dir=%t.klee-out-1 --use-guided-search=error --mock-external-calls --skip-not-symbolic-objects --skip-not-lazy-initialized --check-out-of-memory --max-stepped-instructions=10 --max-cycles-before-stuck=0 --use-lazy-initialization=only --analysis-reproduce=%s.json %t1.bc
// RUN: FileCheck -input-file=%t.klee-out-1/warnings.txt %s -check-prefix=CHECK-NONE
// CHECK-NONE: KLEE: WARNING: 0.00% NullPointerException False Positive at trace 1
// RUN: FileCheck -input-file=%t.klee-out-1/messages.txt %s -check-prefix=CHECK-REACH-1
// CHECK-REACH-1: (0, 1, 4) for Target 1: error in function main (lines 8 to 8)

// RUN: rm -rf %t.klee-out-2
// RUN: %klee --output-dir=%t.klee-out-2 --use-guided-search=error --mock-external-calls --skip-not-symbolic-objects --skip-not-lazy-initialized --check-out-of-memory --max-stepped-instructions=50000 --max-cycles-before-stuck=0 --use-lazy-initialization=only --analysis-reproduce=%s.json %t1.bc
// RUN: FileCheck -input-file=%t.klee-out-2/warnings.txt %s -check-prefix=CHECK-ALL
// CHECK-ALL: KLEE: WARNING: 99.00% NullPointerException False Positive at trace 1
// RUN: FileCheck -input-file=%t.klee-out-2/messages.txt %s -check-prefix=CHECK-REACH-2
// CHECK-REACH-2: (0, 1, 1) for Target 1: error in function main (lines 8 to 8)
