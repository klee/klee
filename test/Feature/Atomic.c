// RUN: %clang %s -emit-llvm -g -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --exit-on-error %t.bc 2>%t.log
// RUN: cat %t.klee-out/assembly.ll | FileCheck %s

// Checks that KLEE lowers atomic instructions to non-atomic operations

#include <assert.h>
#include <stdatomic.h>

void test_add() {
  atomic_int a = 7;
  atomic_fetch_add(&a, 7);
  // CHECK-NOT: atomicrmw add
  assert(a == 14);
}

void test_sub() {
  atomic_int a = 7;
  atomic_fetch_sub(&a, 7);
  // CHECK-NOT: atomicrmw sub
  assert(a == 0);
}

void test_and() {
  atomic_int a = 0b10001;
  atomic_fetch_and(&a, 0b00101);
  // CHECK-NOT: atomicrmw and
  assert(a == 0b00001);
}

void test_or() {
  atomic_int a = 0b10001;
  atomic_fetch_or(&a, 0b00101);
  // CHECK-NOT: atomicrmw or
  assert(a == 0b10101);
}

void test_xor() {
  atomic_int a = 0b10001;
  atomic_fetch_xor(&a, 0b00101);
  // CHECK-NOT: atomicrmw xor
  assert(a == 0b10100);
}

void test_xchg() {
  atomic_int a = 7;
  atomic_exchange(&a, 10);
  // CHECK-NOT: atomicrmw xchg
  assert(a == 10);
}

void test_cmp_xchg() {
  atomic_int a = 7;
  int b = 7;
  atomic_compare_exchange_weak(&a, &b, 10);
  // CHECK-NOT: cmpxchg weak
  assert(a == 10);
  assert(b == 7);
}

int main() {
  test_and();
  test_sub();
  test_and();
  test_or();
  test_xor();
  test_xchg();
  test_cmp_xchg();
  return 0;
}
