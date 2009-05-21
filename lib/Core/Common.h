//===-- Common.h ------------------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef __KLEE_COMMON_H__
#define __KLEE_COMMON_H__

#ifdef __CYGWIN__
#ifndef WINDOWS
#define WINDOWS
#endif
#endif

#include <stdio.h>

// XXX ugh
namespace klee {
  class Solver;

  extern FILE* klee_warning_file;
  extern FILE* klee_message_file;

  /// Print "KLEE: ERROR" followed by the msg in printf format and a
  /// newline on stderr and to warnings.txt, then exit with an error.
  void klee_error(const char *msg, ...)
    __attribute__ ((format (printf, 1, 2), noreturn));

  /// Print "KLEE: " followed by the msg in printf format and a
  /// newline on stderr and to messages.txt.
  void klee_message(const char *msg, ...)
    __attribute__ ((format (printf, 1, 2)));

  /// Print "KLEE: " followed by the msg in printf format and a
  /// newline to messages.txt.
  void klee_message_to_file(const char *msg, ...)
    __attribute__ ((format (printf, 1, 2)));

  /// Print "KLEE: WARNING" followed by the msg in printf format and a
  /// newline on stderr and to warnings.txt.
  void klee_warning(const char *msg, ...)
    __attribute__ ((format (printf, 1, 2)));

  /// Print "KLEE: WARNING" followed by the msg in printf format and a
  /// newline on stderr and to warnings.txt. However, the warning is only 
  /// printed once for each unique (id, msg) pair (as pointers).
  void klee_warning_once(const void *id,
                         const char *msg, ...)
    __attribute__ ((format (printf, 2, 3)));
}

#endif /* __KLEE_COMMON_H__ */
