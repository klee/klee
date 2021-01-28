// RUN: %clang %s -fsanitize=integer-divide-by-zero -emit-llvm -g %O0opt -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --emit-all-errors --ubsan-runtime %t.bc 2>&1 | FileCheck %s

#if defined(__SIZEOF_INT128__) && !defined(_WIN32)
typedef __int128 intmax;
#else
typedef long long intmax;
#endif

int main() {
  intmax x;
  volatile intmax result;

  klee_make_symbolic(&x, sizeof(x), "x");

  // CHECK: runtime/Sanitizer/ubsan/ubsan_handlers.cpp:35: integer division overflow
  result = x / 0;
  return 0;
}
