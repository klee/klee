//===-- strcoll.c ---------------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include <string.h>

// according to the manpage, this is equiv in the POSIX/C locale.
int strcoll(const char *s1, const char *s2) {
  return strcmp(s1,s2);
}
