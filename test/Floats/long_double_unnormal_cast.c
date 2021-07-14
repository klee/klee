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
  // [1] [100000000000001] [0]     [00100000...]
  const uint64_t lowBits = 0x2000000000000000;
  const uint16_t highBits = 0xc001;
  long double v = 0.0l;
  memcpy(&v, &lowBits, sizeof(lowBits));       // 64 bits
  memcpy(((uint8_t *)(&v)) + 8, &highBits, 2); // 16 bits
  dump_long_double(v);
  long double x = 1.0l;
  // TODO: Find out where this behaviour is documented.
  assert(isnan((float)v));
  assert(isnan((double)v));

  // Check signed casts
  assert(((int8_t)v) == 0);
  // FIXME: This assert below doesn't work properly. When we compile this
  // program natively with Clang the casted value is zero and when we compile
  // natively with gcc is -32768. We should report this!
  int16_t temp = (int16_t)v;
  assert((temp == 0) | (temp == -32768));
  assert(((int32_t)v) == -2147483648);
  assert(((int64_t)v) == INT64_MIN);

  // Check signed casts
  assert(((uint8_t)v) == 0);
  assert(((uint16_t)v) == 0);
  assert(((uint32_t)v) == 0);
  uint64_t temp2 = (uint64_t)v;
  // FIXME: This assert below doesn't work properly. When we compile this
  // program natively with Clang the casted value is 0 and when we compile
  // natively with gcc it is 0x8000000000000000. We should report this!
  assert(temp2 == 0 || temp2 == 0x8000000000000000);
}
