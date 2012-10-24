//===-- stubs.c -----------------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <time.h>
#include <utime.h>
#include <utmp.h>
#include <unistd.h>
#include <limits.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/resource.h>
#include <sys/times.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "klee/Config/config.h"

void klee_warning(const char*);
void klee_warning_once(const char*);

/* Silent ignore */

int __syscall_rt_sigaction(int signum, const struct sigaction *act, 
                           struct sigaction *oldact, size_t _something)
     __attribute__((weak));

int __syscall_rt_sigaction(int signum, const struct sigaction *act, 
                           struct sigaction *oldact, size_t _something) {
  klee_warning_once("silently ignoring");
  return 0;
}

int sigaction(int signum, const struct sigaction *act, 
              struct sigaction *oldact) __attribute__((weak));

int sigaction(int signum, const struct sigaction *act, 
              struct sigaction *oldact) {
  klee_warning_once("silently ignoring");
  return 0;
}

int sigprocmask(int how, const sigset_t *set, sigset_t *oldset)
     __attribute__((weak));
int sigprocmask(int how, const sigset_t *set, sigset_t *oldset) {
  klee_warning_once("silently ignoring");
  return 0;
}

/* Not even worth warning about these */
int fdatasync(int fd) __attribute__((weak));
int fdatasync(int fd) {
  return 0;
}

/* Not even worth warning about this */
void sync(void) __attribute__((weak));
void sync(void) {
}

/* Error ignore */

extern int __fgetc_unlocked(FILE *f);
extern int __fputc_unlocked(int c, FILE *f);

int __socketcall(int type, int *args) __attribute__((weak));
int __socketcall(int type, int *args) {
  klee_warning("ignoring (EAFNOSUPPORT)");
  errno = EAFNOSUPPORT;
  return -1;
}

int _IO_getc(FILE *f) __attribute__((weak));
int _IO_getc(FILE *f) {
  return __fgetc_unlocked(f);
}

int _IO_putc(int c, FILE *f) __attribute__((weak));
int _IO_putc(int c, FILE *f) {
  return __fputc_unlocked(c, f);
}

int mkdir(const char *pathname, mode_t mode) __attribute__((weak));
int mkdir(const char *pathname, mode_t mode) {
  klee_warning("ignoring (EIO)");
  errno = EIO;
  return -1;
}

int mkfifo(const char *pathname, mode_t mode) __attribute__((weak));
int mkfifo(const char *pathname, mode_t mode) {
  klee_warning("ignoring (EIO)");
  errno = EIO;
  return -1;
}

int mknod(const char *pathname, mode_t mode, dev_t dev) __attribute__((weak));
int mknod(const char *pathname, mode_t mode, dev_t dev) {
  klee_warning("ignoring (EIO)");
  errno = EIO;
  return -1;
}

int pipe(int filedes[2]) __attribute__((weak));
int pipe(int filedes[2]) {
  klee_warning("ignoring (ENFILE)");
  errno = ENFILE;
  return -1;
}

int link(const char *oldpath, const char *newpath) __attribute__((weak));
int link(const char *oldpath, const char *newpath) {
  klee_warning("ignoring (EPERM)");
  errno = EPERM;
  return -1;  
}

int symlink(const char *oldpath, const char *newpath) __attribute__((weak));
int symlink(const char *oldpath, const char *newpath) {
  klee_warning("ignoring (EPERM)");
  errno = EPERM;
  return -1;  
}

int rename(const char *oldpath, const char *newpath) __attribute__((weak));
int rename(const char *oldpath, const char *newpath) {
  klee_warning("ignoring (EPERM)");
  errno = EPERM;
  return -1;  
}

int nanosleep(const struct timespec *req, struct timespec *rem) __attribute__((weak));
int nanosleep(const struct timespec *req, struct timespec *rem) {
  return 0;
}

/* XXX why can't I call this internally? */
int clock_gettime(clockid_t clk_id, struct timespec *res) __attribute__((weak));
int clock_gettime(clockid_t clk_id, struct timespec *res) {
  /* Fake */
  struct timeval tv;
  gettimeofday(&tv, NULL);
  res->tv_sec = tv.tv_sec;
  res->tv_nsec = tv.tv_usec * 1000;
  return 0;
}

