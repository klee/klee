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

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/syscall.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdarg.h>
#include <assert.h>
#include <sys/vfs.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/ioctl.h>
#include <sys/mtio.h>
#include <termios.h>
#include <sys/select.h>
#include <ctype.h>
#include <net/if.h>
#include <stdbool.h>
#include <sys/mman.h>

#include <klee/klee.h>

/* #define DEBUG */
/* #define _DEBUG_STUBS */
#define INJECT_SYMBOLIC_DIR_ENTRIES 1
#define USE_TRANSPARENT_FDS 0

bool UseConcretePath;

void klee_warning(const char*);
void klee_warning_once(const char*);
int klee_get_errno(void);

static void *__concretize_ptr(const void *p);
static size_t __concretize_size(size_t s);
static const char *__concretize_string(const char *s);
static const char *__concretize_buffer(const char *s, unsigned size);

static exe_disk_file_t *__get_sym_file_internal(const char *pathname, int flags, bool cp_treat_symbolic);

static exe_disk_file_t *__get_sym_file(const char *pathname) {
  return __get_sym_file_internal(pathname, 0, false);
}

/* Returns pointer to the symbolic file structure is the pathname is symbolic */
static exe_disk_file_t *__get_sym_file_internal(const char *pathname, int flags, bool cp_treat_symbolic) {
  char c;
  unsigned i;
  if (!pathname || !pathname[0])
    return NULL;
  UseConcretePath = (klee_zest_enabled() == 1);
  if (!pathname[1]) { // standard one-letter symbolic file
    c = pathname[0];

    for (i=0; i<__exe_fs.n_sym_files; ++i) {
      if (c == 'A' + (char) i) {
        exe_disk_file_t *df = &__exe_fs.sym_files[i];
        if (df->stat->st_ino == 0)
          return NULL;
        return df;
      }
    }
  } else if (UseConcretePath && cp_treat_symbolic) {
    exe_disk_file_t* df = NULL;
    bool file_check = false;
    if (klee_is_symbolic(pathname[0])) {
      fprintf(stderr, "ZEST: opening symbolic file %s\n", pathname);
      file_check = true;
    } else {
      char* dot = strchr(pathname, '.');
      if (dot && 0 == strcmp(dot, ".test")) {
        file_check = true;
        fprintf(stderr, "ZEST: opening symbolic file (.test) %s\n", pathname);
      }
    }
    if (file_check) {
      // concretize the file name in-place
      const char* concrete_path = __concretize_string(pathname);
      for (i=0; i<__exe_fs.n_cp_files; ++i) {
        if (__exe_fs.cp_files[i].path && 0 == strcmp(__exe_fs.cp_files[i].path, pathname)) {
          df = &__exe_fs.cp_files[i];
        }
      }
      if (!df) {
        df = klee_init_cp_file(concrete_path, flags);
        /* if (!df)
          klee_warning("Unable to open symbolic file.");
        */
      }
      // free(concrete_pathname);
      return df;
    } 
    else {
      fprintf(stderr, "ZEST: opening concrete file %s\n", pathname);
    }
  }
  
  return NULL;
}

/* Returns pointer to the file entry for a valid fd */
exe_file_t *__get_file(int fd) {
#if USE_TRANSPARENT_FDS
  if (fd >= 0) {
    unsigned i;
    for(i = 0; i < MAX_FDS; ++i) {
      exe_file_t *f = &__exe_env.fds[i];
      if (f->flags & eOpen && f->fd == fd) {
        return f;
      }
    }
    return NULL;
  }
#else
  if (fd >= 0 && fd < MAX_FDS) {
    exe_file_t *f = &__exe_env.fds[fd];
    if (f->flags & eOpen)
      return f;
  }
#endif

  return NULL;
}

int access(const char *pathname, int mode) {
  exe_disk_file_t *dfile = __get_sym_file(pathname);
  
#ifdef _DEBUG_STUBS
  printf("access %s\n", pathname);
#endif
  if (dfile) {
    /* XXX we should check against stat values but we also need to
       enforce in open and friends then. */
    return 0;
  } else {
    int r = syscall(__NR_access, __concretize_string(pathname), mode);
    if (r == -1)
      errno = klee_get_errno();
    return r;
  } 
}

