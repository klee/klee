//===-- putchar.c ---------------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include <stdio.h>
#include <unistd.h>

// Some header may #define putchar.
#undef putchar

int putchar(int c) {
  char x = c;
  write(1, &x, 1);
  return 1;
}
