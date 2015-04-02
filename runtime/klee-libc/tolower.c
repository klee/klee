/*===-- tolower.c ---------------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===*/

int tolower(int ch) {
  if ( (unsigned int)(ch - 'A') < 26u )
    ch -= 'A' - 'a';
  return ch;
}