int euidaccess(const char *pathname, int mode) {
  exe_disk_file_t *dfile = __get_sym_file(pathname);
  
#ifdef _DEBUG_STUBS
  printf("euidaccess %s\n", pathname);
#endif
  if (dfile) {
    /* XXX we should check against stat values but we also need to
       enforce in open and friends then. */
    return 0;
  } else {
    int r = syscall(__NR_access, __concretize_string(pathname), mode);
    if (r == -1)
      errno = klee_get_errno();
    return r;
  } 
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
//#define _DEBUG_STUBS
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

#ifdef _DEBUG_STUBS
  printf("open %s\n", pathname);
#endif
  /* Should be the case if file was available, but just in case. */
  // memset is expensive. we already do it in close
  //memset(f, 0, sizeof *f);
  df = __get_sym_file_internal(pathname, flags, (flags & O_ACCMODE) == O_RDONLY);
  if (df) {
#ifdef _DEBUG_STUBS
    printf("opened symbolic file\n");
#endif
    /* XXX Should check access against mode / stat / possible
       deletion. */
    f->dfile = df;
#if USE_TRANSPARENT_FDS
    // we need a dummy fd (ideally the lowest not-used fd)
    f->fd = open(".", O_RDONLY);
#endif
    if ((flags & O_CREAT) && (flags & O_EXCL)) {
      errno = EEXIST;
      return -1;
    }
    
    if ((flags & O_TRUNC) && (flags & O_RDONLY)) {
      /* The result of using O_TRUNC with O_RDONLY is undefined, so we
	 return error */
      fprintf(stderr, "Undefined call to open(): O_TRUNC | O_RDONLY\n");
      errno = EACCES;
      return -1;
    }

    if ((flags & O_EXCL) && !(flags & O_CREAT)) {
      /* The result of using O_EXCL without O_CREAT is undefined, so
	 we return error */
      fprintf(stderr, "Undefined call to open(): O_EXCL w/o O_RDONLY\n");
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
    // concretize pathname. should be fine
    int os_fd;
    __concretize_string(pathname);
#if USE_TRANSPARENT_FDS == 0
    if (strncmp(pathname, "/proc/self/fd/", 14) == 0) {
      // program (or fdopendir) is using some hacky way to open a file/folder
      // try to make it work by substituing the runtime fd with the real fd
      char buffer[1024];
      int runtime_fd = atoi(pathname+14);
      exe_file_t *f = __get_file(runtime_fd);
      if (f) {
        char* pathtail = strchr(pathname+14, '/');
        snprintf(buffer, sizeof(buffer)-1, "/proc/self/fd/%d%s", f->fd, (pathtail ? pathtail : ""));
        buffer[sizeof(buffer)-1] = 0;
        os_fd = syscall(__NR_open, buffer, flags, mode);
#ifdef _DEBUG_STUBS
        printf("open: automagically opening %s\n", buffer);
#endif
      } else {
        os_fd = syscall(__NR_open, pathname, flags, mode);
      }
    } else
#endif
    {
      os_fd = syscall(__NR_open, pathname, flags, mode);
    }
    
#ifdef _DEBUG_STUBS
     printf("open returns %d (errno %d)\n", os_fd, errno);
     if (os_fd < 0)
        perror("open");
#endif
    if (os_fd == -1) {
      errno = klee_get_errno();
      return -1;
    }
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
#if USE_TRANSPARENT_FDS 
  return f->fd;
#else
  return fd;
#endif
}

int close(int fd) {
  static int n_calls = 0;
  exe_file_t *f;
  int r = 0;
  
  n_calls++;  

  f = __get_file(fd);

#ifdef _DEBUG_STUBS
  if (f)
    printf("close %d(%d)\n", f->fd, fd);
  else
    printf("close %d(nil)\n", fd);
#endif

  if (!f) {
    errno = EBADF;
    return -1;
  } 

  if (__exe_fs.max_failures && *__exe_fs.close_fail == n_calls) {
    __exe_fs.max_failures--;
    errno = EIO;
    return -1;
  }

  if (!f->dfile && f->flags & eSocket) {
    /* close concrete sockets because the
     * remote endpoint might expect such an event
     * (and would otherwise wait indefinitely) 
     * On the downside, paths which find the socket
     * closed are not going to be very useful */
    r = syscall(__NR_close, f->fd);
  } else {
    r = 0;
  }

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
    void* incoming = buf;
    if ((f->flags & eSocket)  && klee_zest_enabled()) {
      // HAVE to read the data in a pristine concrete variable.
      // otherwise the concrete values may not be copied by mempcpy
      // and klee_make_symbolic won't be able to create the seeds
      incoming = malloc(count);
      if (!incoming) {
        assert(0 && "unable to allocate memory");
      }
    }

    /* XXX Paul: pread breaks dup'ed descriptors */ 
    if (f->fd == 0 || (f->flags & eSocket))
      r = syscall(__NR_read, f->fd, incoming, count);
    else
      r = syscall(__NR_pread64, f->fd, buf, count, (off64_t) f->off);
 
    if ((f->flags & eSocket)  && klee_zest_enabled()) {
      if (-1 != r) {
        klee_make_symbolic(incoming, count, "read data");
        memcpy(buf, incoming, r);
      }
      free(incoming);
    }
   
    if (r == -1) {
      errno = klee_get_errno();
      return -1;
    }
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

ssize_t pread(int fd, void *buf, size_t count, off_t offset) {
  static int n_calls = 0;
  exe_file_t *f;

  n_calls++;

  if (count == 0)
    return 0;

  if (buf == NULL) {
    errno = EFAULT;
    return -1;
  }
  if (offset < 0) {
    errno = EINVAL;
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

    r = syscall(__NR_pread64, f->fd, buf, count, offset);

    if (r == -1) {
      errno = klee_get_errno();
      return -1;
    }

    return r;
  }
  else {
    if (((off64_t)f->dfile->size) < offset)
      return 0;
    /* symbolic file */
    if (offset + count > f->dfile->size) {
      count = f->dfile->size - offset;
    }

    memcpy(buf, f->dfile->contents + offset, count);
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
    char *cbuf = NULL;
    buf = __concretize_ptr(buf);
    count = __concretize_size(count);
    /* XXX In terms of looking for bugs we really should do this check
       before concretization, at least once the routine has been fixed
       to properly work with symbolics. */
    klee_check_memory_access(buf, count);
    UseConcretePath = (klee_zest_enabled() == 1);
    if (UseConcretePath &&
        count > 0) {
      cbuf = malloc(count);
      assert(cbuf);
      memcpy(cbuf, buf, count);
      buf = __concretize_buffer(cbuf, count);
    }
    if (f->fd == 1 || f->fd == 2)
      r = syscall(__NR_write, f->fd, buf, count);
    else r = syscall(__NR_pwrite64, f->fd, buf, count, (off64_t) f->off);
    free(cbuf);

    if (r == -1) {
      errno = klee_get_errno();
      return -1;
    }
    
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
      fprintf(stderr, "WARNING: write() ignores bytes.\n");

    if (f->dfile == __exe_fs.sym_stdout)
      __exe_fs.stdout_writes += actual_count;

    f->off += count;
    return count;
  }
}


off64_t __fd_lseek(int fd, off64_t offset, int whence) {
  off64_t new_off;
  exe_file_t *f = __get_file(fd);
  offset = __concretize_size(offset);
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

    if (new_off == -1) {
      errno = klee_get_errno();
      return -1;
    }

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
#ifdef _DEBUG_STUBS
  printf("stat %s\n", path);
#endif
  char *pathcpy = malloc(strlen(path)+1);
  strcpy(pathcpy, path);
  {
#if __WORDSIZE == 64
    int r = syscall(__NR_stat, __concretize_string(pathcpy), buf);
#else
    int r = syscall(__NR_stat64, __concretize_string(pathcpy), buf);
#endif
    if (r == -1)
      errno = klee_get_errno();
#ifdef _DEBUG_STUBS
    printf("stat returns %d (errno %d)\n", r, errno);
    if (r != 0)
      perror("stat");
#endif
    free(pathcpy);
    return r;
  }
}

int __fd_lstat(const char *path, struct stat64 *buf) {
  exe_disk_file_t *dfile = __get_sym_file(path);
  if (dfile) {
    memcpy(buf, dfile->stat, sizeof(*dfile->stat));
    return 0;
  }

  char *pathcpy = malloc(strlen(path)+1);
  strcpy(pathcpy, path);
  {
#if __WORDSIZE == 64
    int r = syscall(__NR_lstat, __concretize_string(pathcpy), buf);
#else
    int r = syscall(__NR_lstat64, __concretize_string(pathcpy), buf);
#endif
    if (r == -1)
      errno = klee_get_errno();
    free(pathcpy);
    return r;
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

  {
    int r = syscall(__NR_chdir, __concretize_string(path));
    if (r == -1)
      errno = klee_get_errno();
    return r;
  }
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
  } else {
    int r = syscall(__NR_fchdir, f->fd);
    if (r == -1)
      errno = klee_get_errno();
    return r;
  }
}

void* mmap_simple(int fd, size_t length, off_t off) {
  void* buffer = malloc(length);
  __fd_lseek(fd, off, SEEK_SET);

  size_t count = 0;
  ssize_t r;
  while(count < length) {
    r = read(fd, buffer, length-count);
    if (0 == r) {
      klee_warning("mmap_simple: unable to read requested size");
      break;
    } if (-1 == r) {
      klee_warning("mmap_simple: read");
      break;
    }
    count += r;
  }
  return buffer;
}

/* 64 bits only */
void* mmap64(void* addr, size_t length, int prot,
         int flags, int fd, off64_t off) {
  if (NULL == addr && PROT_READ == prot)
    return mmap_simple(fd, length, off*4096);
  else {
    klee_warning("mmap64: unsupported mmap call");
    return (void*)-1l;
  }
  exe_file_t *f = __get_file(fd);
  if (!f) {
    errno = EBADF;
    return (void*)-1l;
  }

  if (f->dfile) {
    klee_warning("mmap64: symbolic file, ignoring (ENOENT)");
    errno = ENOENT;
    return (void*)-1l;
  } else {
    long r = syscall(__NR_mmap, addr, length, prot,
                    flags, f->fd, off);
    if (r == -1)
      errno = klee_get_errno();
    return (void*)r;
  }
}


void* mmap(void* addr, size_t length, int prot,
         int flags, int fd, off_t off) {
  if (NULL == addr && PROT_READ == prot)
    return mmap_simple(fd, length, off);
  else {
    klee_warning("mmap64: unsupported mmap call");
    return (void*)-1l;
  }

  exe_file_t *f = __get_file(fd); 
  if (!f) {
    errno = EBADF;
    return (void*)-1l;
  }

  if (f->dfile) {
    klee_warning("mmap: symbolic file, ignoring (ENOENT)");
    errno = ENOENT;
    return (void*)-1l;
  } else {
    long r = syscall(__NR_mmap, addr, length, prot,
                    flags, f->fd, off);
    if (r == -1)
      errno = klee_get_errno();
    return (void*)r;
  }
}

int munmap(void *addr, size_t length)
{
  free(addr);
  return 0;
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
  } else {
    int r = syscall(__NR_chmod, __concretize_string(path), mode);
    if (r == -1)
      errno = klee_get_errno();
    return r;
  }
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
  } else {
    int r = syscall(__NR_fchmod, f->fd, mode);
    if (r == -1)
      errno = klee_get_errno();
    return r;
  }  
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
  } else {
    int r = syscall(__NR_chown, __concretize_string(path), owner, group);
    if (r == -1)
      errno = klee_get_errno();
    return r;
  }
}

