/*===-- strrchr.c ---------------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===*/

#include <string.h>

char *strrchr(const char *t, int c) {
  char ch;
  const char *l=0;

  ch = c;
  for (;;) {
    if (*t == ch) l=t; if (!*t) return (char*)l; ++t;
  }
  return (char*)l;
}
