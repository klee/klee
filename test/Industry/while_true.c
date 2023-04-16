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
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --use-guided-search=error --mock-external-calls --skip-not-symbolic-objects --skip-not-lazy-initialized --check-out-of-memory --max-stepped-instructions=10 --max-cycles=0 --analysis-reproduce=%s.json %t1.bc
// RUN: FileCheck -input-file=%t.klee-out/warnings.txt %s -check-prefix=CHECK-NONE
// CHECK-NONE: KLEE: WARNING: 0.00% NullPointerException False Positive at trace 1
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --use-guided-search=error --mock-external-calls --skip-not-symbolic-objects --skip-not-lazy-initialized --check-out-of-memory --max-stepped-instructions=5000 --max-cycles=0 --analysis-reproduce=%s.json %t1.bc
// RUN: FileCheck -input-file=%t.klee-out/warnings.txt %s -check-prefix=CHECK-ALL
// CHECK-ALL: KLEE: WARNING: 99.00% NullPointerException False Positive at trace 1