int fchown(int fd, uid_t owner, gid_t group) {
  exe_file_t *f = __get_file(fd);

  if (!f) {
    errno = EBADF;
    return -1;
  }

  if (f->dfile) {
    return __df_chown(f->dfile, owner, group);
  } else {
    int r = syscall(__NR_fchown, f->fd, owner, group);
    if (r == -1)
      errno = klee_get_errno();
    return r;
  }
}

int lchown(const char *path, uid_t owner, gid_t group) {
  /* XXX Ignores 'l' part */
  exe_disk_file_t *df = __get_sym_file(path);

  if (df) {
    return __df_chown(df, owner, group);
  } else {
    int r = syscall(__NR_lchown, __concretize_string(path), owner, group);
    if (r == -1)
      errno = klee_get_errno();
    return r;
  }
}

int __fd_fstat(int fd, struct stat64 *buf) {
  exe_file_t *f = __get_file(fd);

#ifdef _DEBUG_STUBS
  if (f)
    printf("fstat %d(%d)\n", f->fd, fd);
  else
    printf("fstat %d, __get_file failed\n", fd);
#endif
 
  if (!f) {
    errno = EBADF;
    return -1;
  }
 
  if (!f->dfile) {
#if __WORDSIZE == 64
    int r = syscall(__NR_fstat, f->fd, buf);
#else
    int r = syscall(__NR_fstat64, f->fd, buf);
#endif
    if (r == -1)
      errno = klee_get_errno();
#ifdef _DEBUG_STUBS
  printf("fstat returns %d (errno %d)\n", r, errno);
  if (r != 0)
    perror("fstat");
#endif
    return r;
  }
#ifdef _DEBUG_STUBS
  printf("fstat: using symbolic stat data\n");
#endif
  memcpy(buf, f->dfile->stat, sizeof(*f->dfile->stat));
  return 0;
}

