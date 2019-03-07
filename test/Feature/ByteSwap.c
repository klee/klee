// RUN: %clang %s -emit-llvm %O0opt -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --libc=klee --exit-on-error %t1.bc

#include <arpa/inet.h>
#include <assert.h>

int main() {
  
  uint32_t n = 0;
  klee_make_symbolic(&n, sizeof(n), "n");
  
  uint32_t h = ntohl(n);
  assert(htonl(h) == n);
  
  return 0;
}
