// RUN: %clang %s -emit-llvm %O0opt -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --exit-on-error %t1.bc

#include <assert.h>

typedef unsigned short uint16;
typedef unsigned int   uint32;

uint16 byteswap_uint16(uint16 x) {
  return (x << 8) | (x >> 8);
}
uint32 byteswap_uint32(uint32 x) {
  return ((byteswap_uint16(x) << 16) |
          (byteswap_uint16(x >> 16)));
}

uint16 byteswap_uint16_asm(uint16 x) {
  uint16 res;
  __asm__("rorw $8, %w0" : "=r" (res) : "0" (x) : "cc");
  return res;
}

uint32 byteswap_uint32_asm(uint32 x) {
  uint32 res;
  __asm__("rorw $8, %w0;"
          "rorl $16, %0;"
          "rorw $8, %w0" : "=r" (res) : "0" (x) : "cc");
  return res;
}

int main() {
  uint16 ui16 = klee_int("ui16");
  uint32 ui32 = klee_int("ui32");

  assert(ui16 == byteswap_uint16(byteswap_uint16_asm(ui16)));
  assert(ui32 == byteswap_uint32(byteswap_uint32_asm(ui32)));

  __asm__ __volatile__("" : : : "memory");

  return 0;
}
