// RUN: %clang %s -emit-llvm -g %O0opt -c -o %t.bc
// RUN: %clang %s -o %t
// RUN: env TEST_ENV_VAR=foo %t > %S/correct.log
// RUN: echo -e "TEST_ENV_VAR=foo\n" > %S/test.env
// RUN: %klee --libc=uclibc --posix-runtime --env-file=%S/test.env %t.bc > %S/test.log
// RUN: diff %S/correct.log %S/test.log
#include <stdio.h>
#include <stdlib.h>

#define ENV_VAR "TEST_ENV_VAR"

void __attribute__((constructor)) ctor(void) {
    printf("%s=%s\n", ENV_VAR, getenv(ENV_VAR));
}

int main(int argc, const char *argv[]) {
    return 0;
}
