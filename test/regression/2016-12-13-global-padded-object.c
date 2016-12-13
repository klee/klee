// RUN: %llvmgcc %s -emit-llvm -g -O0 -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --exit-on-error %t.bc
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

long double globalThing = 1.0l;

long double globalLongDoubles[] = { 1.0l, 99.0l, -17.0l, 4277.5l };

#if defined(__x86_64__)
typedef long double v4ld __attribute__ ((vector_size(64)));
#elif defined(__i386__)
typedef long double v4ld __attribute__ ((vector_size(48)));
#else
#error Unsupported platform
#endif

v4ld globalVector = { 87.5l, -27.75, 1.0l, 9374.25l };

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

  // Now copy from global
  memcpy(&thing, &globalThing, sizeof(long double));
  assert(thing == 1.0l);

  // Now copy from global array
  long double localLongDoubles[4];
  assert(sizeof(localLongDoubles) == sizeof(globalLongDoubles));
  memcpy(localLongDoubles, globalLongDoubles, sizeof(localLongDoubles));
  // Check that the copy was done correctly
  for (int index=0; index < sizeof(globalLongDoubles); ++index) {
    uint8_t localByte = *(((uint8_t*) localLongDoubles) + index);
    uint8_t globalByte = *(((uint8_t*) globalLongDoubles) + index);
    assert(localByte == globalByte);
  }

  // Now copy from global vector
  v4ld localVector = globalVector;
  printf("sizeof(globalVector) = %zd\n", sizeof(globalVector));
  assert(sizeof(globalVector) == 4*sizeof(long double));
  assert(sizeof(globalVector) == sizeof(localVector));
  /* FIXME: The load from globalVector that then gets stored is terribly
            broken.
  // Check that the store was done correctly
  for (int index = 0; index < sizeof(globalVector); ++index) {
    uint8_t localByte = *(((uint8_t *)&localVector) + index);
    uint8_t globalByte = *(((uint8_t *)&globalVector) + index);
    printf("Checking byte %d local = %u, global = %u\n", index, localByte,
           globalByte);
    assert(localByte == globalByte);
  }
  */

  // Check offset is computed correctly for global array.
  long double* base = globalLongDoubles;
  printf("base: %zd\n", base);
  long double* secondEl = globalLongDoubles + 1;
  printf("secondEl: %zd\n", secondEl);
  size_t secondElExpected = ((size_t) base) + sizeof(long double);
  printf("secondElExpected: %zd\n", secondElExpected);
  assert(((size_t) secondEl) == secondElExpected);

  // Now check global array was correctly initialized
  assert(globalLongDoubles[0] == 1.0l);
  assert(globalLongDoubles[1] == 99.0l);
  assert(globalLongDoubles[2] == -17.0l);
  assert(globalLongDoubles[3] == 4277.5l);

  // Now check global vector was correctly initialized
  // FIXME: KLEE doesn't support vector instructions so examine using
  // pointer arithmetic.
  assert(*(((long double *)&globalVector) + /*lane=*/0) == 87.5l);
  assert(*(((long double *)&globalVector) + /*lane=*/1) == -27.75);
  assert(*(((long double *)&globalVector) + /*lane=*/2) == 1.0l);
  assert(*(((long double *)&globalVector) + /*lane=*/3) == 9374.25l);
  return 0;
}
