// RUN: %clang %s -fsanitize=integer-divide-by-zero -emit-llvm -g %O0opt -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out %t.bc 2>&1 | FileCheck %s

#if defined(__SIZEOF_INT128__) && !defined(_WIN32)
typedef __int128 intmax;
#else
typedef long long intmax;
#endif

int main() {
  intmax DIVIDEND;
  klee_make_symbolic(&DIVIDEND, sizeof DIVIDEND, "DIVIDEND");
  // CHECK: ubsan_integer_divide_by_zero.c:[[@LINE+1]]: overflow on division or remainder
  intmax result = DIVIDEND / 0;
  return 0;
}