int fstatat(int fd, const char* path, struct stat *buf, int flags) {
  exe_disk_file_t *dfile = __get_sym_file(path);
  if (dfile) {
    klee_warning("fstatat: symbolic path, ignoring (ENOENT)");
    errno = ENOENT;
    return -1;
  }
  exe_file_t* f = __get_file(fd);
  //keep in mind that fd can have 'magic' values
  if (f && f->dfile) {
    klee_warning("fstatat: symbolic file descriptor, ignoring (ENOENT)");
    errno = ENOENT;
    return -1;
  }
#ifdef _DEBUG_STUBS
  if (f)
    printf("fstatat %d(%d) %s\n", f->fd, fd, path);
  else
    printf("fstatat %d(nil) %s\n", fd, path);
#endif
  if (f) fd = f->fd;
 
#if (defined __NR_newfstatat) && (__NR_newfstatat != 0)
  int r = syscall(__NR_newfstatat, (long)fd,
               (path ? __concretize_string(path) : NULL),
               buf, (long)flags); 
#else
  int r = syscall(__NR_fstatat64, (long)fd,
               (path ? __concretize_string(path) : NULL),
               buf, (long)flags);
#endif
  if (r == -1)
    errno = klee_get_errno();
#ifdef _DEBUG_STUBS
  printf("fstatat returns %d (errno %d)\n", r, errno);
  if (r == -1)
    perror("fstatat");
#endif
  return r;
}

int unlinkat(int fd, const char* path, int flags) {
  exe_disk_file_t *dfile = __get_sym_file(path);
  if (dfile) {
    klee_warning("unlinkat: symbolic path, ignoring (ENOENT)");
    errno = ENOENT;
    return -1;
  }
  exe_file_t* f = __get_file(fd);
  if (f && f->dfile) {
    klee_warning("unlinkat: symbolic file descriptor, ignoring (ENOENT)");
    errno = ENOENT;
    return -1;
  }
#ifdef _DEBUG_STUBS
  if (f)
    printf("unlinkat %d(%d) %s\n", f->fd, fd, path);
  else
    printf("unlinkat %d(nil) %s\n", fd, path);
#endif
  if (f) fd = f->fd;
  int r = syscall(__NR_unlinkat, (long)fd, 
               (path ? __concretize_string(path) : NULL),
               (long)flags);
  if (r == -1)
    errno = klee_get_errno();
#ifdef _DEBUG_STUBS
  printf("unlinkat returns %d (errno %d)\n", r, errno);
  if (r != 0)
    perror("unlinkat");
#endif
  return r;
}

int futimesat(int fd, const char* path, const struct timeval times[2]) {
  exe_disk_file_t *dfile = __get_sym_file(path);
  if (dfile) {
    klee_warning("futimesat: symbolic path, ignoring (ENOENT)");
    errno = ENOENT;
    return -1;
  }
  exe_file_t* f = __get_file(fd);
  if (f && f->dfile) {
    klee_warning("futimesat: symbolic file descriptor, ignoring (ENOENT)");
    errno = ENOENT;
    return -1;
  }
  if (f) fd = f->fd;
#ifdef _DEBUG_STUBS
  printf("futimesat %d %s %ld %ld\n", fd, path, times[0].tv_sec, times[0].tv_usec);
#endif    
 int r = syscall(__NR_futimesat, (long)fd, 
                 (path ? __concretize_string(path) : NULL),
                 times);
  if (r == -1)
    errno = klee_get_errno();
#ifdef _DEBUG_STUBS
  printf("futimesat returns %d (errno %d)\n", r, errno);
  if (r == -1)
    perror("futimesat");
#endif
  return r;
}

int futimens(int fd, const struct timespec times[2]) {
  exe_file_t* f = __get_file(fd);
  if (f && f->dfile) {
    klee_warning("futimens: symbolic file descriptor, ignoring (ENOENT)");
    errno = ENOENT;
    return -1;
  }
  if (!f) {
    errno = EBADF;
    return -1;
  }

  //see the man page for details
  int r = syscall(__NR_utimensat, (long)f->fd, 0L,
                  times, 0L);
  if (r == -1)
    errno = klee_get_errno();
  return r;
}


