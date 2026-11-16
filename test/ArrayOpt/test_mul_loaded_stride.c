// RUN: %clang %s -emit-llvm %O0opt -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --optimize=false --optimize-array=index --emit-all-errors --max-time=20s --max-memory=512 --output-dir=%t.klee-out %t.bc > %t.log 2>&1
// RUN: FileCheck %s -input-file=%t.log --check-prefix=CHECK
//
// #1841: the index optimizer must not treat a constant factor multiplying a
// value *read from a constant array* as the enumeration stride for the
// array index. Doing so reported a false ASSERTION FAIL on a program whose
// assertions hold for every input.

#include "klee/klee.h"
#include <stdint.h>

static const uint8_t table[8] = {10, 20, 30, 40, 50, 60, 70, 80};

uint64_t subject(uint16_t input) {
    unsigned index = input & 7u;
    return (uint32_t)table[index] * 3u;
}

int main(void) {
    uint8_t input;
    klee_make_symbolic(&input, 1, "input");
    uint64_t actual = subject(input);
    unsigned index = input & 7u;

    if (actual == 90u) {
        klee_assert(index == 2u);
    } else {
        klee_assert(index != 2u);
    }
    return 0;
}

// CHECK: KLEE: done: completed paths = 2
// CHECK-NOT: ASSERTION FAIL
