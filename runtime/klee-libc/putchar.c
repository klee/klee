/*===-- putchar.c ---------------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===*/

#include <stddef.h>
#include <stdint.h>

intptr_t write(int fildes, const void *buf, size_t nbyte);

#define EOF -1

int putchar(int c) {
  char x = c;
  if (1 == write(1, &x, 1))
    return c;
  return EOF;
}