int link(const char* oldpath, const char* newpath) {
  exe_disk_file_t *dfile = __get_sym_file(oldpath);
  if (dfile) {
    klee_warning("link: symbolic path, ignoring (ENOENT)");
    errno = ENOENT;
    return -1;
  }
  dfile = __get_sym_file(newpath);
  if (dfile) {
    klee_warning("link: symbolic path, ignoring (ENOENT)");
    errno = ENOENT;
    return -1;
  }
    int r = syscall(__NR_link, __concretize_string(oldpath), __concretize_string(newpath));
    if (r == -1)
      errno = klee_get_errno();
    return r;
}

int symlink(const char* oldpath, const char* newpath) {
  exe_disk_file_t *dfile = __get_sym_file(oldpath);
  if (dfile) {
    klee_warning("symlink: symbolic path, ignoring (ENOENT)");
    errno = ENOENT;
    return -1;
  }
  dfile = __get_sym_file(newpath);
  if (dfile) {
    klee_warning("symlink: symbolic path, ignoring (ENOENT)");
    errno = ENOENT;
    return -1;
  }
    int r = syscall(__NR_symlink, __concretize_string(oldpath), __concretize_string(newpath));
    if (r == -1)
      errno = klee_get_errno();
    return r;
}

int openat(int fd, const char* path, int flags, ...) {
  //XXX add support for symbolic files
  //exe_disk_file_t *df;
  exe_file_t *f;
  int fd_idx;

  f = __get_file(fd);

  if (f && f->dfile) {
    klee_warning("openat: symbolic file descriptor, ignoring (ENOENT)");
    errno = ENOENT;
    return -1;
  }
#ifdef _DEBUG_STUBS
  if (f)
    printf("openat %d(%d) %s\n", f->fd, fd, path);
  else
    printf("openat %d(nil) %s\n", fd, path);
#endif
  if (f) fd = f->fd;
  for (fd_idx = 0; fd_idx < MAX_FDS; ++fd_idx)
    if (!(__exe_env.fds[fd_idx].flags & eOpen))
      break;
  if (fd_idx == MAX_FDS) {
    errno = EMFILE;
    return -1;
  }
  
  f = &__exe_env.fds[fd_idx];
  va_list ap;
  mode_t mode = 0;
  if (flags & O_CREAT) {
    va_start(ap, flags);
    mode = va_arg(ap, mode_t);
    va_end(ap);
  }
  // memset is expensive. we already do it in close
  // memset(f, 0, sizeof *f);
  /* get_sym_file_internal does not implement openat semantics. fall back to concrete file */
#if 0
  df = __get_sym_file_internal(pathname, flags, ((flags & O_WRONLY || flags & O_RDWR) ? false : true)); 
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
      fprintf(stderr, "Undefined call to open(): O_TRUNC | O_RDONLY\n");
      errno = EACCES;
      return -1;
    }

    if ((flags & O_EXCL) && !(flags & O_CREAT)) {
      /* The result of using O_EXCL without O_CREAT is undefined, so
	 we return error */
      fprintf(stderr, "Undefined call to open(): O_EXCL w/o O_RDONLY\n");
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
  } else
#endif  
  {
     int os_fd = syscall(__NR_openat, (long)fd, __concretize_string(path), (long)flags, mode);
     if (os_fd == -1)
       errno = klee_get_errno();
#ifdef _DEBUG_STUBS
     printf("openat returns %d (errno %d)\n", os_fd, errno);
     if (os_fd < 0)
        perror("openat");
#endif
     if (os_fd == -1)
       return os_fd;
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
#if USE_TRANSPARENT_FDS
  return f->fd;
#else
  return fd_idx;
#endif
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
  } else {
    length = __concretize_size(length);
#if __WORDSIZE == 64
    int r = syscall(__NR_ftruncate, f->fd, length);
#else
    int r = syscall(__NR_ftruncate64, f->fd, length);
#endif
    if (r == -1)
      errno = klee_get_errno();
    return r;
  }  
}

int __fd_getdents(unsigned int fd, struct dirent64 *dirp, unsigned int count) {
  exe_file_t *f = __get_file(fd);

  if (!f) {
    errno = EBADF;
    return -1;
  }
  
#ifdef _DEBUG_STUBS
  printf("getdents %d(%d) count=%u offset=%ul\n", f->fd, fd, count, (long)f->off);
#endif
  if (f->dfile) {
    klee_warning("getdents called on symbolic file, ignoring (EINVAL)");
    errno = EINVAL;
    return -1;
  } else {
    // Paul: the code below always injects symbolic files in readdir calls
    // disabled for now
    if (INJECT_SYMBOLIC_DIR_ENTRIES && (unsigned long) f->off < 4096u) {
      /* Return our dirents */
      unsigned i, pad, bytes=0;

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
        dirp->d_off = (i+1) * sizeof(*dirp);
        bytes += dirp->d_reclen;
        ++dirp;
      }
      
      /* Fake jump to OS records by a "deleted" file. */
      pad = count>=4096 ? 4096 : count;
      dirp->d_ino = 0;
      dirp->d_reclen = pad - bytes;
      dirp->d_type = DT_UNKNOWN;
      dirp->d_name[0] = '\0';
      dirp->d_off = 4096;
      bytes += dirp->d_reclen;
      f->off = pad;
      return bytes;
    } else {
#if INJECT_SYMBOLIC_DIR_ENTRIES
      unsigned os_pos = f->off - 4096;
#endif
      int res;

      /* For reasons which I really don't understand, if I don't
         memset this then sometimes the kernel returns d_ino==0 for
         some valid entries? Am I crazy? Can writeback possibly be
         failing? 
      
         Even more bizarre, interchanging the memset and the seek also
         case strange behavior. Really should be debugged properly. */
      memset(dirp, 0, count);
#if INJECT_SYMBOLIC_DIR_ENTRIES
      int s;
      s = syscall(__NR_lseek, f->fd, (int) os_pos, SEEK_SET);
      assert(s != (off64_t) -1);
#endif
      res = syscall(__NR_getdents64, f->fd, dirp, count);
      if (res == -1) {
        errno = klee_get_errno();
      } else {
#if INJECT_SYMBOLIC_DIR_ENTRIES
        int pos = 0;

        f->off = syscall(__NR_lseek, f->fd, 0, SEEK_CUR) + 4096;

        /* Patch offsets */

        while (pos < res) {
          struct dirent64 *dp = (struct dirent64*) ((char*) dirp + pos);
          dp->d_off += 4096;
          pos += dp->d_reclen;
        }
#endif
      }
      return res;
    }
  }
}

