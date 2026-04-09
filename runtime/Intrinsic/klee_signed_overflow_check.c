//===-- klee_signed_overflow_check.c ----------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "klee/klee.h"

#define ERR_STR_SIZE 128

void klee_signed_overflow_check(_Bool overflow, const char *op) {
  char errorStrBuf[ERR_STR_SIZE];

  if (overflow) {

    snprintf(errorStrBuf, ERR_STR_SIZE, "signed overflow error (%s)", op);

    klee_report_error(__FILE__, __LINE__, errorStrBuf, "overflow.err");
  }
}