// RUN: %clang -DDYNAMIC_LIBRARY=1 %s -shared -o %t1.so
// RUN: %clang %s -emit-llvm %O0opt -g -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// NOTE: Have to pass `--optimize=false` to avoid vector operations being
// constant folded away.
// RUN: export LD_PRELOAD=%t1.so
// RUN: export DYLD_INSERT_LIBRARIES=%t1.so
// RUN: %klee --output-dir=%t.klee-out --optimize=false --exit-on-error --external-calls=all %t1.bc

#include <stdint.h>

typedef uint32_t v4ui __attribute__((vector_size(16)));  // 128-bit vector
typedef uint32_t v8ui __attribute__((vector_size(32)));  // 256-bit vector
typedef uint32_t v16ui __attribute__((vector_size(64))); // 512-bit vector

v4ui call4(v4ui v);

v8ui call8(v8ui v);

v16ui call16(v16ui v);

int call_mixed(v16ui v16, v8ui v8, v4ui v4);

#ifdef DYNAMIC_LIBRARY

v4ui call4(v4ui v) {
  return v;
}

v8ui call8(v8ui v) {
  return v;
}

v16ui call16(v16ui v) {
  return v;
}

int call_mixed(v16ui v16, v8ui v8, v4ui v4) {
  return v16[15] + v8[7] + v4[3];
}

#else

#include "assert.h"
#include "klee/klee.h"

int main() {
  v4ui v4 = {1, 2, 3, 4};
  {
    v4ui r = call4(v4);
    for (int i = 0; i < 4; ++i) {
      klee_assert(r[i] == v4[i]);
    }
  }

  v8ui v8 = {1, 2, 3, 4, 5, 6, 7, 8};
  {
    v8ui r = call8(v8);
    for (int i = 0; i < 8; ++i) {
      klee_assert(r[i] == v8[i]);
    }
  }

  v16ui v16 = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
  {
    v16ui r = call16(v16);
    for (int i = 0; i < 16; ++i) {
      klee_assert(r[i] == v16[i]);
    }
  }

  {
    int r = call_mixed(v16, v8, v4);
    klee_assert(r == 28);
  }
  return 0;
}

#endif