#if 0
#if __WORDSIZE == 64
int ioctl(int fd, unsigned long int request, ...) {
#else
int ioctl(int fd, unsigned long request, ...) {
#endif
  exe_file_t *f = __get_file(fd);
  va_list ap;
  void *buf;


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
        ts->c_line = 0;
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
  } else {
    int r = syscall(__NR_ioctl, f->fd, request, buf );
    if (r == -1) 
      errno = klee_get_errno();
    return r;
  }
}
#endif

int fcntl(int fd, int cmd, ...) {
  exe_file_t *f = __get_file(fd);
  va_list ap;
  unsigned arg; /* 32 bit assumption (int/ptr) */

#ifdef _DEBUG_STUBS
  if (f)
    printf("fcntl %d(%d)\n", f->fd, fd);
  else
    printf("fcntl %d(nil)\n", fd);
    char x[512];
    sprintf(x, "fcntl file %d %d\n", fd, (f ? f->fd : -666));
    write(2, x, strlen(x)+1);
    klee_warning("XXX");
#endif

  if (!f) {
    errno = EBADF;
    return -1;
  }
  
  if (cmd==F_GETFD || cmd==F_GETFL || cmd==F_GETOWN || cmd==F_GETSIG ||
      cmd==F_GETLEASE || cmd==F_NOTIFY) {
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
    case F_GETFL:
    case F_SETFL: {
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
  } else {
    int r = syscall(__NR_fcntl, f->fd, cmd, arg );
    if (r == -1)
      errno = klee_get_errno();
    return r;
  }
}

int __fd_statfs(const char *path, struct statfs *buf) {
  exe_disk_file_t *dfile = __get_sym_file(path);
  if (dfile) {
    /* XXX incorrect */
    klee_warning("symbolic file, ignoring (ENOENT)");
    errno = ENOENT;
    return -1;
  }

  {
    int r = syscall(__NR_statfs, __concretize_string(path), buf);
    if (r == -1)
      errno = klee_get_errno();
    return r;
  }
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
  } else {
    int r = syscall(__NR_fstatfs, f->fd, buf);
    if (r == -1)
      errno = klee_get_errno();
    return r;
  }
}

int fsync(int fd) {
  exe_file_t *f = __get_file(fd);

  if (!f) {
    errno = EBADF;
    return -1;
  } else if (f->dfile) {
    return 0;
  } else {
    int r = syscall(__NR_fsync, f->fd);
    if (r == -1)
      errno = klee_get_errno();
    return r;
  }
}

int dup2(int oldfd, int newfd) {
  exe_file_t *f = __get_file(oldfd);

#ifdef _DEBUG_STUBS
  printf("dup2 %d %d\n", oldfd, newfd);
#endif
  if (!f || !(newfd >= 0 && newfd < MAX_FDS)) {
    errno = EBADF;
    return -1;
  } 
  
  int i, idx = -1;
  for (i = 0; i < MAX_FDS; ++i) {
    if (__exe_env.fds[i].fd == newfd) {
      idx = i;
      break;
    }
    if (idx == -1 && !(__exe_env.fds[i].flags & eOpen))
      idx = i;
  }
  if (-1 == idx) {
    errno = EMFILE;
    return -1;
  }

  exe_file_t *f2 = &__exe_env.fds[idx];
  if (f2->flags & eOpen) close(newfd);
#if USE_TRANSPARENT_FDS
  newfd = syscall(__NR_dup2, oldfd, newfd);
  if (-1 == newfd)
    errno = klee_get_errno();
  else
    f2->flags = f->flags & (~eCloseOnExec);
#else
  /* XXX Incorrect, really we need another data structure for open
     files */
  *f2 = *f;

  f2->flags &= ~eCloseOnExec;
      
  /* I'm not sure it is wise, but we can get away with not dup'ng
     the OS fd, since actually that will in many cases effect the
     sharing of the open file (and the process should never have
     access to it). */
#endif
  return newfd;
}