int clock_settime(clockid_t clk_id, const struct timespec *res) __attribute__((weak));
int clock_settime(clockid_t clk_id, const struct timespec *res) {
  klee_warning("ignoring (EPERM)");
  errno = EPERM;
  return -1;
}

time_t time(time_t *t) {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  if (t)
    *t = tv.tv_sec;
  return tv.tv_sec;
}

clock_t times(struct tms *buf) {
  /* Fake */
  buf->tms_utime = 0;
  buf->tms_stime = 0;
  buf->tms_cutime = 0;
  buf->tms_cstime = 0;
  return 0;
}

struct utmpx *getutxent(void) __attribute__((weak));
struct utmpx *getutxent(void) {
  return (struct utmpx*) getutent();
}

void setutxent(void) __attribute__((weak));
void setutxent(void) {
  setutent();
}

void endutxent(void) __attribute__((weak));
void endutxent(void) {
  endutent();
}

int utmpxname(const char *file) __attribute__((weak));
int utmpxname(const char *file) {
  utmpname(file);
  return 0;
}

int euidaccess(const char *pathname, int mode) __attribute__((weak));
int euidaccess(const char *pathname, int mode) {
  return access(pathname, mode);
}

int eaccess(const char *pathname, int mode) __attribute__((weak));
int eaccess(const char *pathname, int mode) {
  return euidaccess(pathname, mode);
}

int group_member (gid_t __gid) __attribute__((weak));
int group_member (gid_t __gid) {
  return ((__gid == getgid ()) || (__gid == getegid ()));
}

int utime(const char *filename, const struct utimbuf *buf) __attribute__((weak));
int utime(const char *filename, const struct utimbuf *buf) {
  klee_warning("ignoring (EPERM)");
  errno = EPERM;
  return -1;
}

int utimes(const char *filename, const struct timeval times[2]) __attribute__((weak));
int utimes(const char *filename, const struct timeval times[2]) {
  klee_warning("ignoring (EPERM)");
  errno = EPERM;
  return -1;
}

int futimes(int fd, const struct timeval times[2]) __attribute__((weak));
int futimes(int fd, const struct timeval times[2]) {
  klee_warning("ignoring (EBADF)");
  errno = EBADF;
  return -1;
}

int strverscmp (__const char *__s1, __const char *__s2) {
  return strcmp(__s1, __s2); /* XXX no doubt this is bad */
}

unsigned int gnu_dev_major(unsigned long long int __dev) __attribute__((weak));
unsigned int gnu_dev_major(unsigned long long int __dev) {
  return ((__dev >> 8) & 0xfff) | ((unsigned int) (__dev >> 32) & ~0xfff);
}

unsigned int gnu_dev_minor(unsigned long long int __dev) __attribute__((weak));
unsigned int gnu_dev_minor(unsigned long long int __dev) {
  return (__dev & 0xff) | ((unsigned int) (__dev >> 12) & ~0xff);
}

unsigned long long int gnu_dev_makedev(unsigned int __major, unsigned int __minor) __attribute__((weak));
unsigned long long int gnu_dev_makedev(unsigned int __major, unsigned int __minor) {
  return ((__minor & 0xff) | ((__major & 0xfff) << 8)
	  | (((unsigned long long int) (__minor & ~0xff)) << 12)
	  | (((unsigned long long int) (__major & ~0xfff)) << 32));
}

char *canonicalize_file_name (const char *name) __attribute__((weak));
char *canonicalize_file_name (const char *name) {
  char *res = malloc(PATH_MAX);
  char *rp_res = realpath(name, res);
  if (!rp_res)
    free(res);
  return rp_res;
}

int getloadavg(double loadavg[], int nelem) __attribute__((weak));
int getloadavg(double loadavg[], int nelem) {
  klee_warning("ignoring (-1 result)");
  return -1;
}

pid_t wait(int *status) __attribute__((weak));
pid_t wait(int *status) {
  klee_warning("ignoring (ECHILD)");
  errno = ECHILD;
  return -1;
}

pid_t wait3(int *status, int options, struct rusage *rusage) __attribute__((weak));
pid_t wait3(int *status, int options, struct rusage *rusage) {
  klee_warning("ignoring (ECHILD)");
  errno = ECHILD;
  return -1;
}

pid_t wait4(pid_t pid, int *status, int options, struct rusage *rusage) __attribute__((weak));
pid_t wait4(pid_t pid, int *status, int options, struct rusage *rusage) {
  klee_warning("ignoring (ECHILD)");
  errno = ECHILD;
  return -1;
}

