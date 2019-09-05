//===-- fd.c --------------------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#define _LARGEFILE64_SOURCE
#include "fd.h"

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
#include <sys/time.h>
#include <termios.h>
#include <unistd.h>

/* Returns pointer to the symbolic file structure fs the pathname is symbolic */
static exe_disk_file_t *__get_sym_file(const char *pathname) {
  if (!pathname)
    return NULL;

  char c = pathname[0];
  unsigned i;

  if (c == 0 || pathname[1] != 0)
    return NULL;

  for (i=0; i<__exe_fs.n_sym_files; ++i) {
    if (c == 'A' + (char) i) {
      exe_disk_file_t *df = &__exe_fs.sym_files[i];
      if (df->stat->st_ino == 0)
        return NULL;
      return df;
    }
  }
  
  return NULL;
}

static void *__concretize_ptr(const void *p);
static size_t __concretize_size(size_t s);
static const char *__concretize_string(const char *s);

/* Returns pointer to the file entry for a valid fd */
static exe_file_t *__get_file(int fd) {
  if (fd>=0 && fd<MAX_FDS) {
    exe_file_t *f = &__exe_env.fds[fd];
    if (f->flags & eOpen)
      return f;
  }

  return 0;
}

int access(const char *pathname, int mode) {
  exe_disk_file_t *dfile = __get_sym_file(pathname);
  
  if (dfile) {
    /* XXX we should check against stat values but we also need to
       enforce in open and friends then. */
    return 0;
  }
  return syscall(__NR_access, __concretize_string(pathname), mode);
}

mode_t umask(mode_t mask) {  
  mode_t r = __exe_env.umask;
  __exe_env.umask = mask & 0777;
  return r;
}


/* Returns 1 if the process has the access rights specified by 'flags'
   to the file with stat 's'.  Returns 0 otherwise*/
static int has_permission(int flags, struct stat64 *s) {
  int write_access, read_access;
  mode_t mode = s->st_mode;
  
  if (flags & O_RDONLY || flags & O_RDWR)
    read_access = 1;
  else read_access = 0;

  if (flags & O_WRONLY || flags & O_RDWR)
    write_access = 1;
  else write_access = 0;

  /* XXX: We don't worry about process uid and gid for now. 
     We allow access if any user has access to the file. */
#if 0
  uid_t uid = s->st_uid;
  uid_t euid = geteuid();
  gid_t gid = s->st_gid;
  gid_t egid = getegid();
#endif  

  if (read_access && ((mode & S_IRUSR) | (mode & S_IRGRP) | (mode & S_IROTH)))
    return 0;

  if (write_access && !((mode & S_IWUSR) | (mode & S_IWGRP) | (mode & S_IWOTH)))
    return 0;

  return 1;
}


int __fd_open(const char *pathname, int flags, mode_t mode) {
  exe_disk_file_t *df;
  exe_file_t *f;
  int fd;

  for (fd = 0; fd < MAX_FDS; ++fd)
    if (!(__exe_env.fds[fd].flags & eOpen))
      break;
  if (fd == MAX_FDS) {
    errno = EMFILE;
    return -1;
  }
  
  f = &__exe_env.fds[fd];

  /* Should be the case if file was available, but just in case. */
  memset(f, 0, sizeof *f);

  df = __get_sym_file(pathname); 
  if (df) {    
    /* XXX Should check access against mode / stat / possible
       deletion. */
    f->dfile = df;
    
    if ((flags & O_CREAT) && (flags & O_EXCL)) {
      errno = EEXIST;
      return -1;
    }
    
    if ((flags & O_TRUNC) && (flags & O_RDONLY)) {
      /* The result of using O_TRUNC with O_RDONLY is undefined, so we
	 return error */
      klee_warning("Undefined call to open(): O_TRUNC | O_RDONLY\n");
      errno = EACCES;
      return -1;
    }

    if ((flags & O_EXCL) && !(flags & O_CREAT)) {
      /* The result of using O_EXCL without O_CREAT is undefined, so
	 we return error */
      klee_warning("Undefined call to open(): O_EXCL w/o O_RDONLY\n");
      errno = EACCES;
      return -1;
    }

    if (!has_permission(flags, df->stat)) {
	errno = EACCES;
	return -1;
    }
    else
      f->dfile->stat->st_mode = ((f->dfile->stat->st_mode & ~0777) |
				 (mode & ~__exe_env.umask));
  } else {    
    int os_fd = syscall(__NR_open, __concretize_string(pathname), flags, mode);
    if (os_fd == -1)
      return -1;
    f->fd = os_fd;
  }
  
  f->flags = eOpen;
  if ((flags & O_ACCMODE) == O_RDONLY) {
    f->flags |= eReadable;
  } else if ((flags & O_ACCMODE) == O_WRONLY) {
    f->flags |= eWriteable;
  } else { /* XXX What actually happens here if != O_RDWR. */
    f->flags |= eReadable | eWriteable;
  }
  
  return fd;
}