int dup(int oldfd) {
  exe_file_t *f = __get_file(oldfd);
  if (!f) {
    errno = EBADF;
    return -1;
  } else {
#if USE_TRANSPARENT_FDS
    klee_warning_once("Cannot guarantee dup semantics with transparent file descriptors\n");
#endif
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
  } else {
    int r = syscall(__NR_rmdir, __concretize_string(pathname));
    if (r == -1)
      errno = klee_get_errno();
    return r;
  }
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
  } else {
    int r = syscall(__NR_unlink, __concretize_string(pathname));
    if (r == -1)
      errno = klee_get_errno();
    return r;
  }
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
  } else {
    int r = syscall(__NR_readlink, path, buf, bufsize);
    if (r == -1)
      errno = klee_get_errno();
    return r;
  }
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
  //char msg[64];
  //int sec = -1; int usec = 0;
  //if(timeout) { sec = timeout->tv_sec; usec = timeout->tv_usec;} 

  //sprintf(msg, "model select for %d fds. timeout %d s %d us", nfds, sec, usec);
  //klee_warning(msg);

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
        unsigned flags = 0;
        if (FD_ISSET(i, &in_read)) {
          //sprintf(msg, "read on %d", i);
          //klee_warning(msg);
          if (!(f->flags & eSocket))
            flags |= 01;
          else if (f->flags & eDgramSocket)
            flags |= (__exe_fs.n_sym_dgrams_used  < __exe_fs.n_sym_dgrams)  ? 01 : 0; /* more dgrams available */
          else if (f->flags & eListening)
            flags |= (__exe_fs.n_sym_streams_used < __exe_fs.n_sym_streams) ? 01 : 0; /* more streams available */
          else
            flags |= 01;
          if (flags & 01) FD_SET(i, read);
        }
        if (FD_ISSET(i, &in_write)) {
          //sprintf(msg, "write on %d", i);
          //klee_warning(msg);
          flags |= 02; FD_SET(i, write);
        }
        //if (FD_ISSET(i, &in_except)) { flags |= 04; FD_SET(i, except); }
        if (flags) ++count;
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
      if (!count) {
        errno = klee_get_errno();
        return -1;
      }
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
  if (r == -1) {
    errno = klee_get_errno();
    return NULL;
  }
    
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
  
  for (i=0; ; ++i) {
    char c = *sc;
    if (!UseConcretePath && !(i&(i-1))) {
      if (!c) {
        *sc++ = 0;
        break;
      } else if (c=='/') {
        *sc++ = '/';
      } 
    } else {
      char cc = (char) klee_get_valuel((long)c);
      klee_assume(cc == c);
      *sc++ = cc;
      if (!cc) break;
    }
  }

  return s;
}

