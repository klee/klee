// RUN: %llvmgcc -emit-llvm -g -c -o %t.bc %s
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --exit-on-error --use-asm-addresses %t.bc

// RUN: %llvmgcc -emit-llvm -DOVERLAP -g -c -o %t.bc %s
// RUN: rm -rf %t.klee-out
// RUN: not %klee --output-dir=%t.klee-out --exit-on-error --use-asm-addresses %t.bc

#include <assert.h>


volatile unsigned char x0 __asm ("0x0021");
volatile unsigned char whee __asm ("0x0WHEE");

#ifdef OVERLAP
volatile unsigned int y0 __asm ("0x0030");
volatile unsigned int y1 __asm ("0x0032");
#endif

int main() {
  assert(&x0 == (void*) 0x0021);
  assert(&whee != (void*) 0x0);

  return 0;
}
