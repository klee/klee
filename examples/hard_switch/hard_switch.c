// hard_no_loop.c
#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include <klee/klee.h>

static inline uint32_t rotl32(uint32_t x, unsigned r) {
  return (x << r) | (x >> (32 - r));
}
static inline uint64_t rotl64(uint64_t x, unsigned r) {
  return (x << r) | (x >> (64 - r));
}

#define C1 0x9E3779B97F4A7C15ULL
#define C2 0xD1B54A32D192ED03ULL
#define C3 0x94D049BB133111EBULL
#define M1 0x45d9f3bu
#define M2 0x27d4eb2du

static const uint32_t PR[16] = {
  2u, 3u, 5u, 7u, 11u, 13u, 17u, 19u,
  23u, 29u, 31u, 37u, 41u, 43u, 47u, 53u
};

int main(void) {
  uint8_t in[8];

  klee_make_symbolic(in, sizeof(in), "in");
  for (int i = 0; i < 8; ++i) {
    klee_assume(in[i] >= 0x20);
    klee_assume(in[i] <= 0x7e);
  }

  uint64_t x = ((uint64_t)in[0])       |
               ((uint64_t)in[1] << 8)  |
               ((uint64_t)in[2] << 16) |
               ((uint64_t)in[3] << 24) |
               ((uint64_t)in[4] << 32) |
               ((uint64_t)in[5] << 40) |
               ((uint64_t)in[6] << 48) |
               ((uint64_t)in[7] << 56);

  uint64_t a = x * C1 + C2;
  a ^= (a >> 33);
  uint64_t b = rotl64(a ^ (x * C3), 17) + (a ^ 0xA5A5A5A5A5A5A5A5ULL);
  uint32_t c = (uint32_t)(b ^ (b >> 32));
  c = rotl32(c * M1 + M2, 13) ^ (c * 0x9e3779b1u);

  uint8_t s0 = (uint8_t)( c        & 0xFF);
  uint8_t s1 = (uint8_t)((c >>  8) & 0xFF);
  uint8_t s2 = (uint8_t)((c >> 16) & 0xFF);
  uint8_t s3 = (uint8_t)((c >> 24) & 0xFF);

  uint32_t p  = PR[(in[0] ^ in[7]) & 0x0F];
  uint32_t q  = PR[(in[3] + in[4]) & 0x0F];
  uint32_t k  = (uint32_t)((in[1] ^ in[6]) & 0x0F);

  uint32_t t = 0;
  switch (k) {
    case 0:  t = (uint32_t)((s0 + 1u) * (p | 1u)) ^ (s1 + 7u); break;
    case 1:  t = (uint32_t)((s1 + 3u) * (q | 1u)) + (s2 ^ 0x5Au); break;
    case 2:  t = (uint32_t)(rotl32(c, s0 & 31) ^ (p * 257u)); break;
    case 3:  t = (uint32_t)((s2 + 1u) * (s2 + 3u)) ^ (q * 131u); break;
    case 4:  t = (uint32_t)((s3 * 257u) + (s0 * 17u)) ^ p; break;
    case 5:  t = (uint32_t)((s0 ^ s1) * (s2 | 1u)) + q; break;
    case 6:  t = (uint32_t)((s1 + s3) * (p + 1u)) ^ (c * 33u); break;
    case 7:  t = (uint32_t)((s0 + s1 + s2 + s3) * (q + 2u)); break;
    case 8:  t = (uint32_t)(rotl32(c ^ (p * 0x1f3u), (s2 & 15)) + 0x13579bdu); break;
    case 9:  t = (uint32_t)((s0 * s0) + (s1 * s2) + (s3 * 313u)); break;
    case 10: t = (uint32_t)((s0 + 1u) * (s1 + 1u)) ^ ((s2 + 1u) * (s3 + 1u)); break;
    case 11: t = (uint32_t)((s0 | s2) * (s1 | 1u)) + (p ^ q); break;
    case 12: t = (uint32_t)((s3 + 5u) * (s0 + 11u)) ^ rotl32(c, 7); break;
    case 13: t = (uint32_t)((s2 ^ 0xA5u) * (s1 ^ 0x5Au)) + 0x3141592u; break;
    case 14: t = (uint32_t)((s0 + s2) * (s1 + s3)) ^ 0xC0FFEEu; break;
    default: t = (uint32_t)(c ^ (p * q * 17u)); break;
  }

  uint32_t mod1 = (uint32_t)((t + (s0 * (uint32_t)(s1 | 1u)) + 7u) % 257u);

  uint32_t mix2 = rotl32(t ^ (c * 0x85ebca6bu), (s2 & 31));

  int condA = (int)((mod1 ^ (p & 0xFFu)) == 123);
  int condB = (int)(((mix2 & 0xFFFFu) + (s0 & 0xF) + (s1 & 0xF)) == 0x1F3);
  int condC = (int)(((t ^ (q * 97u)) & 0x3FFFu) == ((s2 * 33u + s3) & 0x3FFFu));

  if ((condA && condB) ^ condC) {
    klee_assert(0 && "BUG reached: tricky non-loop constraints satisfied");
  }

  return 0;
}
