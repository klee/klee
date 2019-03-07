// RUN: %clang %s -emit-llvm %O0opt -g -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// NOTE: Have to pass `--optimize=false` to avoid vector operations being
// constant folded away.
// RUN: %klee --output-dir=%t.klee-out --optimize=false --exit-on-error %t1.bc
#include "klee/klee.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define ASSERT_EQ_V4(X, Y) \
  assert(X[0] == Y[0]); \
  assert(X[1] == Y[1]); \
  assert(X[2] == Y[2]); \
  assert(X[3] == Y[3]);

#define ASSERT_EQ_V8(X, Y) \
  assert(X[0] == Y[0]); \
  assert(X[1] == Y[1]); \
  assert(X[2] == Y[2]); \
  assert(X[3] == Y[3]); \
  assert(X[4] == Y[4]); \
  assert(X[5] == Y[5]); \
  assert(X[6] == Y[6]); \
  assert(X[7] == Y[7]); \

typedef uint32_t v4ui __attribute__ ((vector_size (16)));
typedef uint32_t v8ui __attribute__ ((vector_size (32)));
int main() {
  v4ui f = { 0, 19, 129, 255 };
  klee_make_symbolic(&f, sizeof(v4ui), "f");
  klee_print_expr("f:=", f);

  // Test Single vector shuffle.

  // Extract same element uniformly
  for (int index=0; index < 4; ++index) {
    v4ui mask = { index, index, index, index };
    v4ui result = __builtin_shufflevector(f, mask);
    v4ui expectedResult = { f[index], f[index], f[index], f[index]};
    ASSERT_EQ_V4(result, expectedResult);
  }

  // Reverse vector
  v4ui mask = { 3, 2, 1, 0 };
  v4ui result = __builtin_shufflevector(f, mask);
  v4ui expectedResult = {f[3], f[2], f[1], f[0]};
  ASSERT_EQ_V4(result, expectedResult);

  // Extract a single element (Clang requires the mask to be constants so we can't use a for a loop).
#define TEST_SINGLE_EXTRACT(I)\
  do \
  { \
    uint32_t v __attribute__((vector_size(sizeof(uint32_t)))) = __builtin_shufflevector(f, f, I); \
    uint32_t expected = f[ I % 4 ]; \
    assert(v[0] == expected); \
  } while(0);
  TEST_SINGLE_EXTRACT(0);
  TEST_SINGLE_EXTRACT(1);
  TEST_SINGLE_EXTRACT(2);
  TEST_SINGLE_EXTRACT(3);
  TEST_SINGLE_EXTRACT(4);
  TEST_SINGLE_EXTRACT(5);
  TEST_SINGLE_EXTRACT(6);
  TEST_SINGLE_EXTRACT(7);

  // Test two vector shuffle
  v4ui a = { 0, 1, 2, 3 };
  v4ui b = { 4, 5, 6, 7 };

  {
    // [a] + [b]
    v8ui result = __builtin_shufflevector(a, b, 0, 1, 2, 3, 4, 5, 6, 7);
    v8ui expected = { a[0], a[1], a[2], a[3], b[0], b[1], b[2], b[3] };
    ASSERT_EQ_V8(result, expected);
  }



  return 0;
}
