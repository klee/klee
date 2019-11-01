//===-- fd_64.c -----------------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#if __GNUC__
#if __x86_64__ || __ppc64__
#define ENV64
#else
#define ENV32
#endif
#endif

#define INSIDE_FD_64
#define _LARGEFILE64_SOURCE
#define _FILE_OFFSET_BITS 64
#include "fd.h"

#include "klee/Config/Version.h"
#include "klee/klee.h"

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/types.h>
#ifndef __FreeBSD__
#include <sys/vfs.h>
#endif
#include <dirent.h>
#include <sys/ioctl.h>
#include <sys/mtio.h>
#include <sys/select.h>
#include <termios.h>
#include <unistd.h>

/*** Forward to actual implementations ***/

int open(const char *pathname, int flags, ...) {
  mode_t mode = 0;

  if (flags & O_CREAT) {
    /* get mode */
    va_list ap;
    va_start(ap, flags);
    mode = va_arg(ap, int);
    va_end(ap);
  }

  return __fd_open(pathname, flags, mode);
}

int openat(int fd, const char *pathname, int flags, ...) {
  mode_t mode = 0;

  if (flags & O_CREAT) {
    /* get mode */
    va_list ap;
    va_start(ap, flags);
    mode = va_arg(ap, int);
    va_end(ap);
  }

  return __fd_openat(fd, pathname, flags, mode);
}

off64_t lseek(int fd, off64_t offset, int whence) {
  return __fd_lseek(fd, offset, whence);
}

int __xstat(int vers, const char *path, struct stat *buf) {
  return __fd_stat(path, (struct stat64*) buf);
}

int stat(const char *path, struct stat *buf) {
  return __fd_stat(path, (struct stat64*) buf);
}

int __lxstat(int vers, const char *path, struct stat *buf) {
  return __fd_lstat(path, (struct stat64*) buf);
}

int lstat(const char *path, struct stat *buf) {
  return __fd_lstat(path, (struct stat64*) buf);
}

int __fxstat(int vers, int fd, struct stat *buf) {
  return __fd_fstat(fd, (struct stat64*) buf);
}

int fstat(int fd, struct stat *buf) {
  return __fd_fstat(fd, (struct stat64*) buf);
}

int ftruncate64(int fd, off64_t length) {
  return __fd_ftruncate(fd, length);
}

int statfs(const char *path, struct statfs *buf) __attribute__((weak));
int statfs(const char *path, struct statfs *buf) {
  return __fd_statfs(path, buf);
}

ssize_t getdents64(int fd, void *dirp, size_t count) {
  return __fd_getdents(fd, (struct dirent64*) dirp, count);
}
ssize_t __getdents64(int fd, void *dirp, size_t count)
     __attribute__((alias("getdents64")));
