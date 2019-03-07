// RUN: %clang %s -emit-llvm %O0opt -g -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// NOTE: Have to pass `--optimize=false` to avoid vector operations being
// optimized away.
// RUN: %klee --output-dir=%t.klee-out --optimize=false --exit-on-error %t1.bc
#include "klee/klee.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

typedef uint32_t v4ui __attribute__((vector_size(16)));

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
  uint32_t data[8];
  klee_make_symbolic(&data, sizeof(data), "unsigned_data");
  v4ui a = {data[0], data[1], data[2], data[3]};
  v4ui b = {data[4], data[5], data[6], data[7]};

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
  // We do not use && on purpose = make klee fork exactly once
  if ((data[4] != 0) & (data[5] != 0) & (data[6] != 0) & (data[7] != 0)) {
    c = a / b;
    ASSERT_ELV4(c, /, a, b);

    // Test mod
    c = a % b;
    ASSERT_ELV4(c, %, a, b);
    return 0;
  }

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
  // no '&&', the same as above
  if (((data[0]) < 32) & (data[1] < 32) & (data[2] < 32) & (data[3] < 32)) {
    c = b << a;
    ASSERT_ELV4(c, <<, b, a);

    // Test logic right shift
    c = b >> a;
    ASSERT_ELV4(c, >>, b, a);
    return 0;
  }

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
  int condition = 0;
  klee_make_symbolic(&condition, sizeof(condition), "unsigned_condition");
  c = condition ? a : b;
  ASSERT_EL_TERNARY_SCALAR_CONDITION_V4(c, condition, a, b);
  return 0;
}
