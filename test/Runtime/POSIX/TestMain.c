// Run applications with the posix environment but without additional arguments argc, argv for main
// RUN: %clang -DMAIN1 %s -emit-llvm %O0opt -c -g -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --posix-runtime %t.bc | FileCheck %s -check-prefix=CHECK-MAIN1
// RUN: %clang -DMAIN2 %s -emit-llvm %O0opt -c -g -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --posix-runtime %t.bc | FileCheck %s -check-prefix=CHECK-MAIN2
#ifdef MAIN1
#include <stdio.h>
int main() {
 printf("Execute main1\n");
 // CHECK-MAIN1: Execute main1
 return 0;
}
#endif
#ifdef MAIN2
#include <stdio.h>
int main(void) {
 printf("Execute main2\n");
 // CHECK-MAIN2: Execute main2
 return 0;
}
#endif
