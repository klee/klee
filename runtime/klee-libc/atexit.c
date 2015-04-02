/*==-- atexit.c ----------------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===*/

int __cxa_atexit(void (*fn)(void*),
                 void *arg,
                 void *dso_handle);

int atexit(void (*fn)(void)) {
  return __cxa_atexit((void(*)(void*)) fn, 0, 0);
}
