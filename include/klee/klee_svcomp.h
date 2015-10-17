//===-- klee_svomp.h           ---------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

// This file declares the SV-COMP 2016 functions
// as defined in http://sv-comp.sosy-lab.org/2016/rules.php

#include "klee/klee.h"
#include "stdint.h"

/* Start SV-COMP specific type declarations
 *
 * Taken from ``ddv-machzwd/ddv_machzwd_all_false-unreach-call.i``. Not really
 * sure if it's applicable everywhere.
 */
typedef long long loff_t;
typedef unsigned long sector_t;
struct __pthread_t_struct
{
      int id;
};
typedef struct __pthread_t_struct pthread_t;

/* End SV-COMP specific type declarations */

#define SVCOMP_NONDET_DECL_D(NAME,T) \
  T __VERIFIER_nondet_ ## NAME();

#define SVCOMP_NONDET_DECL(NAME) SVCOMP_NONDET_DECL_D(NAME,NAME)

SVCOMP_NONDET_DECL_D(bool,_Bool)
SVCOMP_NONDET_DECL(char)
SVCOMP_NONDET_DECL(float)
SVCOMP_NONDET_DECL(int)
SVCOMP_NONDET_DECL(long)
SVCOMP_NONDET_DECL(loff_t)
SVCOMP_NONDET_DECL_D(pointer,void*)
SVCOMP_NONDET_DECL_D(pchar,char*)
SVCOMP_NONDET_DECL(pthread_t)
SVCOMP_NONDET_DECL(sector_t)
SVCOMP_NONDET_DECL(short)
SVCOMP_NONDET_DECL(size_t)
SVCOMP_NONDET_DECL_D(u32, uint32_t)
SVCOMP_NONDET_DECL_D(uchar,unsigned char)
SVCOMP_NONDET_DECL_D(uint, unsigned int)
SVCOMP_NONDET_DECL_D(ulong, unsigned long)
SVCOMP_NONDET_DECL(unsigned)
SVCOMP_NONDET_DECL_D(ushort, unsigned short)

#undef SVCOMP_NONDET_D_DECL
#undef SVCOMP_NONDET_DECL

void __VERIFIER_assume(int condition);
void __VERIFIER_assert(int cond);
__attribute__ ((__noreturn__)) void __VERIFIER_error();
void __VERIFIER_atomic_begin();
void __VERIFIER_atomic_end();
