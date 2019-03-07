// RUN: %clang %s -emit-llvm %O0opt -g -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// NOTE: Have to pass `--optimize=false` to avoid vector operations being
// constant folded away.
// RUN: %klee --output-dir=%t.klee-out --optimize=false --exit-on-error %t1.bc
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

typedef uint32_t v4ui __attribute__((vector_size(16)));
typedef int32_t v4si __attribute__((vector_size(16)));

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
  // Unsigned tests
  {
    v4ui a = {0, 1, 2, 3};
    v4ui b = {10, 20, 30, 3};

    // Test addition
    v4ui c = a + b;
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

    // Test mod
    c = a % b;
    ASSERT_ELV4(c, %, a, b);

    // Test bitwise and
    c = a & b;
    ASSERT_ELV4(c, &, a, b);

    // Test bitwise or
    c = a | b;
    ASSERT_ELV4(c, |, a, b);

    // Test bitwise xor
    c = a ^ b;
    ASSERT_ELV4(c, ^, a, b);

    // Test left shift
    c = b << a;
    ASSERT_ELV4(c, <<, b, a);

    // Test logic right shift
    c = b >> a;
    ASSERT_ELV4(c, >>, b, a);

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

  // Signed tests
  {
    v4si a = {-1, 2, -3, 4};
    v4si b = {-10, 20, -30, 40};

    // Test addition
    v4si c = a + b;
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

    // Test mod
    c = a % b;
    ASSERT_ELV4(c, %, a, b);

    // Test bitwise and
    c = a & b;
    ASSERT_ELV4(c, &, a, b);

    // Test bitwise or
    c = a | b;
    ASSERT_ELV4(c, |, a, b);

    // Test bitwise xor
    c = a ^ b;
    ASSERT_ELV4(c, ^, a, b);

    // Test left shift
    v4si shifts = {1, 2, 3, 4};
    c = b << shifts;
    ASSERT_ELV4(c, <<, b, shifts);

    // Test arithmetic right shift
    c = b >> shifts;
    ASSERT_ELV4(c, >>, b, shifts);

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
