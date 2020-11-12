//===-- fortify-fs.c ------------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

/* Fortified versions of the libc functions defined in the FreeStanding library
 */

#include "klee/klee.h"

#include <string.h>

void *__memmove_chk(void *dest, const void *src, size_t len, size_t destlen) {
  if (len > destlen)
    klee_report_error(__FILE__, __LINE__, "memmove overflow", "ptr.err");

  return memmove(dest, src, len);
}

void *__memset_chk(void *dest, int c, size_t len, size_t destlen) {
  if (len > destlen)
    klee_report_error(__FILE__, __LINE__, "memset overflow", "ptr.err");

  return memset(dest, c, len);
}

void *__memcpy_chk(void *dest, const void *src, size_t len, size_t destlen) {
  if (len > destlen)
    klee_report_error(__FILE__, __LINE__, "memcpy overflow", "ptr.err");

  return memcpy(dest, src, len);
}