pid_t waitpid(pid_t pid, int *status, int options) __attribute__((weak));
pid_t waitpid(pid_t pid, int *status, int options) {
  klee_warning("ignoring (ECHILD)");
  errno = ECHILD;
  return -1;
}

pid_t waitid(idtype_t idtype, id_t id, siginfo_t *infop, int options) __attribute__((weak));
pid_t waitid(idtype_t idtype, id_t id, siginfo_t *infop, int options) {
  klee_warning("ignoring (ECHILD)");
  errno = ECHILD;
  return -1;
}

/* ACL */

/* FIXME: We need autoconf magic for this. */

#ifdef HAVE_SYS_ACL_H

#include <sys/acl.h>

int acl_delete_def_file(const char *path_p) __attribute__((weak));
int acl_delete_def_file(const char *path_p) {
  klee_warning("ignoring (EPERM)");
  errno = EPERM;
  return -1;
}

int acl_extended_file(const char path_p) __attribute__((weak));
int acl_extended_file(const char path_p) {
  klee_warning("ignoring (ENOENT)");
  errno = ENOENT;
  return -1;
}

int acl_entries(acl_t acl) __attribute__((weak));
int acl_entries(acl_t acl) {
  klee_warning("ignoring (EINVAL)");
  errno = EINVAL;
  return -1;
}

acl_t acl_from_mode(mode_t mode) __attribute__((weak));
acl_t acl_from_mode(mode_t mode) {
  klee_warning("ignoring (ENOMEM)");
  errno = ENOMEM;
  return NULL;
}

acl_t acl_get_fd(int fd) __attribute__((weak));
acl_t acl_get_fd(int fd) {
  klee_warning("ignoring (ENOMEM)");
  errno = ENOMEM;
  return NULL;
}

acl_t acl_get_file(const char *pathname, acl_type_t type) __attribute__((weak));
acl_t acl_get_file(const char *pathname, acl_type_t type) {
  klee_warning("ignoring (ENONMEM)");
  errno = ENOMEM;
  return NULL;
}

int acl_set_fd(int fd, acl_t acl) __attribute__((weak));
int acl_set_fd(int fd, acl_t acl) {
  klee_warning("ignoring (EPERM)");
  errno = EPERM;
  return -1;
}

int acl_set_file(const char *path_p, acl_type_t type, acl_t acl) __attribute__((weak));
int acl_set_file(const char *path_p, acl_type_t type, acl_t acl) {
  klee_warning("ignoring (EPERM)");
  errno = EPERM;
  return -1;
}

int acl_free(void *obj_p) __attribute__((weak));
int acl_free(void *obj_p) {
  klee_warning("ignoring (EINVAL)");
  errno = EINVAL;
  return -1;
}

#endif

int mount(const char *source, const char *target, const char *filesystemtype, unsigned long mountflags, const void *data) __attribute__((weak));
int mount(const char *source, const char *target, const char *filesystemtype, unsigned long mountflags, const void *data) {
  klee_warning("ignoring (EPERM)");
  errno = EPERM;
  return -1;
}

int umount(const char *target) __attribute__((weak));
int umount(const char *target) {
  klee_warning("ignoring (EPERM)");
  errno = EPERM;
  return -1;
}

int umount2(const char *target, int flags) __attribute__((weak));
int umount2(const char *target, int flags) {
  klee_warning("ignoring (EPERM)");
  errno = EPERM;
  return -1;
}

int swapon(const char *path, int swapflags) __attribute__((weak));
int swapon(const char *path, int swapflags) {
  klee_warning("ignoring (EPERM)");
  errno = EPERM;
  return -1;
}

int swapoff(const char *path) __attribute__((weak));
int swapoff(const char *path) {
  klee_warning("ignoring (EPERM)");
  errno = EPERM;
  return -1;
}

int setgid(gid_t gid) __attribute__((weak));
int setgid(gid_t gid) {
  klee_warning("silently ignoring (returning 0)");
  return 0;
}

int setgroups(size_t size, const gid_t *list) __attribute__((weak));
int setgroups(size_t size, const gid_t *list) {
  klee_warning("ignoring (EPERM)");
  errno = EPERM;
  return -1;
}

