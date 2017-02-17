// RUN: %llvmgcc %s -emit-llvm -g -O0 -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --exit-on-error %t.bc
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#if defined(__x86_64__)
typedef long double v4ld __attribute__ ((vector_size(64)));
#elif defined(__i386__)
typedef long double v4ld __attribute__ ((vector_size(48)));
#else
#error Unsupported platform
#endif

// FIXME: Come up with a way to test ConstantDataSequential
// and ConstantAggregateZero with padded data types.

int main() {
  long double thing = 2.0l;

  // `long double` on x86_64/i386 (x87 fp80) has 10 bytes
  // of useful data and the rest is padding.
#if defined(__x86_64__)
  assert(sizeof(long double) == 16);
#elif defined(__i386__)
  assert(sizeof(long double) == 12);
#else
#error Unsupported platform
#endif

  // Older versions of KLEE would report an about of bounds
  // access during the `memcpy()` because it wouldn't allocate
  // the right amount of memory for `long double`.

  // Access all bits of the type including the padding.
  long double thingCopy = 0.0l;
  memcpy(&thingCopy, &thing, sizeof(long double));
  assert(thingCopy == 2.0l);

  // Check offset is computed correctly.
  long double foo[] = { 1.0l, 2.0l, 3.0l };
  long double* base = foo;
  printf("base: %zd\n", base);

  // Let KLEE compute the offset
  long double* secondEl = foo + 1;
  printf("secondEl: %zd\n", secondEl);

  // Compute what the offset should be
  size_t secondElExpected = ((size_t) base) + sizeof(long double);
  // Check they are equal
  assert(((size_t)secondEl) == secondElExpected);

  // Do the same for the next element
  long double* thirdEl = foo + 2;
  printf("thirdEl: %zd\n", thirdEl);
  size_t thirdElExpected = ((size_t) base) + (2*sizeof(long double));
  assert(((size_t)thirdEl) == thirdElExpected);

  // Now check values are correct
  assert(foo[0] == 1.0l);
  assert(foo[1] == 2.0l);
  assert(foo[2] == 3.0l);

  // TODO: Enable once we have vector support
  //v4ld localVector = { 87.5l, -27.75, 1.0l, 9374.25l };
  //assert(sizeof(localVector) == 4*sizeof(long double));
  // Now check local vector was correctly initialized
  // FIXME: KLEE doesn't support vector instructions so examine using
  // pointer arithmetic.
  //assert(*(((long double *)&localVector) + /*lane=*/0) == 87.5l);
  //assert(*(((long double *)&localVector) + /*lane=*/1) == -27.75);
  //assert(*(((long double *)&localVector) + /*lane=*/2) == 1.0l);
  //assert(*(((long double *)&localVector) + /*lane=*/3) == 9374.25l);
  return 0;
}
