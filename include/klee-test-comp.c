//===-- klee-test-comp.cpp ------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifdef EXTERNAL
#include "klee.h"
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#else
void klee_make_symbolic(void *addr, unsigned int nbytes, const char *name);
void klee_assume(_Bool condition);
__attribute__((noreturn)) void klee_silent_exit(int status);
void __assert_fail(const char *assertion, const char *file, unsigned int line,
                   const char *function);
#endif

int __VERIFIER_nondet_int(void) {
  int x;
  klee_make_symbolic(&x, sizeof(x), "int");
  return x;
}

unsigned int __VERIFIER_nondet_uint(void) {
  unsigned int x;
  klee_make_symbolic(&x, sizeof(x), "unsigned int");
  return x;
}

unsigned __int128 __VERIFIER_nondet_uint128(void) {
  unsigned __int128 x;
  klee_make_symbolic(&x, sizeof(x), "unsigned __int128");
  return x;
}

unsigned __VERIFIER_nondet_unsigned(void) {
  unsigned x;
  klee_make_symbolic(&x, sizeof(x), "unsigned");
  return x;
}

short __VERIFIER_nondet_short(void) {
  short x;
  klee_make_symbolic(&x, sizeof(x), "short");
  return x;
}

unsigned short __VERIFIER_nondet_ushort(void) {
  unsigned short x;
  klee_make_symbolic(&x, sizeof(x), "unsigned short");
  return x;
}

char __VERIFIER_nondet_char(void) {
  char x;
  klee_make_symbolic(&x, sizeof(x), "char");
  return x;
}

unsigned char __VERIFIER_nondet_uchar(void) {
  unsigned char x;
  klee_make_symbolic(&x, sizeof(x), "unsigned char");
  return x;
}

char *__VERIFIER_nondet_pchar(void) {
  char *x;
  klee_make_symbolic(&x, sizeof(x), "char *");
  return x;
}

long __VERIFIER_nondet_long(void) {
  long x;
  klee_make_symbolic(&x, sizeof(x), "long");
  return x;
}

unsigned long __VERIFIER_nondet_ulong(void) {
  unsigned long x;
  klee_make_symbolic(&x, sizeof(x), "unsigned long");
  return x;
}

double __VERIFIER_nondet_double(void) {
  double x;
  klee_make_symbolic(&x, sizeof(x), "double");
  return x;
}

void *__VERIFIER_nondet_pointer(void) {
  int size = 1024;
  char *obj = (char *)calloc(1, size);
  // klee_make_symbolic(obj, size, "obj");

  return obj;
}

float __VERIFIER_nondet_float(void) {
  float x;
  klee_make_symbolic(&x, sizeof(x), "float");
  return x;
}

_Bool __VERIFIER_nondet_bool(void) {
  _Bool x;
  klee_make_symbolic(&x, sizeof(x), "_Bool");
  klee_assume(x == 0 || x == 1);
  return x;
}

typedef unsigned int u32;
u32 __VERIFIER_nondet_u32(void) {
  u32 x;
  klee_make_symbolic(&x, sizeof(x), "u32");
  return x;
}

#ifndef EXTERNAL
typedef long long loff_t;
#endif
loff_t __VERIFIER_nondet_loff_t(void) {
  loff_t x;
  klee_make_symbolic(&x, sizeof(x), "loff_t");
  return x;
}

typedef unsigned long sector_t;
sector_t __VERIFIER_nondet_sector_t(void) {
  sector_t x;
  klee_make_symbolic(&x, sizeof(x), "sector_t");
  return x;
}

#ifndef EXTERNAL
typedef unsigned int __kernel_size_t;
typedef __kernel_size_t size_t;
#endif

size_t __VERIFIER_nondet_size_t(void) {
  size_t x;
  klee_make_symbolic(&x, sizeof(x), "size_t");
  return x;
}

#ifndef EXTERNAL
struct __pthread_t_struct {
  int id;
};
typedef struct __pthread_t_struct pthread_t;
#endif

pthread_t __VERIFIER_nondet_pthread_t(void) {
  pthread_t x;
  klee_make_symbolic(&x, sizeof(x), "pthread_t");
  return x;
}

void __VERIFIER_assume(int x) {
  if (!x)
    klee_silent_exit(0);
}

void __VERIFIER_error(void) {
#ifndef EXTERNAL
  __assert_fail("Failed", __FILE__, __LINE__, __PRETTY_FUNCTION__);
#else
  assert(0 && "Failure");
#endif
}
