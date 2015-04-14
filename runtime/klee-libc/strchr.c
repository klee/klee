/*===-- strchr.c ----------------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===*/

char *strchr(const char *p, int ch) {
  char c;

  c = ch;
  for (;; ++p) {
    if (*p == c) {
      return ((char *)p);
    } else if (*p == '\0') {
      return 0;
    }
  }
  /* NOTREACHED */  
  return 0;
}
