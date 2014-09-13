/* Very basic test, as right now SELinux support is extremely basic */
// RUN: %llvmgcc %s -emit-llvm -O0 -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --posix-runtime --exit-on-error %t1.bc --sym-arg 2 > %t.log
// XFAIL: no-selinux

#include <selinux/selinux.h>
#include <stdio.h>
#include <assert.h>

int main(int argc, char** argv) {

  security_context_t con;

  assert(argc == 2);

  int selinux = is_selinux_enabled();
  printf("selinux enabled = %d\n", selinux);
  
  if (setfscreatecon(argv[1]) < 0)
    printf("Error: set\n");
  else printf("Success: set\n");
  
  if (getfscreatecon(&con) < 0)
    printf("Error: get\n");
  else printf("Success: get\n");

  printf("create_con = %s\n", con);

  return 0;
}
