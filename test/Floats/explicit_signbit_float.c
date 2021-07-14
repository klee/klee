// RUN: %clang %s -emit-llvm -O0 -g -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --libc=klee --fp-runtime --output-dir=%t.klee-out --exit-on-error %t1.bc > %t-output.txt 2>&1
// RUN: FileCheck -input-file=%t-output.txt %s
// REQUIRES: fp-runtime
#include "klee/klee.h"
#include <math.h>
#include <assert.h>

int main() {
    float a;
    klee_make_symbolic(&a, sizeof(a), "a");
    if (__signbitf(a)) {
        if (!klee_is_normal_float(a)) {
            return 0;
        } else {
            assert(a < 0.0);
        }
    } else {
        if (!klee_is_normal_float(a)) {
            return 1;
        } else {
            assert(!(a < 0.0));
        }
    }
}
// CHECK: KLEE: done: completed paths = 4
