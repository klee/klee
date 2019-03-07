// RUN: %clang %s -emit-llvm -g %O0opt -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out %t.bc 2> %t.log
// RUN: FileCheck -input-file=%t.log %s

/**
 * This test generates one execution state without constraints.
 *
 * The state gets terminated (in this case return) and initial values
 * are generated.
 * Make sure we are able to generate an input.
 */
int main() {
  int d;

  klee_make_symbolic(&d, sizeof(d), "d");

  // CHECK-NOT: unable to compute initial values (invalid constraints?)!
  if ((d & 2) / 4)
    return 1;
  return 0;
}
