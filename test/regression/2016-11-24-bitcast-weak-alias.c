// REQUIRES: not-darwin
// RUN: %clang %s -Wall -emit-llvm -g %O0opt -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out -exit-on-error -search=nurs:covnew %t.bc DUMMY_ARG >%t1.log 2>&1
// RUN: FileCheck -input-file=%t1.log %s

// This test case is designed to cover code in `klee::getDirectCallTarget(CallSite)`.
// In particular it designed to test the case where a bitcasted function call, calls
// a weak alias.
struct v1 {
  int c;
  int d;
};

int __real_function(struct v1 *unused, struct v1 *unused2, int unused3) {
  return 0;
}

struct v2 {
  int e;
  int f;
};

int alias_function(struct v1 *, struct v1 *, int)
    __attribute__((weak, alias("__real_function")));

int main(int argc, char** argv) {
  struct v2 local = { .e= 0, .f=0 };
  int number = 0;

  // Call via a bitcasted alias.
  number = ((int (*)(struct v2 *, struct v2 *, int))alias_function)(&local,
                                                                    &local, 0);
  return number % 255;
}

// CHECK: KLEE: done: completed paths = 1
