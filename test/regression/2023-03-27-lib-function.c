// RUN: %clang %s -g -emit-llvm -DINET_NTOP %O0opt -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --exit-on-error --output-dir=%t.klee-out %t1.bc 2>&1 | FileCheck -check-prefix=CHECK_INTERNAL %s
// RUN: %clang %s -g -emit-llvm -DINET_NTOP -DINET_PTON %O0opt -c -o %t2.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --exit-on-error --output-dir=%t.klee-out %t2.bc 2>&1 | FileCheck -check-prefix=CHECK_INTERNAL %s
// RUN: %clang %s -g -emit-llvm %O0opt -c -o %t3.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --exit-on-error --output-dir=%t.klee-out %t3.bc 2>&1 | FileCheck -check-prefix=CHECK_EXTERNAL %s

#include <arpa/inet.h>
#include <stdio.h>

#ifdef __FreeBSD__
#include <sys/socket.h>
#endif

#ifdef INET_NTOP
const char *inet_ntop(int af, const void *restrict src,
                      char *restrict dst, socklen_t size) {
  printf("Hello\n");
  // CHECK_INTERNAL: Hello
  return 0;
}
#endif

int main(int argc, char **argv) {
  inet_ntop(0, 0, 0, 0);
  // CHECK_INTERNAL-NOT: calling external: {{inet_ntop|__inet_ntop}}
  // CHECK_EXTERNAL: calling external: {{inet_ntop|__inet_ntop}}
#ifdef INET_PTON
  struct in_addr inaddr;
  inet_pton(AF_INET, "10.1.0.29", &inaddr);
#endif
  return 0;
}
