//===-- functions.c           ---------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "klee/klee_svcomp.h"

#define SVCOMP_NONDET_DEFN_D(NAME,T) \
T __VERIFIER_nondet_ ## NAME() { \
  T initialValue; \
  klee_make_symbolic(&initialValue,\
                     sizeof(T),\
                     "svcomp.sym" # T);\
  return initialValue; \
}

#define SVCOMP_NONDET_DEFN(NAME) SVCOMP_NONDET_DEFN_D(NAME, NAME)

#define SVCOMP_NONDET_DEFN_NOT_SUPPORTED(NAME) \
    SVCOMP_NONDET_DEFN_D_NOT_SUPPORTED(NAME, NAME)

SVCOMP_NONDET_DEFN_D(bool,_Bool)
SVCOMP_NONDET_DEFN(char)
SVCOMP_NONDET_DEFN(int)
SVCOMP_NONDET_DEFN(long)
SVCOMP_NONDET_DEFN(loff_t)
SVCOMP_NONDET_DEFN_D(pointer,void*)
SVCOMP_NONDET_DEFN_D(pchar,char*)
SVCOMP_NONDET_DEFN(pthread_t)
SVCOMP_NONDET_DEFN(sector_t)
SVCOMP_NONDET_DEFN(short)
SVCOMP_NONDET_DEFN(size_t)
SVCOMP_NONDET_DEFN_D(u32, uint32_t)
SVCOMP_NONDET_DEFN_D(uchar,unsigned char)
SVCOMP_NONDET_DEFN_D(uint, unsigned int)
SVCOMP_NONDET_DEFN_D(ulong, unsigned long)
SVCOMP_NONDET_DEFN(unsigned)
SVCOMP_NONDET_DEFN_D(ushort, unsigned short)

float __VERIFIER_nondet_float() {
  union {
    uint32_t asBits;
    float asFloat;
  } data;
  klee_warning("Symbolic data of type float not supported. Returning quiet NaN");
  data.asBits = 0x7fc00000;
  return data.asFloat;
}

void __VERIFIER_assume(int condition) {
  // Guard the condition we assume so that if it is not
  // satisfiable we don't flag up an error. Instead we'll
  // just silently terminate this state.
  if (condition) {
    klee_assume(condition);
  } else {
    klee_silent_exit(0);
  }
}

void __VERIFIER_assert(int cond) {
  if (!cond) {
    klee_report_error(__FILE__, __LINE__ ,
                      "svcomp assert failed",
                      "svcomp.assertfail");
  }
}

__attribute__ ((__noreturn__)) void __VERIFIER_error()  {
  klee_report_error(__FILE__, __LINE__,"svcomp error", "svcomp.err");
}

void __VERIFIER_atomic_begin() {
  klee_warning("__VERIFIER_atomic_begin() is a no-op");
}

void __VERIFIER_atomic_end() {
  klee_warning("__VERIFIER_atomic_end() is a no-op");
}
