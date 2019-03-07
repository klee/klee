// RUN: %clang %s -emit-llvm %O0opt -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out %t.bc | FileCheck %s

#include <stdarg.h>
#include <assert.h>
#include <stdio.h>

void ld_after_zero(int first,...)
{
	va_list ap;
	long double dub;

	va_start(ap, first);

	while (va_arg(ap, int) != 0)
		;

	dub = va_arg(ap, long double);

	printf("%Lf\n", dub);

}

int main() {
  long double dub = 1.123456;
  int zero = 0;
  int one = 1;

  // AMD64-ABI 3.5.7p5: Step 7. Align l->overflow_arg_area upwards to a 16
  // byte boundary if alignment needed by type exceeds 8 byte boundary.
  // 
  // the long double dub requires 16 byte alignment.
  // we try passing one,zero and one, one, zero
  // at least on those will align dub such that it needs the extra alignment
  ld_after_zero(one, zero, dub);
  // CHECK: 1.123456
  ld_after_zero(one, one, zero, dub);
  // CHECK-NEXT: 1.123456

  return 0;
}
