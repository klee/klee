// REQUIRES: geq-llvm-19.1
// RUN: %clang %s -fsanitize=implicit-bitfield-conversion -emit-llvm -g %O0opt -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --emit-all-errors --ubsan-runtime %t.bc 2>&1 | FileCheck %s
// RUN: ls %t.klee-out/ | grep .ktest | wc -l | grep 1
// RUN: ls %t.klee-out/ | grep .implicit_truncation.err | wc -l | grep 1

struct Bits {
  unsigned int value : 3;
};

int main(void) {
  struct Bits bits;
  unsigned int value = 8U;

  // CHECK: KLEE: ERROR: {{.*}}runtime/Sanitizer/ubsan/ubsan_handlers.cpp:{{[0-9]+}}: implicit-unsigned-integer-truncation
  bits.value = value;
  return bits.value;
}
