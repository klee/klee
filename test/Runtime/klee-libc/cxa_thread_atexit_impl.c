// RUN: %clang %s -emit-llvm %O0opt -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --exit-on-error --libc=klee %t1.bc

// just make sure __cxa_thread_atexit_impl works ok

#include <assert.h>

extern int __cxa_thread_atexit_impl(void (*)(void*), void*, void *);

static int x; // a global whose address we can take

void boo(void *h) {
  assert(h == (void*)&x);
}

int main() {
  __cxa_thread_atexit_impl(boo, (void*)&x, (void*)0);
  return 0;
}
