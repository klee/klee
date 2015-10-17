// // Test for 32-bit
// RUN: %llvmgcc %s -emit-llvm -m32 -g -std=c99 -c -o %t_32.bc
// RUN: rm -rf %t.klee-out_32
// RUN: %klee --output-dir=%t.klee-out_32 --svcomp-runtime %t_32.bc > %t.kleeoutput.32.log 2>&1
// RUN: FileCheck -check-prefix=first-CHECK -input-file=%t.kleeoutput.32.log %s
// RUN: FileCheck -check-prefix=second-CHECK -input-file=%t.kleeoutput.32.log %s
// RUN: not test -f %t.klee-out_32/test*.err
// Test again for 64-bit
// RUN: %llvmgcc %s -emit-llvm -m64 -g -std=c99 -c -o %t_64.bc
// RUN: rm -rf %t.klee-out_64
// RUN: %klee --output-dir=%t.klee-out_64 --svcomp-runtime %t_64.bc > %t.kleeoutput.64.log 2>&1
// RUN: FileCheck -check-prefix=first-CHECK -input-file=%t.kleeoutput.64.log %s
// RUN: FileCheck -check-prefix=second-CHECK -input-file=%t.kleeoutput.64.log %s
// RUN: not test -f %t.klee-out_64/test*.err

// first-CHECK: completed paths = 1
// second-CHECK-NOT: ERROR
// second-CHECK-NOT: abort
#include "klee/klee.h"
#include "klee/klee_svcomp.h"

#define test_call_D(NAME,T) \
{ \
  T initialValue = __VERIFIER_nondet_ ## NAME(); \
  if (!klee_is_symbolic(initialValue)) { \
    klee_warning("Failed to get symbolic value for type " # T); \
    klee_abort(); \
  } \
}

#define test_call_ptr_D(NAME,T) \
{ \
  T initialValue = __VERIFIER_nondet_ ## NAME(); \
  if (!klee_is_symbolic((uintptr_t) initialValue)) { \
    klee_warning("Failed to get symbolic value for type " # T); \
    klee_abort(); \
  } \
}

#define test_call(NAME) test_call_D(NAME,NAME)
#define test_call_ptr(NAME) test_call_ptr_D(NAME,NAME)

int main() {
  test_call_D(bool,_Bool);
  test_call(char);
  {
    float initialValue = __VERIFIER_nondet_float();
    if (klee_is_symbolic(initialValue)) {
      klee_warning("Did not expect float to be symbolic");
      klee_abort();
    }
  }
  test_call(int)
  test_call(long)
  test_call(loff_t)
  test_call_ptr_D(pointer,void*)
  test_call_ptr_D(pchar,char*)
  {
    pthread_t initialValue = __VERIFIER_nondet_pthread_t();
    if (!klee_is_symbolic(initialValue.id)) {
    klee_warning("Failed to get symbolic value for id field");
    klee_abort();
    }
  }
  test_call(sector_t)
  test_call(short)
  test_call(size_t)
  test_call_D(u32, uint32_t)
  test_call_D(uchar,unsigned char)
  test_call_D(uint, unsigned int)
  test_call_D(ulong, unsigned long)
  test_call(unsigned)
  test_call_D(ushort, unsigned short)
  return 0;
}