int __fd_openat(int basefd, const char *pathname, int flags, mode_t mode) {
  exe_file_t *f;
  int fd;
  if (basefd != AT_FDCWD) {
    exe_file_t *bf = __get_file(basefd);

    if (!bf) {
      errno = EBADF;
      return -1;
    } else if (bf->dfile) {
      klee_warning("symbolic file descriptor, ignoring (ENOENT)");
      errno = ENOENT;
      return -1;
    }
    basefd = bf->fd;
  }

  if (__get_sym_file(pathname)) {
    /* for a symbolic file, it doesn't matter if/where it exists on disk */
    return __fd_open(pathname, flags, mode);
  }

  for (fd = 0; fd < MAX_FDS; ++fd)
    if (!(__exe_env.fds[fd].flags & eOpen))
      break;
  if (fd == MAX_FDS) {
    errno = EMFILE;
    return -1;
  }
  
  f = &__exe_env.fds[fd];

  /* Should be the case if file was available, but just in case. */
  memset(f, 0, sizeof *f);

  int os_fd = syscall(__NR_openat, (long)basefd, __concretize_string(pathname), (long)flags, mode);
  if (os_fd == -1)
    return -1;

  f->fd = os_fd;
  f->flags = eOpen;
  if ((flags & O_ACCMODE) == O_RDONLY) {
    f->flags |= eReadable;
  } else if ((flags & O_ACCMODE) == O_WRONLY) {
    f->flags |= eWriteable;
  } else { /* XXX What actually happens here if != O_RDWR. */
    f->flags |= eReadable | eWriteable;
  }

  return fd;
}


int utimes(const char *path, const struct timeval times[2]) {
  exe_disk_file_t *dfile = __get_sym_file(path);

  if (dfile) {

    if (!times) {
      struct timeval newTimes[2];
      gettimeofday(&(newTimes[0]), NULL);
      newTimes[1] = newTimes[0];
      times = newTimes;
    }

    /* don't bother with usecs */
    dfile->stat->st_atime = times[0].tv_sec;
    dfile->stat->st_mtime = times[1].tv_sec;
#ifdef _BSD_SOURCE
    dfile->stat->st_atim.tv_nsec = 1000000000ll * times[0].tv_sec;
    dfile->stat->st_mtim.tv_nsec = 1000000000ll * times[1].tv_sec;
#endif
    return 0;
  }
  return syscall(__NR_utimes, __concretize_string(path), times);
}


int futimesat(int fd, const char* path, const struct timeval times[2]) {
  if (fd != AT_FDCWD) {
    exe_file_t *f = __get_file(fd);

    if (!f) {
      errno = EBADF;
      return -1;
    } else if (f->dfile) {
      klee_warning("symbolic file descriptor, ignoring (ENOENT)");
      errno = ENOENT;
      return -1;
    }
    fd = f->fd;
  }
  if (__get_sym_file(path)) {
    return utimes(path, times);
  }

  return syscall(__NR_futimesat, (long)fd,
                 (path ? __concretize_string(path) : NULL), times);
}
 
int close(int fd) {
  static int n_calls = 0;
  exe_file_t *f;
  int r = 0;
  
  n_calls++;  

  f = __get_file(fd);
  if (!f) {
    errno = EBADF;
    return -1;
  } 

  if (__exe_fs.max_failures && *__exe_fs.close_fail == n_calls) {
    __exe_fs.max_failures--;
    errno = EIO;
    return -1;
  }

#if 0
  if (!f->dfile) {
    /* if a concrete fd */
    r = syscall(__NR_close, f->fd);
  }
  else r = 0;
#endif

  memset(f, 0, sizeof *f);
  
  return r;
}

ssize_t read(int fd, void *buf, size_t count) {
  static int n_calls = 0;
  exe_file_t *f;

  n_calls++;

  if (count == 0) 
    return 0;

  if (buf == NULL) {
    errno = EFAULT;
    return -1;
  }
  
  f = __get_file(fd);

  if (!f) {
    errno = EBADF;
    return -1;
  }  

  if (__exe_fs.max_failures && *__exe_fs.read_fail == n_calls) {
    __exe_fs.max_failures--;
    errno = EIO;
    return -1;
  }
  
  if (!f->dfile) {
    /* concrete file */
    int r;
    buf = __concretize_ptr(buf);
    count = __concretize_size(count);
    /* XXX In terms of looking for bugs we really should do this check
       before concretization, at least once the routine has been fixed
       to properly work with symbolics. */
    klee_check_memory_access(buf, count);
    if (f->fd == 0)
      r = syscall(__NR_read, f->fd, buf, count);
    else
      r = syscall(__NR_pread64, f->fd, buf, count, (off64_t) f->off);

    if (r == -1)
      return -1;
    
    if (f->fd != 0)
      f->off += r;
    return r;
  }
  else {
    assert(f->off >= 0);
    if (((off64_t)f->dfile->size) < f->off)
      return 0;

    /* symbolic file */
    if (f->off + count > f->dfile->size) {
      count = f->dfile->size - f->off;
    }
    
    memcpy(buf, f->dfile->contents + f->off, count);
    f->off += count;
    
    return count;
  }
}


