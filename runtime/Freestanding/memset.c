/*===-- memset.c ----------------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===*/

#include <stdlib.h>

char __klee_handle_memset(void *, int, size_t);

void *memset(void *dst, int s, size_t count) {
  if (__klee_handle_memset(dst, s, count))
    return dst;
  char *a = dst;
  while (count-- > 0)
    *a++ = s;
  return dst;
}
