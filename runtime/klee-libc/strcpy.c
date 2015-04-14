/*===-- strcpy.c ----------------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===*/

char *strcpy(char *to, const char *from) {
  char *start = to;

  while ((*to = *from))
    ++to, ++from;

  return start;
}