ssize_t write(int fd, const void *buf, size_t count) {
  static int n_calls = 0;
  exe_file_t *f;

  n_calls++;

  f = __get_file(fd);

  if (!f) {
    errno = EBADF;
    return -1;
  }

  if (__exe_fs.max_failures && *__exe_fs.write_fail == n_calls) {
    __exe_fs.max_failures--;
    errno = EIO;
    return -1;
  }

  if (!f->dfile) {
    int r;

    buf = __concretize_ptr(buf);
    count = __concretize_size(count);
    /* XXX In terms of looking for bugs we really should do this check
       before concretization, at least once the routine has been fixed
       to properly work with symbolics. */
    klee_check_memory_access(buf, count);
    if (f->fd == 1 || f->fd == 2)
      r = syscall(__NR_write, f->fd, buf, count);
    else r = syscall(__NR_pwrite64, f->fd, buf, count, (off64_t) f->off);

    if (r == -1)
      return -1;
    
    assert(r >= 0);
    if (f->fd != 1 && f->fd != 2)
      f->off += r;

    return r;
  }
  else {
    /* symbolic file */    
    size_t actual_count = 0;
    if (f->off + count <= f->dfile->size)
      actual_count = count;
    else {
      if (__exe_env.save_all_writes)
	assert(0);
      else {
	if (f->off < (off64_t) f->dfile->size)
	  actual_count = f->dfile->size - f->off;	
      }
    }
    
    if (actual_count)
      memcpy(f->dfile->contents + f->off, buf, actual_count);
    
    if (count != actual_count)
      klee_warning("write() ignores bytes.\n");

    if (f->dfile == __exe_fs.sym_stdout)
      __exe_fs.stdout_writes += actual_count;

    f->off += count;
    return count;
  }
}


off64_t __fd_lseek(int fd, off64_t offset, int whence) {
  off64_t new_off;
  exe_file_t *f = __get_file(fd);

  if (!f) {
    errno = EBADF;
    return -1;
  }

  if (!f->dfile) {
    /* We could always do SEEK_SET then whence, but this causes
       troubles with directories since we play nasty tricks with the
       offset, and the OS doesn't want us to randomly seek
       directories. We could detect if it is a directory and correct
       the offset, but really directories should only be SEEK_SET, so
       this solves the problem. */
    if (whence == SEEK_SET) {
      new_off = syscall(__NR_lseek, f->fd, offset, SEEK_SET);
    } else {
      new_off = syscall(__NR_lseek, f->fd, f->off, SEEK_SET);

      /* If we can't seek to start off, just return same error.
         Probably ESPIPE. */
      if (new_off != -1) {
        assert(new_off == f->off);
        new_off = syscall(__NR_lseek, f->fd, offset, whence);
      }
    }

    if (new_off == -1)
      return -1;

    f->off = new_off;
    return new_off;
  }
  
  switch (whence) {
  case SEEK_SET: new_off = offset; break;
  case SEEK_CUR: new_off = f->off + offset; break;
  case SEEK_END: new_off = f->dfile->size + offset; break;
  default: {
    errno = EINVAL;
    return (off64_t) -1;
  }
  }

  if (new_off < 0) {
    errno = EINVAL;
    return (off64_t) -1;
  }
    
  f->off = new_off;
  return f->off;
}

int __fd_stat(const char *path, struct stat64 *buf) {  
  exe_disk_file_t *dfile = __get_sym_file(path);
  if (dfile) {
    memcpy(buf, dfile->stat, sizeof(*dfile->stat));
    return 0;
  } 

  {
#if __WORDSIZE == 64
    return syscall(__NR_stat, __concretize_string(path), buf);
#else
    return syscall(__NR_stat64, __concretize_string(path), buf);
#endif
  }
}

int fstatat(int fd, const char *path, struct stat *buf, int flags) {  
  if (fd != AT_FDCWD) {
    exe_file_t *f = __get_file(fd);

    if (!f) {
      errno = EBADF;
      return -1;
    } else if (f->dfile) {
      klee_warning("symbolic file descriptor, ignoring (ENOENT)");
      errno = ENOENT;
      return -1;
    }
    fd = f->fd;
  }
  exe_disk_file_t *dfile = __get_sym_file(path);
  if (dfile) {
    memcpy(buf, dfile->stat, sizeof(*dfile->stat));
    return 0;
  } 

#if (defined __NR_newfstatat) && (__NR_newfstatat != 0)
  return syscall(__NR_newfstatat, (long)fd,
                 (path ? __concretize_string(path) : NULL), buf, (long)flags);
#else
  return syscall(__NR_fstatat64, (long)fd,
                 (path ? __concretize_string(path) : NULL), buf, (long)flags);
#endif
}


