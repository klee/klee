// RUN: rm -rf %t.klee-out %t.klee-out-normal %t.klee-out-skip
// RUN: %gen-bout hellojjj --bout-file %t.hellojjj.ktest
// RUN: %gen-bout hellojjc --bout-file %t.hellojjc.ktest
// RUN: %clang -D__NO_STRING_INLINES  -D_FORTIFY_SOURCE=0 -U__OPTIMIZE__ -emit-llvm -g -c %s -o %t.bc
// RUN: %klee -pending -output-dir=%t.klee-out -seed-file=%t.hellojjc.ktest -seed-file=%t.hellojjj.ktest -max-solver-time=2s -max-tests=2 %t.bc > %t.log
// RUN: %klee -output-dir=%t.klee-out-normal -seed-file=%t.hellojjc.ktest -seed-file=%t.hellojjj.ktest -max-solver-time=2s -max-tests=2 %t.bc > %t.nonpendinglog
// RUN: FileCheck --input-file=%t.log %s
// We check that this benchmark is too hard to seed with non pending KLEE
// RUN: FileCheck --input-file=%t.nonpendinglog -check-prefix=NONPENDING %s
// RUN: %klee -pending -output-dir=%t.klee-out-skip -seed-file=%t.hellojjc.ktest -seed-file=%t.hellojjj.ktest -max-solver-time=2s -max-forks=0 %t.bc &> %t.log
// RUN: FileCheck --input-file=%t.log -check-prefix=SKIPFORK %s
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// leftrotate function definition
#define LEFTROTATE(x, c) (((x) << (c)) | ((x) >> (32 - (c))))

// These vars will contain the hash
uint32_t h0, h1, h2, h3;
//From https://gist.github.com/creationix/4710780
void md5(uint8_t *initial_msg, size_t initial_len) {

  // Message (to prepare)
  uint8_t *msg = NULL;

  // Note: All variables are unsigned 32 bit and wrap modulo 2^32 when calculating

  // r specifies the per-round shift amounts

  uint32_t r[] = {7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22,
                  5, 9, 14, 20, 5, 9, 14, 20, 5, 9, 14, 20, 5, 9, 14, 20,
                  4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23,
                  6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21};

  // Use binary integer part of the sines of integers (in radians) as constants// Initialize variables:
  uint32_t k[] = {
      0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee,
      0xf57c0faf, 0x4787c62a, 0xa8304613, 0xfd469501,
      0x698098d8, 0x8b44f7af, 0xffff5bb1, 0x895cd7be,
      0x6b901122, 0xfd987193, 0xa679438e, 0x49b40821,
      0xf61e2562, 0xc040b340, 0x265e5a51, 0xe9b6c7aa,
      0xd62f105d, 0x02441453, 0xd8a1e681, 0xe7d3fbc8,
      0x21e1cde6, 0xc33707d6, 0xf4d50d87, 0x455a14ed,
      0xa9e3e905, 0xfcefa3f8, 0x676f02d9, 0x8d2a4c8a,
      0xfffa3942, 0x8771f681, 0x6d9d6122, 0xfde5380c,
      0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70,
      0x289b7ec6, 0xeaa127fa, 0xd4ef3085, 0x04881d05,
      0xd9d4d039, 0xe6db99e5, 0x1fa27cf8, 0xc4ac5665,
      0xf4292244, 0x432aff97, 0xab9423a7, 0xfc93a039,
      0x655b59c3, 0x8f0ccc92, 0xffeff47d, 0x85845dd1,
      0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1,
      0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391};

  h0 = 0x67452301;
  h1 = 0xefcdab89;
  h2 = 0x98badcfe;
  h3 = 0x10325476;

  // Pre-processing: adding a single 1 bit
  //append "1" bit to message
  /* Notice: the input bytes are considered as bits strings,
       where the first bit is the most significant bit of the byte.[37] */

  // Pre-processing: padding with zeros
  //append "0" bit until message length in bit ≡ 448 (mod 512)
  //append length mod (2 pow 64) to message

  int new_len = ((((initial_len + 8) / 64) + 1) * 64) - 8;

  msg = calloc(new_len + 64, 1); // also appends "0" bits
                                 // (we alloc also 64 extra bytes...)
  memcpy(msg, initial_msg, initial_len);
  msg[initial_len] = 128; // write the "1" bit

  uint32_t bits_len = 8 * initial_len; // note, we append the len
  memcpy(msg + new_len, &bits_len, 4); // in bits at the end of the buffer

  // Process the message in successive 512-bit chunks:
  //for each 512-bit chunk of message:
  int offset;
  for (offset = 0; offset < new_len; offset += (512 / 8)) {

    // break chunk into sixteen 32-bit words w[j], 0 ≤ j ≤ 15
    uint32_t *w = (uint32_t *)(msg + offset);

    // Initialize hash value for this chunk:
    uint32_t a = h0;
    uint32_t b = h1;
    uint32_t c = h2;
    uint32_t d = h3;

    // Main loop:
    uint32_t i;
    for (i = 0; i < 64; i++) {

      uint32_t f, g;

      if (i < 16) {
        f = (b & c) | ((~b) & d);
        g = i;
      } else if (i < 32) {
        f = (d & b) | ((~d) & c);
        g = (5 * i + 1) % 16;
      } else if (i < 48) {
        f = b ^ c ^ d;
        g = (3 * i + 5) % 16;
      } else {
        f = c ^ (b | (~d));
        g = (7 * i) % 16;
      }

      uint32_t temp = d;
      d = c;
      c = b;
      b = b + LEFTROTATE((a + f + k[i] + w[g]), r[i]);
      a = temp;
    }

    // Add this chunk's hash to result so far:

    h0 += a;
    h1 += b;
    h2 += c;
    h3 += d;
  }

  // cleanup
  free(msg);
}

// SKIPFORK-DAG: skipping fork (max-forks reached)
int main(int argc, char **argv) {

  char msg[9];
  //This array is names arg00, so that it is easy to generate seeds with gen-bout
  klee_make_symbolic(msg, sizeof(msg), "arg00");
  for (int i = 0; i < sizeof(msg); i++)
    klee_assume(msg[i] != '\0');
  size_t len = 8;

  printf("Computing MD5\n");
  md5((uint8_t *)msg, len);

  if (h0 == 4210213877 & h1 == 504154841 & h2 == 4280798836 & h3 == 2754401878) {
    //CHECK-DAG: hellojjc MD5 ends with 78!
    //SKIPFORK-DAG: hellojjc MD5 ends with 78!
    //NONPENDING-NOT: hellojjc MD5 ends with 78!
    printf("hellojjc MD5 ends with 78!\n");
  } else if (h0 == 3460298846 & h1 == 4202711927 & h2 == 1111332085 & h3 == 234909586) {
    //CHECK-DAG: hellojjj MD5 ends with 86!
    //NONPENDING-NOT: hellojjj MD5 ends with 86!
    printf("hellojjj MD5 ends with 86!\n");
  }

  return 0;
}
