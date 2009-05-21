// RUN: %llvmgcc -g -c -o %t.bc %s
// RUN: %klee --exit-on-error --use-asm-addresses %t.bc
// RUN: %llvmgcc -DOVERLAP -g -c -o %t.bc %s
// RUN: not %klee --exit-on-error --use-asm-addresses %t.bc

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