int __fd_lstat(const char *path, struct stat64 *buf) {
  exe_disk_file_t *dfile = __get_sym_file(path);
  if (dfile) {
    memcpy(buf, dfile->stat, sizeof(*dfile->stat));
    return 0;
  } 

  {    
#if __WORDSIZE == 64
    return syscall(__NR_lstat, __concretize_string(path), buf);
#else
    return syscall(__NR_lstat64, __concretize_string(path), buf);
#endif
  }
}

int chdir(const char *path) {
  exe_disk_file_t *dfile = __get_sym_file(path);

  if (dfile) {
    /* XXX incorrect */
    klee_warning("symbolic file, ignoring (ENOENT)");
    errno = ENOENT;
    return -1;
  }

  return syscall(__NR_chdir, __concretize_string(path));
}

int fchdir(int fd) {
  exe_file_t *f = __get_file(fd);
  
  if (!f) {
    errno = EBADF;
    return -1;
  }

  if (f->dfile) {
    klee_warning("symbolic file, ignoring (ENOENT)");
    errno = ENOENT;
    return -1;
  }

  return syscall(__NR_fchdir, f->fd);
}

/* Sets mode and or errno and return appropriate result. */
static int __df_chmod(exe_disk_file_t *df, mode_t mode) {
  if (geteuid() == df->stat->st_uid) {
    if (getgid() != df->stat->st_gid)
      mode &= ~ S_ISGID;
    df->stat->st_mode = ((df->stat->st_mode & ~07777) | 
                         (mode & 07777));
    return 0;
  } else {
    errno = EPERM;
    return -1;
  }
}

int chmod(const char *path, mode_t mode) {
  static int n_calls = 0;

  exe_disk_file_t *dfile = __get_sym_file(path);

  n_calls++;
  if (__exe_fs.max_failures && *__exe_fs.chmod_fail == n_calls) {
    __exe_fs.max_failures--;
    errno = EIO;
    return -1;
  }

  if (dfile) {
    return __df_chmod(dfile, mode);
  }

  return syscall(__NR_chmod, __concretize_string(path), mode);
}

int fchmod(int fd, mode_t mode) {
  static int n_calls = 0;

  exe_file_t *f = __get_file(fd);
  
  if (!f) {
    errno = EBADF;
    return -1;
  }

  n_calls++;
  if (__exe_fs.max_failures && *__exe_fs.fchmod_fail == n_calls) {
    __exe_fs.max_failures--;
    errno = EIO;
    return -1;
  }

  if (f->dfile) {
    return __df_chmod(f->dfile, mode);
  }

  return syscall(__NR_fchmod, f->fd, mode);
}

static int __df_chown(exe_disk_file_t *df, uid_t owner, gid_t group) {
  klee_warning("symbolic file, ignoring (EPERM)");
  errno = EPERM;
  return -1;  
}

int chown(const char *path, uid_t owner, gid_t group) {
  exe_disk_file_t *df = __get_sym_file(path);

  if (df) {
    return __df_chown(df, owner, group);
  }

  return syscall(__NR_chown, __concretize_string(path), owner, group);
}

int fchown(int fd, uid_t owner, gid_t group) {
  exe_file_t *f = __get_file(fd);

  if (!f) {
    errno = EBADF;
    return -1;
  }

  if (f->dfile) {
    return __df_chown(f->dfile, owner, group);
  }

  return syscall(__NR_fchown, fd, owner, group);
}

int lchown(const char *path, uid_t owner, gid_t group) {
  /* XXX Ignores 'l' part */
  exe_disk_file_t *df = __get_sym_file(path);

  if (df) {
    return __df_chown(df, owner, group);
  }

  return syscall(__NR_chown, __concretize_string(path), owner, group);
}

int __fd_fstat(int fd, struct stat64 *buf) {
  exe_file_t *f = __get_file(fd);

  if (!f) {
    errno = EBADF;
    return -1;
  }
  
  if (!f->dfile) {
#if __WORDSIZE == 64
    return syscall(__NR_fstat, f->fd, buf);
#else
    return syscall(__NR_fstat64, f->fd, buf);
#endif
  }
  
  memcpy(buf, f->dfile->stat, sizeof(*f->dfile->stat));
  return 0;
}

int __fd_ftruncate(int fd, off64_t length) {
  static int n_calls = 0;
  exe_file_t *f = __get_file(fd);

  n_calls++;

  if (!f) {
    errno = EBADF;
    return -1;
  }

  if (__exe_fs.max_failures && *__exe_fs.ftruncate_fail == n_calls) {
    __exe_fs.max_failures--;
    errno = EIO;
    return -1;
  }
  
  if (f->dfile) {
    klee_warning("symbolic file, ignoring (EIO)");
    errno = EIO;
    return -1;
  }
#if __WORDSIZE == 64
  return syscall(__NR_ftruncate, f->fd, length);
#else
  return syscall(__NR_ftruncate64, f->fd, length);
#endif
}

