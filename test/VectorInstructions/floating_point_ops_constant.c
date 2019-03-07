// RUN: %clang %s -emit-llvm %O0opt -g -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// NOTE: Have to pass `--optimize=false` to avoid vector operations being
// constant folded away.
// RUN: %klee --output-dir=%t.klee-out --optimize=false --exit-on-error %t1.bc
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

typedef float v4float __attribute__((vector_size(16)));
typedef double v4double __attribute__((vector_size(32)));

#define ASSERT_EL(C, OP, A, B, INDEX) assert(C[INDEX] == (A[INDEX] OP B[INDEX]))
#define ASSERT_ELV4(C, OP, A, B)                                               \
  do {                                                                         \
    ASSERT_EL(C, OP, A, B, 0);                                                 \
    ASSERT_EL(C, OP, A, B, 1);                                                 \
    ASSERT_EL(C, OP, A, B, 2);                                                 \
    ASSERT_EL(C, OP, A, B, 3);                                                 \
  } while (0);

#define ASSERT_EL_TRUTH(C, OP, A, B, INDEX)                                    \
  assert(C[INDEX] ? (A[INDEX] OP B[INDEX]) : (!(A[INDEX] OP B[INDEX])))
#define ASSERT_EL_TRUTH_V4(C, OP, A, B)                                        \
  do {                                                                         \
    ASSERT_EL_TRUTH(C, OP, A, B, 0);                                           \
    ASSERT_EL_TRUTH(C, OP, A, B, 1);                                           \
    ASSERT_EL_TRUTH(C, OP, A, B, 2);                                           \
    ASSERT_EL_TRUTH(C, OP, A, B, 3);                                           \
  } while (0);

#define ASSERT_EL_TERNARY_SCALAR_CONDITION(C, CONDITION, A, B, INDEX)          \
  assert(C[INDEX] == (CONDITION ? A[INDEX] : B[INDEX]))
#define ASSERT_EL_TERNARY_SCALAR_CONDITION_V4(C, CONDITION, A, B)              \
  do {                                                                         \
    ASSERT_EL_TERNARY_SCALAR_CONDITION(C, CONDITION, A, B, 0);                 \
    ASSERT_EL_TERNARY_SCALAR_CONDITION(C, CONDITION, A, B, 1);                 \
    ASSERT_EL_TERNARY_SCALAR_CONDITION(C, CONDITION, A, B, 2);                 \
    ASSERT_EL_TERNARY_SCALAR_CONDITION(C, CONDITION, A, B, 3);                 \
  } while (0);

int main() {
  // Float tests
  {
    v4float a = {1.0f, 2.5f, -3.5f, 4.0f};
    v4float b = {-10.0f, 20.2f, -30.5f, 40.1f};

    // Test addition
    v4float c = a + b;
    ASSERT_ELV4(c, +, a, b);

    // Test subtraction
    c = b - a;
    ASSERT_ELV4(c, -, b, a);

    // Test multiplication
    c = a * b;
    ASSERT_ELV4(c, *, a, b);

    // Test division
    c = a / b;
    ASSERT_ELV4(c, /, a, b);

    // NOTE: Can't use `ASSERT_ELV4` due to semantics
    // of GCC vector extensions. See
    // https://gcc.gnu.org/onlinedocs/gcc/Vector-Extensions.html

    // Test ==
    c = a == b;
    ASSERT_EL_TRUTH_V4(c, ==, a, b);

    // Test <
    c = a < b;
    ASSERT_EL_TRUTH_V4(c, <, a, b);

    // Test <=
    c = a <= b;
    ASSERT_EL_TRUTH_V4(c, <=, a, b);

    // Test >
    c = a > b;
    ASSERT_EL_TRUTH_V4(c, >, a, b);

    // Test >=
    c = a > b;
    ASSERT_EL_TRUTH_V4(c, >, a, b);

    // Test !=
    c = a != b;
    ASSERT_EL_TRUTH_V4(c, !=, a, b);

    // Test ternary operator
    c = 0 ? a : b;
    ASSERT_EL_TERNARY_SCALAR_CONDITION_V4(c, 0, a, b);
    c = 1 ? a : b;
    ASSERT_EL_TERNARY_SCALAR_CONDITION_V4(c, 1, a, b);
  }

  // double tests
  {
    v4double a = {1.0, 2.5, -3.5, 4.0};
    v4double b = {-10.0, 20.2, -30.5, 40.1};

    // Test addition
    v4double c = a + b;
    ASSERT_ELV4(c, +, a, b);

    // Test subtraction
    c = b - a;
    ASSERT_ELV4(c, -, b, a);

    // Test multiplication
    c = a * b;
    ASSERT_ELV4(c, *, a, b);

    // Test division
    c = a / b;
    ASSERT_ELV4(c, /, a, b);

    // Test ==
    c = a == b;
    ASSERT_EL_TRUTH_V4(c, ==, a, b);

    // Test <
    c = a < b;
    ASSERT_EL_TRUTH_V4(c, <, a, b);

    // Test <=
    c = a <= b;
    ASSERT_EL_TRUTH_V4(c, <=, a, b);

    // Test >
    c = a > b;
    ASSERT_EL_TRUTH_V4(c, >, a, b);

    // Test >=
    c = a > b;
    ASSERT_EL_TRUTH_V4(c, >, a, b);

    // Test !=
    c = a != b;
    ASSERT_EL_TRUTH_V4(c, !=, a, b);

    // Test ternary operator
    c = 0 ? a : b;
    ASSERT_EL_TERNARY_SCALAR_CONDITION_V4(c, 0, a, b);
    c = 1 ? a : b;
    ASSERT_EL_TERNARY_SCALAR_CONDITION_V4(c, 1, a, b);
  }
  return 0;
}
