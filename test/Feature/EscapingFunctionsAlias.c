// Darwin does not support strong aliases.
// REQUIRES: not-darwin
// RUN: %clang -emit-llvm %O0opt -g -c %s -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee -debug-print-escaping-functions --output-dir=%t.klee-out %t.bc 2> %t.log
// RUN: FileCheck --input-file=%t.log %s

#include <stdint.h>

void global_alias(void) __attribute__((alias("global_aliasee")));
void global_aliasee(void) {
    return;
}

uint8_t bitcast_of_alias(void) __attribute__((alias("bitcast_of_global_alias")));
uint8_t bitcast_of_global_alias(void) {
    return 1;
}

uint8_t bitcast_of_aliasee(void) __attribute__((alias("bitcast_of_global_aliasee")));
uint8_t bitcast_of_global_aliasee(void) {
    return 1;
}

uint64_t bitcast_in_global_alias(void) __attribute__((alias("bitcast_in_alias")));
uint8_t bitcast_in_alias(void) {
    return 1;
}

int main(int argc, char *argv[]) {
    global_aliasee();
    global_alias();

    // casting alias
    // should result in aliasee being marked as escaping
    uint64_t (*f1)(void) =(uint64_t (*)(void))bitcast_of_alias;
    f1();

    // casting aliasee
    uint64_t (*f2)(void) =(uint64_t (*)(void))bitcast_of_global_aliasee;
    f2();

    bitcast_in_alias();
    // cast when aliasing should not result in either function being marked as escaping
    bitcast_in_global_alias();

    // CHECK: KLEE: escaping functions: {{\[((bitcast_of_global_alias|bitcast_of_global_aliasee)(, )?){2}\]}}
    return 0;
}