int __fd_getdents(unsigned int fd, struct dirent64 *dirp, unsigned int count) {
  exe_file_t *f = __get_file(fd);

  if (!f) {
    errno = EBADF;
    return -1;
  }

  if (f->dfile) {
    klee_warning("symbolic file, ignoring (EINVAL)");
    errno = EINVAL;
    return -1;
  } else {
    if ((unsigned long) f->off < 4096u) {
      /* Return our dirents */
      off64_t i, pad, bytes=0;

      /* What happens for bad offsets? */
      i = f->off / sizeof(*dirp);
      if (((off64_t) (i * sizeof(*dirp)) != f->off) ||
          i > __exe_fs.n_sym_files) {
        errno = EINVAL;
        return -1;
      } 
      for (; i<__exe_fs.n_sym_files; ++i) {
        exe_disk_file_t *df = &__exe_fs.sym_files[i];
        dirp->d_ino = df->stat->st_ino;
        dirp->d_reclen = sizeof(*dirp);
        dirp->d_type = IFTODT(df->stat->st_mode);
        dirp->d_name[0] = 'A' + i;
        dirp->d_name[1] = '\0';
#ifdef _DIRENT_HAVE_D_OFF
        dirp->d_off = (i+1) * sizeof(*dirp);
#endif
        bytes += dirp->d_reclen;
        ++dirp;
      }
      
      /* Fake jump to OS records by a "deleted" file. */
      pad = count>=4096 ? 4096 : count;
      dirp->d_ino = 0;
      dirp->d_reclen = pad - bytes;
      dirp->d_type = DT_UNKNOWN;
      dirp->d_name[0] = '\0';
#ifdef _DIRENT_HAVE_D_OFF
      dirp->d_off = 4096;
#endif
      bytes += dirp->d_reclen;
      f->off = pad;

      return bytes;
    } else {
      off64_t os_pos = f->off - 4096;
      int res;
      off64_t s = 0;

      /* For reasons which I really don't understand, if I don't
         memset this then sometimes the kernel returns d_ino==0 for
         some valid entries? Am I crazy? Can writeback possibly be
         failing? 
      
         Even more bizarre, interchanging the memset and the seek also
         case strange behavior. Really should be debugged properly. */
      memset(dirp, 0, count);
      s = syscall(__NR_lseek, f->fd, os_pos, SEEK_SET);
      assert(s != (off64_t) -1);
      res = syscall(__NR_getdents64, f->fd, dirp, count);
      if (res > -1) {
        int pos = 0;
        f->off = syscall(__NR_lseek, f->fd, 0, SEEK_CUR);
        assert(f->off != (off64_t)-1);
        f->off += 4096;

        /* Patch offsets */
        while (pos < res) {
          struct dirent64 *dp = (struct dirent64*) ((char*) dirp + pos);
#ifdef _DIRENT_HAVE_D_OFF
          dp->d_off += 4096;
#endif
          pos += dp->d_reclen;
        }
      }
      return res;
    }
  }
}

