/*===-- strcmp.c ----------------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===*/

int strcmp(const char *a, const char *b) {
  while (*a && *a == *b)
    ++a, ++b;
  return *a - *b;
}
