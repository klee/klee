//===-- fortify.c ---------------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "klee/klee.h"

#include <limits.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

char *__fgets_chk(char *s, size_t size, int strsize, FILE *stream) {
  return fgets(s, size, stream);
}

ssize_t __pread_chk(int fd, void *buf, size_t nbytes, off_t offset,
                    size_t buflen) {
  if (nbytes > buflen)
    klee_report_error(__FILE__, __LINE__, "pread overflow", "ptr.err");

  return pread(fd, buf, nbytes, offset);
}

ssize_t __read_chk(int fd, void *buf, size_t nbytes, size_t buflen) {
  if (nbytes > buflen)
    klee_report_error(__FILE__, __LINE__, "read overflow", "ptr.err");

  return read(fd, buf, nbytes);
}

ssize_t __readlink_chk(const char *path, char *buf, size_t len, size_t buflen) {
  if (len > buflen)
    klee_report_error(__FILE__, __LINE__, "readlink overflow", "ptr.err");

  return readlink(path, buf, len);
}

char *__realpath_chk(const char *path, char *resolved_path,
                     size_t resolved_len) {
  if (resolved_len < PATH_MAX)
    klee_report_error(__FILE__, __LINE__, "realpath overflow", "ptr.err");

  return realpath(path, resolved_path);
}

ssize_t __recv_chk(int fd, void *buf, size_t len, size_t buflen, int flag) {
  if (len > buflen)
    klee_report_error(__FILE__, __LINE__, "recv overflow", "ptr.err");

  return recv(fd, buf, len, flag);
}

ssize_t __recvfrom_chk(int fd, void *buf, size_t len, size_t buflen, int flag,
                       struct sockaddr *from, socklen_t *fromlen) {
  if (len > buflen)
    klee_report_error(__FILE__, __LINE__, "recvfrom overflow", "ptr.err");

  return recvfrom(fd, buf, len, flag, from, fromlen);
}

char *__stpncpy_chk(char *dest, const char *src, size_t n, size_t destlen) {
  if (n > destlen)
    klee_report_error(__FILE__, __LINE__, "stpncpy overflow", "ptr.err");
  return stpncpy(dest, src, n);
}

char *__strncat_chk(char *s1, const char *s2, size_t n, size_t s1len) {
  return strncat(s1, s2, n);
}

int __ttyname_r_chk(int fd, char *buf, size_t buflen, size_t nreal) {
  if (buflen > nreal)
    klee_report_error(__FILE__, __LINE__, "ttyname_r overflow", "ptr.err");
  return ttyname_r(fd, buf, nreal);
}