#if __WORDSIZE == 64
int ioctl(int fd, unsigned long int request, ...) {
#else
int ioctl(int fd, unsigned long request, ...) {
#endif
  exe_file_t *f = __get_file(fd);
  va_list ap;
  void *buf;

#if 0
  printf("In ioctl(%d, ...)\n", fd);
#endif

  if (!f) {
    errno = EBADF;
    return -1;
  }
  
  va_start(ap, request);
  buf = va_arg(ap, void*);
  va_end(ap);
  if (f->dfile) {
    struct stat *stat = (struct stat*) f->dfile->stat;

    switch (request) {
    case TCGETS: {      
      struct termios *ts = buf;

      klee_warning_once("(TCGETS) symbolic file, incomplete model");

      /* XXX need more data, this is ok but still not good enough */
      if (S_ISCHR(stat->st_mode)) {
        /* Just copied from my system, munged to match what fields
           uclibc thinks are there. */
        ts->c_iflag = 27906;
        ts->c_oflag = 5;
        ts->c_cflag = 1215;
        ts->c_lflag = 35287;
#ifdef __GLIBC__
        ts->c_line = 0;
#endif
        ts->c_cc[0] = '\x03';
        ts->c_cc[1] = '\x1c';
        ts->c_cc[2] = '\x7f';
        ts->c_cc[3] = '\x15';
        ts->c_cc[4] = '\x04';
        ts->c_cc[5] = '\x00';
        ts->c_cc[6] = '\x01';
        ts->c_cc[7] = '\xff';
        ts->c_cc[8] = '\x11';
        ts->c_cc[9] = '\x13';
        ts->c_cc[10] = '\x1a';
        ts->c_cc[11] = '\xff';
        ts->c_cc[12] = '\x12';
        ts->c_cc[13] = '\x0f';
        ts->c_cc[14] = '\x17';
        ts->c_cc[15] = '\x16';
        ts->c_cc[16] = '\xff';
        ts->c_cc[17] = '\x0';
        ts->c_cc[18] = '\x0';
        return 0;
      } else {
        errno = ENOTTY;
        return -1;
      }
    }
    case TCSETS: {
      /* const struct termios *ts = buf; */
      klee_warning_once("(TCSETS) symbolic file, silently ignoring");
      if (S_ISCHR(stat->st_mode)) {
        return 0;
      } else {
        errno = ENOTTY;
        return -1;
      }
    }
    case TCSETSW: {
      /* const struct termios *ts = buf; */
      klee_warning_once("(TCSETSW) symbolic file, silently ignoring");
      if (fd==0) {
        return 0;
      } else {
        errno = ENOTTY;
        return -1;
      }
    }
    case TCSETSF: {
      /* const struct termios *ts = buf; */
      klee_warning_once("(TCSETSF) symbolic file, silently ignoring");
      if (S_ISCHR(stat->st_mode)) {        
        return 0;
      } else {
        errno = ENOTTY;
        return -1;
      }
    }
    case TIOCGWINSZ: {
      struct winsize *ws = buf;
      ws->ws_row = 24;
      ws->ws_col = 80;
      klee_warning_once("(TIOCGWINSZ) symbolic file, incomplete model");
      if (S_ISCHR(stat->st_mode)) {
        return 0;
      } else {
        errno = ENOTTY;
        return -1;
      }
    }
    case TIOCSWINSZ: {
      /* const struct winsize *ws = buf; */
      klee_warning_once("(TIOCSWINSZ) symbolic file, ignoring (EINVAL)");
      if (S_ISCHR(stat->st_mode)) {
        errno = EINVAL;
        return -1;
      } else {
        errno = ENOTTY;
        return -1;
      }
    }
    case FIONREAD: {
      int *res = buf;
      klee_warning_once("(FIONREAD) symbolic file, incomplete model");
      if (S_ISCHR(stat->st_mode)) {
        if (f->off < (off64_t) f->dfile->size) {
          *res = f->dfile->size - f->off;
        } else {
          *res = 0;
        }
        return 0;
      } else {
        errno = ENOTTY;
        return -1;
      }
    }
    case MTIOCGET: {
      klee_warning("(MTIOCGET) symbolic file, ignoring (EINVAL)");
      errno = EINVAL;
      return -1;
    }
    default:
      klee_warning("symbolic file, ignoring (EINVAL)");
      errno = EINVAL;
      return -1;
    }
  }
  return syscall(__NR_ioctl, f->fd, request, buf);
}

int fcntl(int fd, int cmd, ...) {
  exe_file_t *f = __get_file(fd);
  va_list ap;
  unsigned arg; /* 32 bit assumption (int/ptr) */

  if (!f) {
    errno = EBADF;
    return -1;
  }
#ifdef F_GETSIG
  if (cmd==F_GETFD || cmd==F_GETFL || cmd==F_GETOWN || cmd==F_GETSIG ||
      cmd==F_GETLEASE || cmd==F_NOTIFY) {
#else
   if (cmd==F_GETFD || cmd==F_GETFL || cmd==F_GETOWN) {
#endif
    arg = 0;
  } else {
    va_start(ap, cmd);
    arg = va_arg(ap, int);
    va_end(ap);
  }

  if (f->dfile) {
    switch(cmd) {
    case F_GETFD: {
      int flags = 0;
      if (f->flags & eCloseOnExec)
        flags |= FD_CLOEXEC;
      return flags;
    } 
    case F_SETFD: {
      f->flags &= ~eCloseOnExec;
      if (arg & FD_CLOEXEC)
        f->flags |= eCloseOnExec;
      return 0;
    }
    case F_GETFL: {
      /* XXX (CrC): This should return the status flags: O_APPEND,
	 O_ASYNC, O_DIRECT, O_NOATIME, O_NONBLOCK.  As of now, we
	 discard these flags during open().  We should save them and
	 return them here.  These same flags can be set by F_SETFL,
	 which we could also handle properly. 
      */
      return 0;
    }
    default:
      klee_warning("symbolic file, ignoring (EINVAL)");
      errno = EINVAL;
      return -1;
    }
  }
  return syscall(__NR_fcntl, f->fd, cmd, arg);
}

int __fd_statfs(const char *path, struct statfs *buf) {
  exe_disk_file_t *dfile = __get_sym_file(path);
  if (dfile) {
    /* XXX incorrect */
    klee_warning("symbolic file, ignoring (ENOENT)");
    errno = ENOENT;
    return -1;
  }

  return syscall(__NR_statfs, __concretize_string(path), buf);
}

int fstatfs(int fd, struct statfs *buf) {
  exe_file_t *f = __get_file(fd);

  if (!f) {
    errno = EBADF;
    return -1;
  }
  
  if (f->dfile) {
    klee_warning("symbolic file, ignoring (EBADF)");
    errno = EBADF;
    return -1;
  }
  return syscall(__NR_fstatfs, f->fd, buf);
}

int fsync(int fd) {
  exe_file_t *f = __get_file(fd);

  if (!f) {
    errno = EBADF;
    return -1;
  } else if (f->dfile) {
    return 0;
  }
  return syscall(__NR_fsync, f->fd);
}

int dup2(int oldfd, int newfd) {
  exe_file_t *f = __get_file(oldfd);

  if (!f || !(newfd>=0 && newfd<MAX_FDS)) {
    errno = EBADF;
    return -1;
  } else {
    exe_file_t *f2 = &__exe_env.fds[newfd];
    if (f2->flags & eOpen) close(newfd);

    /* XXX Incorrect, really we need another data structure for open
       files */
    *f2 = *f;

    f2->flags &= ~eCloseOnExec;
      
    /* I'm not sure it is wise, but we can get away with not dup'ng
       the OS fd, since actually that will in many cases effect the
       sharing of the open file (and the process should never have
       access to it). */

    return newfd;
  }
}

int dup(int oldfd) {
  exe_file_t *f = __get_file(oldfd);
  if (!f) {
    errno = EBADF;
    return -1;
  } else {
    int fd;
    for (fd = 0; fd < MAX_FDS; ++fd)
      if (!(__exe_env.fds[fd].flags & eOpen))
        break;
    if (fd == MAX_FDS) {
      errno = EMFILE;
      return -1;
    } else {
      return dup2(oldfd, fd);
    }
  }
}

int rmdir(const char *pathname) {
  exe_disk_file_t *dfile = __get_sym_file(pathname);
  if (dfile) {
    /* XXX check access */ 
    if (S_ISDIR(dfile->stat->st_mode)) {
      dfile->stat->st_ino = 0;
      return 0;
    } else {
      errno = ENOTDIR;
      return -1;
    }
  }

  klee_warning("ignoring (EPERM)");
  errno = EPERM;
  return -1;
}

int unlink(const char *pathname) {
  exe_disk_file_t *dfile = __get_sym_file(pathname);
  if (dfile) {
    /* XXX check access */ 
    if (S_ISREG(dfile->stat->st_mode)) {
      dfile->stat->st_ino = 0;
      return 0;
    } else if (S_ISDIR(dfile->stat->st_mode)) {
      errno = EISDIR;
      return -1;
    } else {
      errno = EPERM;
      return -1;
    }
  }

  klee_warning("ignoring (EPERM)");
  errno = EPERM;
  return -1;
}

int unlinkat(int dirfd, const char *pathname, int flags) {
  /* similar to unlink. keep them separated though to avoid
     problems if unlink changes to actually delete files */
  exe_disk_file_t *dfile = __get_sym_file(pathname);
  if (dfile) {
    /* XXX check access */ 
    if (S_ISREG(dfile->stat->st_mode)) {
      dfile->stat->st_ino = 0;
      return 0;
    } else if (S_ISDIR(dfile->stat->st_mode)) {
      errno = EISDIR;
      return -1;
    } else {
      errno = EPERM;
      return -1;
    }
  }

  klee_warning("ignoring (EPERM)");
  errno = EPERM;
  return -1;
}

ssize_t readlink(const char *path, char *buf, size_t bufsize) {
  exe_disk_file_t *dfile = __get_sym_file(path);
  if (dfile) {
    /* XXX We need to get the sym file name really, but since we don't
       handle paths anyway... */
    if (S_ISLNK(dfile->stat->st_mode)) {
      buf[0] = path[0];
      if (bufsize>1) buf[1] = '.';
      if (bufsize>2) buf[2] = 'l';
      if (bufsize>3) buf[3] = 'n';
      if (bufsize>4) buf[4] = 'k';
      return (bufsize>5) ? 5 : bufsize;
    } else {
      errno = EINVAL;
      return -1;
    }
  }
  return syscall(__NR_readlink, path, buf, bufsize);
}

#undef FD_SET
#undef FD_CLR
#undef FD_ISSET
#undef FD_ZERO
#define	FD_SET(n, p)	((p)->fds_bits[(n)/NFDBITS] |= (1 << ((n) % NFDBITS)))
#define	FD_CLR(n, p)	((p)->fds_bits[(n)/NFDBITS] &= ~(1 << ((n) % NFDBITS)))
#define	FD_ISSET(n, p)	((p)->fds_bits[(n)/NFDBITS] & (1 << ((n) % NFDBITS)))
#define FD_ZERO(p)	memset((char *)(p), '\0', sizeof(*(p)))
int select(int nfds, fd_set *read, fd_set *write,
           fd_set *except, struct timeval *timeout) {
  fd_set in_read, in_write, in_except, os_read, os_write, os_except;
  int i, count = 0, os_nfds = 0;

  if (read) {
    in_read = *read;
    FD_ZERO(read);
  } else {
    FD_ZERO(&in_read);
  }

  if (write) {
    in_write = *write;
    FD_ZERO(write);
  } else {
    FD_ZERO(&in_write);
  }
   
  if (except) {
    in_except = *except;
    FD_ZERO(except);
  } else {
    FD_ZERO(&in_except);
  }

  FD_ZERO(&os_read);
  FD_ZERO(&os_write);
  FD_ZERO(&os_except);

  /* Check for symbolic stuff */
  for (i=0; i<nfds; i++) {    
    if (FD_ISSET(i, &in_read) || FD_ISSET(i, &in_write) || FD_ISSET(i, &in_except)) {
      exe_file_t *f = __get_file(i);
      if (!f) {
        errno = EBADF;
        return -1;
      } else if (f->dfile) {
        /* Operations on this fd will never block... */
        if (FD_ISSET(i, &in_read)) FD_SET(i, read);
        if (FD_ISSET(i, &in_write)) FD_SET(i, write);
        if (FD_ISSET(i, &in_except)) FD_SET(i, except);
        ++count;
      } else {
        if (FD_ISSET(i, &in_read)) FD_SET(f->fd, &os_read);
        if (FD_ISSET(i, &in_write)) FD_SET(f->fd, &os_write);
        if (FD_ISSET(i, &in_except)) FD_SET(f->fd, &os_except);
        if (f->fd >= os_nfds) os_nfds = f->fd + 1;
      }
    }
  }

  if (os_nfds > 0) {
    /* Never allow blocking select. This is broken but what else can
       we do. */
    struct timeval tv = { 0, 0 };    
    int r = syscall(__NR_select, os_nfds, 
                    &os_read, &os_write, &os_except, &tv);
    
    if (r == -1) {
      /* If no symbolic results, return error. Otherwise we will
         silently ignore the OS error. */
      if (!count)
        return -1;
    } else {
      count += r;

      /* Translate resulting sets back */
      for (i=0; i<nfds; i++) {
        exe_file_t *f = __get_file(i);
        if (f && !f->dfile) {
          if (read && FD_ISSET(f->fd, &os_read)) FD_SET(i, read);
          if (write && FD_ISSET(f->fd, &os_write)) FD_SET(i, write);
          if (except && FD_ISSET(f->fd, &os_except)) FD_SET(i, except);
        }
      }
    }
  }

  return count;
}

/*** Library functions ***/

char *getcwd(char *buf, size_t size) {
  static int n_calls = 0;
  int r;

  n_calls++;

  if (__exe_fs.max_failures && *__exe_fs.getcwd_fail == n_calls) {
    __exe_fs.max_failures--;
    errno = ERANGE;
    return NULL;
  }

  if (!buf) {
    if (!size)
      size = 1024;
    buf = malloc(size);
  }
  
  buf = __concretize_ptr(buf);
  size = __concretize_size(size);
  /* XXX In terms of looking for bugs we really should do this check
     before concretization, at least once the routine has been fixed
     to properly work with symbolics. */
  klee_check_memory_access(buf, size);
  r = syscall(__NR_getcwd, buf, size);
  if (r == -1)
    return NULL;
  return buf;
}

/*** Helper functions ***/

static void *__concretize_ptr(const void *p) {
  /* XXX 32-bit assumption */
  char *pc = (char*) klee_get_valuel((long) p);
  klee_assume(pc == p);
  return pc;
}

static size_t __concretize_size(size_t s) {
  size_t sc = klee_get_valuel((long)s);
  klee_assume(sc == s);
  return sc;
}

static const char *__concretize_string(const char *s) {
  char *sc = __concretize_ptr(s);
  unsigned i;

  for (i = 0;; ++i, ++sc) {
    char c = *sc;
    // Avoid writing read-only memory locations
    if (!klee_is_symbolic(c)) {
      if (!c)
        break;
      continue;
    }
    if (!(i&(i-1))) {
      if (!c) {
        *sc = 0;
        break;
      } else if (c=='/') {
        *sc = '/';
      } 
    } else {
      char cc = (char) klee_get_valuel((long)c);
      klee_assume(cc == c);
      *sc = cc;
      if (!cc) break;
    }
  }

  return s;
}



/* Trivial model:
   if path is "/" (basically no change) accept, otherwise reject
*/
int chroot(const char *path) {
  if (path[0] == '\0') {
    errno = ENOENT;
    return -1;
  }
    
  if (path[0] == '/' && path[1] == '\0') {
    return 0;
  }
  
  klee_warning("ignoring (ENOENT)");
  errno = ENOENT;
  return -1;
}
