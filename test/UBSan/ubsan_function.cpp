// REQUIRES: geq-llvm-17.0
// XFAIL: *
// RUN: %clangxx %s -fsanitize=function -emit-llvm -g %O0opt -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --emit-all-errors --ubsan-runtime %t.bc 2>&1 | FileCheck %s
// RUN: ls %t.klee-out/ | grep .ktest | wc -l | grep 1
// RUN: ls %t.klee-out/ | grep .function_type_mismatch.err | wc -l | grep 1

int target(int x) {
  return x;
}

int main() {
  typedef int (*MismatchedFunction)(double);
  MismatchedFunction fp = reinterpret_cast<MismatchedFunction>(&target);

  // CHECK: KLEE: ERROR: {{.*}}runtime/Sanitizer/ubsan/ubsan_handlers.cpp:{{[0-9]+}}: function-type-mismatch
  return fp(1.0);
}
