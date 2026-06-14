/*===-- memcmp_bcmp.c -----------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===*/

#include <stddef.h>

int memcmp(const void *s1, const void *s2, size_t n) {
  const unsigned char *a = s1;
  const unsigned char *b = s2;

  while (n) {
    if (*a != *b)
      return *a - *b;
    n--;
    a++;
    b++;
  }

  return 0;
}

int bcmp(const void *s1, const void *s2, size_t n) { return memcmp(s1, s2, n); }
