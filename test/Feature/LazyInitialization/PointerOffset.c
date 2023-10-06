// RUN: %clang %s -emit-llvm %O0opt -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --skip-not-symbolic-objects --use-timestamps=false --skip-local=false %t.bc
// RUN: %ktest-tool %t.klee-out/test*.ktest > %t.log
// RUN: FileCheck %s -input-file=%t.log
// CHECK: pointers: [(0, 1, 4)]

int main() {
  int *index;
  int a[2];
  klee_make_symbolic(&index, sizeof(index), "index");
  klee_make_symbolic(a, sizeof(a), "a");
  a[0] = 0;
  a[1] = 0;

  *index = 1;

  if (a[1] == 1) {
    return 1;
  } else {
    return 0;
  }
}