static const char *__concretize_buffer(const char *s, unsigned size) {
  char *sc = __concretize_ptr(s);
  unsigned i;

  for (i=0; i < size; ++i) {
    char c = *sc;
    char cc = (char) klee_get_valuel((long)c);
    klee_assume(cc == c);
    *sc++ = cc;
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

int utimes(const char *path, const struct timeval* times) {
  exe_disk_file_t *dfile = __get_sym_file(path);

  if (dfile) {
    klee_warning("utimes: symbolic file, ignoring (ENOENT)");
    errno = ENOENT;
    return -1;
  }
  /* may not be needed but got spurious results with
     coreutils tests without
  */
  int r = syscall(__NR_utimes, __concretize_string(path), times);
  if (r == -1)
    errno = klee_get_errno();

  return r;
}

int utime(const char *path, const void* times) {
  exe_disk_file_t *dfile = __get_sym_file(path);

  if (dfile) {
    /* XXX incorrect */
    klee_warning("utime: symbolic file, ignoring (ENOENT)");
    errno = ENOENT;
    return -1;
  }

  int r = syscall(__NR_utime, __concretize_string(path), times);
  if (r == -1)
    errno = klee_get_errno();
  return r;
}

/* Returns next available fd.  Sets eOpen in associated flags.  
   If no more fds are available, returns -1 and sets errno to ENFILE.
   Otherwise, set *pf to &__exe_env.fds[fd]. */
int __get_new_fd(exe_file_t **pf) {
  int fd;

  for (fd = 0; fd < MAX_FDS; ++fd)
    if (!(__exe_env.fds[fd].flags & eOpen))
      break;
  if (fd == MAX_FDS) {
    errno = ENFILE;
    return -1;
  }
  
  *pf = &__exe_env.fds[fd];

  /* Should be the case if file was available, but just in case. */
  memset(*pf, 0, sizeof **pf);
  (*pf)->fd = -1;
  (*pf)->flags = eOpen;

  return fd;
}


void  __undo_get_new_fd(exe_file_t *f) {
  memset(f, 0, sizeof *f);
}

int pipe(int filedes[2]) __attribute__((weak));
int pipe(int filedes[2]) {
  exe_file_t *f[2];
  int os_filedes[2];
  int r;

  if ((filedes[0] = __get_new_fd(&f[0])) < 0) {
    errno = klee_get_errno();
    return filedes[0];
  }
  if ((filedes[1] = __get_new_fd(&f[1])) < 0) {
    errno = klee_get_errno();
    return filedes[1];
  }

  r = syscall(__NR_pipe, os_filedes);
  if (r < 0) {
    errno = klee_get_errno();
    return r;
  }

  f[0]->fd = os_filedes[0]; f[0]->flags = eOpen | eReadable;
  f[1]->fd = os_filedes[1]; f[1]->flags = eOpen | eWriteable;
  fprintf(stderr, "KLEE: MODEL: pipe: returning {%d,%d}->{%d,%d}\n", filedes[0], filedes[1], os_filedes[0], os_filedes[1]);
  return 0;
}

ssize_t __fd_scatter_read(exe_file_t *f, const struct iovec *iov, int iovcnt)
{
  klee_warning("scatter read malfunctions when provided a symbolic iovec");
  if (f->dfile) {
    ssize_t total = 0;

    assert(f->off >= 0);
    if (f->dfile->size < f->off)
      return 0;

    for (; iovcnt > 0; iov++, iovcnt--) {
      size_t this_len = iov->iov_len;
      if (f->off + this_len > f->dfile->size)
        this_len = f->dfile->size - f->off;
      memcpy(iov->iov_base, f->dfile->contents + f->off, this_len);
      total += this_len;
      f->off += this_len;
    }

    return total;
  } else {
    ssize_t os = syscall(__NR_readv, f->fd, iov, iovcnt);
    if (os < 0)
      errno = klee_get_errno();
    else if (klee_zest_enabled()) {
      char *p;
      ssize_t cb = os;
      size_t this_len;
      while (cb) {
        if ((size_t)cb > iov->iov_len) {
          this_len = iov->iov_len;
          cb -= iov->iov_len;
        } else {
          this_len = cb;
          cb = 0;
        }
        p = malloc(this_len);
        if (!p) {
          assert(0 && "unable to allocate memory");
        }
        memcpy(p, iov->iov_base, this_len);
        klee_make_symbolic(p, this_len, "readv data");
        memcpy(iov->iov_base, p, this_len);
        free(p);
        ++iov;
      }
    }
    return os;
  }
}

ssize_t __fd_gather_write(exe_file_t *f, const struct iovec *iov, int iovcnt)
{
  ssize_t total = 0;

  if (f->dfile) {
    for (; iovcnt > 0; iov++, iovcnt--) {
      size_t this_len = iov->iov_len;
      if (f->dfile->src) {
        /* writes to a symbolic socket always succeed, ignored */
        klee_check_memory_access(iov->iov_base, iov->iov_len);
        goto skip;
      }

      if (f->off + this_len > f->dfile->size) {
        if (__exe_env.save_all_writes)
          assert(0);
        else {
          if (f->off < f->dfile->size)
            this_len = f->dfile->size - f->off;
          else
            this_len = 0;
        }
      }

      if (this_len)
        memcpy(f->dfile->contents + f->off, iov->iov_base, this_len);
      
      if (this_len != iov->iov_len)
        klee_warning_once("write() ignores bytes.");

      if (f->dfile == __exe_fs.sym_stdout)
        __exe_fs.stdout_writes += this_len;

    skip:
      total += iov->iov_len;
      f->off += iov->iov_len;
    }

    return total;
  } else {
    int os = syscall(__NR_writev, f->fd, iov, iovcnt);
    if (os < 0)
      errno = klee_get_errno();
    return os;
  }
}

ssize_t writev(int fd, const struct iovec *iov, int iovcnt) {
  exe_file_t *f;

  f = __get_file(fd);
  if (!f) {
    errno = EBADF;
    return -1;
  }

  if (!f->dfile) {
    //concrete case is over-simplistic
    int r;
    r = syscall(__NR_writev, f->fd, iov, iovcnt);

    if (r == -1) {
      errno = klee_get_errno();
      return -1;
    }
    return r;
  }
  else {
    return __fd_gather_write(f, iov, iovcnt);
  }
}

int ioctl(int fd, unsigned long request, ...) {
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
        ts->c_line = 0;
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
        if (f->off < f->dfile->size) {
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
    case SIOCGIFINDEX: {
      struct ifreq *ifr = buf;
      ifr->ifr_ifindex = 0;
      return 0;
    }
    case SIOCGIFHWADDR: {
      struct ifreq *ifr = buf;
      char (*ia)[14] = &ifr->ifr_hwaddr.sa_data;
    /*klee_make_symbolic(ia, sizeof(*ia), "IFADDR");*/
      memset(ia, 0xdd, sizeof(*ia));
      return 0;
    }
    case SIOCGIFADDR: {
      struct ifreq *ifr = buf;
      switch (ifr->ifr_addr.sa_family) {
      default:
        ifr->ifr_addr.sa_family = AF_INET;
        /* fall through */
      case AF_INET: {
        struct in_addr* ia = &((struct sockaddr_in *)&ifr->ifr_addr)->sin_addr;
      /*klee_make_symbolic(ia, sizeof(*ia), "IFADDR");*/
        memset(ia, 0xdd, sizeof(*ia));
        return 0;
      }
      case AF_INET6: {
        struct in6_addr* ia = &((struct sockaddr_in6 *)&ifr->ifr_addr)->sin6_addr;
      /*klee_make_symbolic(ia, sizeof(*ia), "IFADDR");*/
        memset(ia, 0xdd, sizeof(*ia));
        return 0;
      }
      }
    }
    default:
      klee_warning("symbolic file, ignoring (EINVAL)");
      errno = EINVAL;
      return -1;
    }
  } else {
    int r = syscall(__NR_ioctl, f->fd, request, buf );
    if (r == -1) 
      errno = klee_get_errno();
    return r;
  }
}

ssize_t sendfile(int out_fd, int in_fd, off_t *offset, size_t count)
{
  exe_file_t *out_f = __get_file(out_fd);
  exe_file_t *in_f = __get_file(in_fd);

  if (out_f->dfile || in_f->dfile) {
    errno = EINVAL;
    return -1;
  } else {
    ssize_t os = syscall(__NR_sendfile, out_f->fd, in_f->fd, offset, count);
    if (os < 0)
      errno = klee_get_errno();
    return os;
  }
}

ssize_t sendfile64(int out_fd, int in_fd, off_t *offset, size_t count)
{
  return sendfile(out_fd, in_fd, offset, count);
}

