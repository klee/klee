/*===-- mempcpy.c ---------------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===*/

#include <stddef.h>

void *memcpy(void *destaddr, void const *srcaddr, size_t len);

void *mempcpy(void *destaddr, void const *srcaddr, size_t len) {
  return (char *)memcpy(destaddr, srcaddr, len) + len;
}
