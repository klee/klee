// RUN: %llvmgcc %s -g -emit-llvm -O0 -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --posix-runtime %t.bc --sym-args 1 1 1 2>&1 | FileCheck %s
// RUN: test -f %t.klee-out/test000001.free.err
// RUN: test -f %t.klee-out/test000002.free.err
// RUN: test -f %t.klee-out/test000003.free.err

int main(int argc, char **argv) {
  // FIXME: Use FileCheck's CHECK-DAG to check source locations
  switch(klee_range(0, 3, "range")) {
  case 0:
    // CHECK: free of global
    free(argv);
    break;
  case 1:
    // CHECK: free of global
    free(argv[0]);
    break;
  case 2:
    // CHECK: free of global
    free(argv[1]);
    break;
  }
  return 0;
}
