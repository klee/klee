// RUN: %llvmgcc %s -emit-llvm -g -O0 -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --search=dfs %t.bc
// RUN: test -f %t.klee-out/test000001.ktest
// RUN: test -f %t.klee-out/test000001.abort.err
// RUN: test ! -f %t.klee-out/test000002.ktest

// Now try to replay with libkleeRuntest
// RUN: %cc %s %libkleeruntest -Wl,-rpath %libkleeruntestdir -o %t_runner

// Check that the default is to exit with an error
// RUN: not env KTEST_FILE=%t.klee-out/test000001.ktest %t_runner 2>&1

// Check that setting `KLEE_RUN_TEST_ERRORS_NON_FATAL` will still exit with an error
// RUN: not env KTEST_FILE=%t.klee-out/test000001.ktest KLEE_RUN_TEST_ERRORS_NON_FATAL=1 %t_runner 2>&1

#include "klee/klee.h"

int main(int argc, char** argv) {
  klee_abort();
}