int sethostname(const char *name, size_t len) __attribute__((weak));
int sethostname(const char *name, size_t len) {
  klee_warning("ignoring (EPERM)");
  errno = EPERM;
  return -1;
}

int setpgid(pid_t pid, pid_t pgid) __attribute__((weak));
int setpgid(pid_t pid, pid_t pgid) {
  klee_warning("ignoring (EPERM)");
  errno = EPERM;
  return -1;
}

int setpgrp(void) __attribute__((weak));
int setpgrp(void) {
  klee_warning("ignoring (EPERM)");
  errno = EPERM;
  return -1;
}

int setpriority(__priority_which_t which, id_t who, int prio) __attribute__((weak));
int setpriority(__priority_which_t which, id_t who, int prio) {
  klee_warning("ignoring (EPERM)");
  errno = EPERM;
  return -1;
}

int setresgid(gid_t rgid, gid_t egid, gid_t sgid) __attribute__((weak));
int setresgid(gid_t rgid, gid_t egid, gid_t sgid) {
  klee_warning("ignoring (EPERM)");
  errno = EPERM;
  return -1;
}

int setresuid(uid_t ruid, uid_t euid, uid_t suid) __attribute__((weak));
int setresuid(uid_t ruid, uid_t euid, uid_t suid) {
  klee_warning("ignoring (EPERM)");
  errno = EPERM;
  return -1;
}

int setrlimit(__rlimit_resource_t resource, const struct rlimit *rlim) __attribute__((weak));
int setrlimit(__rlimit_resource_t resource, const struct rlimit *rlim) {
  klee_warning("ignoring (EPERM)");
  errno = EPERM;
  return -1;
}

int setrlimit64(__rlimit_resource_t resource, const struct rlimit64 *rlim) __attribute__((weak));
int setrlimit64(__rlimit_resource_t resource, const struct rlimit64 *rlim) {
  klee_warning("ignoring (EPERM)");
  errno = EPERM;
  return -1;
}

pid_t setsid(void) __attribute__((weak));
pid_t setsid(void) {
  klee_warning("ignoring (EPERM)");
  errno = EPERM;
  return -1;
}

int settimeofday(const struct timeval *tv, const struct timezone *tz) __attribute__((weak));
int settimeofday(const struct timeval *tv, const struct timezone *tz) {
  klee_warning("ignoring (EPERM)");
  errno = EPERM;
  return -1;
}

int setuid(uid_t uid) __attribute__((weak));
int setuid(uid_t uid) {
  klee_warning("silently ignoring (returning 0)");
  return 0;
}

int reboot(int flag) __attribute__((weak));
int reboot(int flag) {
  klee_warning("ignoring (EPERM)");
  errno = EPERM;
  return -1;
}

int mlock(const void *addr, size_t len) __attribute__((weak));
int mlock(const void *addr, size_t len) {
  klee_warning("ignoring (EPERM)");
  errno = EPERM;
  return -1;
}

int munlock(const void *addr, size_t len) __attribute__((weak));
int munlock(const void *addr, size_t len) {
  klee_warning("ignoring (EPERM)");
  errno = EPERM;
  return -1;
}

int pause(void) __attribute__((weak));
int pause(void) {
  klee_warning("ignoring (EPERM)");
  errno = EPERM;
  return -1;
}

ssize_t readahead(int fd, off64_t *offset, size_t count) __attribute__((weak));
ssize_t readahead(int fd, off64_t *offset, size_t count) {
  klee_warning("ignoring (EPERM)");
  errno = EPERM;
  return -1;
}

void *mmap(void *start, size_t length, int prot, int flags, int fd, off_t offset) __attribute__((weak));
void *mmap(void *start, size_t length, int prot, int flags, int fd, off_t offset) {
  klee_warning("ignoring (EPERM)");
  errno = EPERM;
  return (void*) -1;
}

void *mmap64(void *start, size_t length, int prot, int flags, int fd, off64_t offset) __attribute__((weak));
void *mmap64(void *start, size_t length, int prot, int flags, int fd, off64_t offset) {
  klee_warning("ignoring (EPERM)");
  errno = EPERM;
  return (void*) -1;
}

int munmap(void*start, size_t length) __attribute__((weak));
int munmap(void*start, size_t length) {
  klee_warning("ignoring (EPERM)");
  errno = EPERM;
  return -1;
}
