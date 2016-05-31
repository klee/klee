//===-- klee_mark_arg_symbolic.c
//-------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include <klee/klee.h>

/* avoid having to compile the POSIX runtime to use this feature */
static int my_strlen(char *s) {
  int i = 0;
  while (s[i++])
    ;
  return i - 1;
}

void klee_mark_arg_symbolic(int argc, char **argv) {
  int i, len;
  for (i = 1; i < argc; ++i) {
    len = my_strlen(argv[i]);
    klee_make_symbolic(argv[i], len + 1, "argv");
    klee_assume((char)argv[i][len] == 0);
  }
}
