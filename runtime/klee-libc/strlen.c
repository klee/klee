/*===-- strlen.c ----------------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===*/

#include <string.h>

size_t strlen(const char *str) {
  const char *s = str;
  while (*s)
    ++s;
  return s - str;
}
