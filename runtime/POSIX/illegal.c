//===-- illegal.c ---------------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "klee/klee.h"

#include <errno.h>
#include <setjmp.h>
#include <signal.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

void klee_warning(const char*);
void klee_warning_once(const char*);

int kill(pid_t pid, int sig) {
  klee_warning("ignoring (EPERM)");
  errno = EPERM;
  return -1;
}

#ifndef __FreeBSD__
int _setjmp (struct __jmp_buf_tag __env[1]) __attribute__((weak));
int _setjmp (struct __jmp_buf_tag __env[1]) {
#else
int _setjmp (jmp_buf env) __returns_twice;
int _setjmp (jmp_buf env) {
#endif
  klee_warning_once("ignoring");
  return 0;
}

void longjmp(jmp_buf env, int val) {
  klee_report_error(__FILE__, __LINE__, "longjmp unsupported", "xxx.err");
}

/* Macro so function name from klee_warning comes out correct. */
#define __bad_exec() \
  (klee_warning("ignoring (EACCES)"),\
   errno = EACCES,\
   -1)

/* This need to be weak because uclibc wants to define them as well,
   but we will want to make sure a definition is around in case we
   don't link with it. */

int execl(const char *path, const char *arg, ...) __attribute__((weak));
int execlp(const char *file, const char *arg, ...) __attribute__((weak));
int execle(const char *path, const char *arg, ...) __attribute__((weak));
int execv(const char *path, char *const argv[]) __attribute__((weak));
int execvp(const char *file, char *const argv[]) __attribute__((weak));
int execve(const char *file, char *const argv[], char *const envp[]) __attribute__((weak));

int execl(const char *path, const char *arg, ...) { return __bad_exec(); }
int execlp(const char *file, const char *arg, ...) { return __bad_exec(); }
int execle(const char *path, const char *arg, ...)  { return __bad_exec(); }
int execv(const char *path, char *const argv[]) { return __bad_exec(); }
int execvp(const char *file, char *const argv[]) { return __bad_exec(); }
int execve(const char *file, char *const argv[], char *const envp[]) { return __bad_exec(); }

pid_t fork(void) {
  klee_warning("ignoring (ENOMEM)");
  errno = ENOMEM;
  return -1;
}

pid_t vfork(void) {
  return fork();
}
