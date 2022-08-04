/*===-- __cxa_atexit.c ----------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===*/

#include "klee/klee.h"

#define MAX_ATEXIT 128

static struct {
  void (*fn)(void*);
  void *arg;
  void *dso_handle;
} AtExit[MAX_ATEXIT];
static unsigned NumAtExit = 0;

static void RunAtExit(void) __attribute__((destructor));
static void RunAtExit(void) {
  unsigned i;

  for (i = NumAtExit - 1; i < MAX_ATEXIT; --i)
    AtExit[i].fn(AtExit[i].arg);
}

int __cxa_atexit(void (*fn)(void*),
                 void *arg,
                 void *dso_handle) {
  klee_warning_once("FIXME: __cxa_atexit being ignored");
  
  /* Better to just report an error here than return 1 (the defined
   * semantics).
   */
  if (NumAtExit == MAX_ATEXIT)
    klee_report_error(__FILE__,
                      __LINE__,
                      "__cxa_atexit: no room in array!",
                      "exec.err");
  
  AtExit[NumAtExit].fn = fn;
  AtExit[NumAtExit].arg = arg;
  ++NumAtExit;
  
  return 0;
}

// This variant is part of more recent glibc versions and
// is required by the Rust standard library
int __cxa_thread_atexit_impl(void (*fn)(void*), void *arg, void *dso_handle) {
  return __cxa_atexit(fn, arg, dso_handle);
}
