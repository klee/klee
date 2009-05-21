// RUN: %llvmgcc %s -emit-llvm -O0 -c -o %t1.bc
// RUN: %klee --libc=klee --exit-on-error %t1.bc

#include <arpa/inet.h>
#include <assert.h>

int main() {
  
  uint32_t n = 0;
  klee_make_symbolic(&n, sizeof(n));
  
  uint32_t h = ntohl(n);
  assert(htonl(h) == n);
  
  return 0;
}
