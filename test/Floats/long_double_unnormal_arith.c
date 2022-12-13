// RUN: %clang %s -O0 -g -o %t1_native
// RUN: %t1_native
// RUN: %clang %s -emit-llvm -O0 -g -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --libc=klee --output-dir=%t.klee-out --exit-on-error %t1.bc
// REQUIRES: x86_64
#include <assert.h>
#include <inttypes.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

void dump_long_double(long double ld) {
  uint64_t data[2] = {0, 0};
  memcpy(data, &ld, 10);
  printf("a: 0x%.4" PRIx16 " %.16" PRIx64 "\n", (uint16_t)data[1], data[0]);
}

int main() {
  // In "8.2.2 Unsupported Double
  // Extended-Precision Float-Point Encodings and Pseudo-Denormals" from the
  // Intel(R) 64 and IA-32 Architectures Software Developer's Manual a list
  // of several classes of encodings are listed that are no longer considered
  // valid. Below is an "unnormal" which is an invalid operand which should
  // not compare equal to itself.
  //
  // It should raise the IEEE754 invalid operation exception too but we
  // don't model these in KLEE right now.
  //
  //                          Significand
  // Sign   Exponent       Integer  Fraction
  // [0] [000000000000001] [0]     [0000...1]
  const uint64_t lowBits = 0x0000000000000001;
  const uint16_t highBits = 0x0001;
  long double v = 0.0l;
  memcpy(&v, &lowBits, sizeof(lowBits));       // 64 bits
  memcpy(((uint8_t *)(&v)) + 8, &highBits, 2); // 16 bits
  dump_long_double(v);
  long double x = 1.0l;
  // FIXME: This should be isnanl()
  assert(isnan(v + x));
  assert(isnan(v - x));
  assert(isnan(v * x));
  assert(isnan(v / x));
}
