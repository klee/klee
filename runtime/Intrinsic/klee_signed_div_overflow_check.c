//===-- klee_signed_div_overflow_check.c ----------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "klee/klee.h"

void klee_signed_div_overflow_check(_Bool res) {
  if (res) {
    klee_report_error(
      __FILE__, __LINE__, "signed overflow error (div)", "overflow.err");
  }
}