//===-- fd_32.c -----------------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
// Contains 32bit definitions of posix file functions
//===---

#if __GNUC__
#if __x86_64__ || __ppc64__
#define ENV64
#else
#define ENV32
#endif
#endif

#include "klee/Config/Version.h"
#define _LARGEFILE64_SOURCE
#include "fd.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/syscall.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#ifndef __FreeBSD__
#include <sys/vfs.h>
#endif
#include <fcntl.h>
#include <stdarg.h>
#include <assert.h>
#include <unistd.h>
#include <dirent.h>

/***/

static void __stat64_to_stat(struct stat64 *a, struct stat *b) {
  b->st_dev = a->st_dev;
  b->st_ino = a->st_ino;
  b->st_mode = a->st_mode;
  b->st_nlink = a->st_nlink;
  b->st_uid = a->st_uid;
  b->st_gid = a->st_gid;
  b->st_rdev = a->st_rdev;
  b->st_size = a->st_size;
  b->st_atime = a->st_atime;
  b->st_mtime = a->st_mtime;
  b->st_ctime = a->st_ctime;
  b->st_blksize = a->st_blksize;
  b->st_blocks = a->st_blocks;
#ifdef _BSD_SOURCE
  b->st_atim.tv_nsec = a->st_atim.tv_nsec;
  b->st_mtim.tv_nsec = a->st_mtim.tv_nsec;
  b->st_ctim.tv_nsec = a->st_ctim.tv_nsec;
#endif

}

/***/

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

off_t lseek(int fd, off_t off, int whence) {
  return (off_t) __fd_lseek(fd, off, whence);
}

int __xstat(int vers, const char *path, struct stat *buf) {
  struct stat64 tmp;
  int res = __fd_stat(path, &tmp);
  __stat64_to_stat(&tmp, buf);
  return res;
}

int stat(const char *path, struct stat *buf) {
  struct stat64 tmp;
  int res = __fd_stat(path, &tmp);
  __stat64_to_stat(&tmp, buf);
  return res;
}

int __lxstat(int vers, const char *path, struct stat *buf) {
  struct stat64 tmp;
  int res = __fd_lstat(path, &tmp);
  __stat64_to_stat(&tmp, buf);
  return res;
}

int lstat(const char *path, struct stat *buf) {
  struct stat64 tmp;
  int res = __fd_lstat(path, &tmp);
  __stat64_to_stat(&tmp, buf);
  return res;
}

int __fxstat(int vers, int fd, struct stat *buf) {
  struct stat64 tmp;
  int res = __fd_fstat(fd, &tmp);
  __stat64_to_stat(&tmp, buf);
  return res;
}

int fstat(int fd, struct stat *buf) {
  struct stat64 tmp;
  int res = __fd_fstat(fd, &tmp);
  __stat64_to_stat(&tmp, buf);
  return res;
}

int ftruncate(int fd, off_t length) {
  return __fd_ftruncate(fd, length);
}

int statfs(const char *path, struct statfs *buf32) {
#if 0
    struct statfs64 buf;

    if (__fd_statfs(path, &buf) < 0)
	return -1;

    buf32->f_type = buf.f_type;
    buf32->f_bsize = buf.f_bsize;
    buf32->f_blocks = buf.f_blocks;
    buf32->f_bfree = buf.f_bfree;
    buf32->f_bavail = buf.f_bavail;
    buf32->f_files = buf.f_files;
    buf32->f_ffree = buf.f_ffree;
    buf32->f_fsid = buf.f_fsid;
    buf32->f_namelen = buf.f_namelen;

    return 0;
#else
    return __fd_statfs(path, buf32);
#endif
}

/* Based on uclibc version. We use getdents64 and then rewrite the
   results over themselves, as dirent32s. */
#ifndef __FreeBSD__
ssize_t getdents(int fd, struct dirent *dirp, size_t nbytes) {
#else
#if __FreeBSD__ > 11
ssize_t getdents(int fd, char *dirp, size_t nbytes) {
#else
int getdents(int fd, char *dirp, int nbytes) {
#endif
#endif
  struct dirent64 *dp64 = (struct dirent64*) dirp;
  ssize_t res = __fd_getdents(fd, dp64, nbytes);

  if (res>0) {
    struct dirent64 *end = (struct dirent64*) ((char*) dp64 + res);
    while (dp64 < end) {
      struct dirent *dp = (struct dirent *) dp64;
      size_t name_len = (dp64->d_reclen -
                           (size_t) &((struct dirent64*) 0)->d_name);
      dp->d_ino = dp64->d_ino;
#ifdef _DIRENT_HAVE_D_OFF
      dp->d_off = dp64->d_off;
#endif
      dp->d_reclen = dp64->d_reclen;
      dp->d_type = dp64->d_type;
      memmove(dp->d_name, dp64->d_name, name_len);
      dp64 = (struct dirent64*) ((char*) dp64 + dp->d_reclen);
    }
  }

  return res;
}
int __getdents(unsigned int fd, struct dirent *dirp, unsigned int count)
     __attribute__((alias("getdents")));

/* Forward to 64 versions (uclibc expects versions w/o asm specifier) */

__attribute__((weak)) int open64(const char *pathname, int flags, ...) {
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
