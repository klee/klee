// RUN: %llvmgcc %s -emit-llvm -g -c -o %t1.bc
// RUN: %klee --no-output --exit-on-error %t1.bc

#include <stdio.h>

void foo(const char *msg) { printf("foo: %s\n", msg); }
void baz(const char *msg) { printf("baz: %s\n", msg); }

void (*xx)(const char *) = foo;

void bar(void (*fp)(const char *)) { fp("called via bar"); }

int main(int argc, char **argv) {
  void (*fp)(const char *) = foo;

  printf("going to call through fp\n");
  fp("called via fp");

  printf("calling via pass through\n");
  bar(foo);
        
  fp = baz;
  fp("called via fp");

  xx("called via xx");

#if 0
  klee_make_symbolic(&fp, sizeof fp);
  if(fp == baz) {
    printf("fp = %p, baz = %p\n", fp, baz);
    fp("calling via symbolic!");
  }
#endif

  return 0;
}
