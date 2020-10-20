/*===-- memcpy.c ----------------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===*/

#include <stdlib.h>

char __klee_handle_memcpy(void *, void const *, size_t);

void *memcpy(void *destaddr, void const *srcaddr, size_t len) {
  if (__klee_handle_memcpy(destaddr, srcaddr, len))
    return destaddr;
  char *dest = destaddr;
  char const *src = srcaddr;

  while (len-- > 0)
    *dest++ = *src++;
  return destaddr;
}
