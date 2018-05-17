//===-- fd.h ---------------------------------------------------*- C++ -*--===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef __EXE_FD__
#define __EXE_FD__

#ifndef _LARGEFILE64_SOURCE
#error "_LARGEFILE64_SOURCE should be defined"
#endif
#include <sys/types.h>
#include <sys/statfs.h>
#include <dirent.h>
#include <netinet/in.h>


typedef struct {
    struct sockaddr_storage *addr;
    socklen_t addrlen;
} exe_sockaddr_t;

typedef struct {  
  unsigned size;  /* in bytes */
  char* contents;
  char* path;
  struct stat64* stat;
  exe_sockaddr_t* src;
} exe_disk_file_t;

typedef enum {
  eOpen         = (1 << 0),
  eCloseOnExec  = (1 << 1),
  eReadable     = (1 << 2),
  eWriteable    = (1 << 3),
  eSocket       = (1 << 4),
  eDgramSocket  = (1 << 5),
  eListening    = (1 << 6)
} exe_file_flag_t;

typedef struct exe_file_t {
  int fd;                   /* actual fd if not symbolic */
  unsigned flags;           /* set of exe_file_flag_t values. fields
                               are only defined when flags at least
                               has eOpen. */
  off64_t off;              /* offset */
  exe_disk_file_t* dfile;   /* ptr to file on disk, if symbolic */
  exe_sockaddr_t local;
  exe_sockaddr_t *foreign; /* socket addresses, if a symbolic socket;
                              symbolic sockets have the addr allocated
                              and addrlen properly assigned.  If TCP,
                              then foreign points to dfile->src */
  int domain;              /* if socket */
} exe_file_t;

typedef struct {
  unsigned n_sym_files; /* number of symbolic input files, excluding stdin */
  unsigned n_sym_files_used;
  exe_disk_file_t *sym_stdin, *sym_stdout;
  unsigned stdout_writes; /* how many chars were written to stdout */
  exe_disk_file_t *sym_files;
  /* --- */
  unsigned n_sym_streams;
  unsigned n_sym_streams_used;
  exe_disk_file_t *sym_streams;

  unsigned n_sym_dgrams;
  unsigned n_sym_dgrams_used;
  exe_disk_file_t *sym_dgrams;
  /* the maximum number of failures on one path; gets decremented after each failure */
  unsigned max_failures; 

  /* Which read, write etc. call should fail */
  int *read_fail, *write_fail, *close_fail, *ftruncate_fail, *getcwd_fail;
  int *chmod_fail, *fchmod_fail;
} exe_file_system_t;

#define MAX_FDS 32

/* Note, if you change this structure be sure to update the
   initialization code if necessary. New fields should almost
   certainly be at the end. */
typedef struct {
  exe_file_t fds[MAX_FDS];
  mode_t umask; /* process umask */
  unsigned version;
  /* If set, writes execute as expected.  Otherwise, writes extending
     the file size only change the contents up to the initial
     size. The file offset is always incremented correctly. */
  int save_all_writes; 
} exe_sym_env_t;

extern exe_file_system_t __exe_fs;
extern exe_sym_env_t __exe_env;

void klee_init_fds(unsigned n_files, unsigned file_length,  unsigned sym_stdin_len,
		   int sym_stdout_flag, int do_all_writes_flag, 
                   unsigned n_streams, unsigned stream_len,
                   unsigned n_dgrams, unsigned dgram_len,
		   unsigned max_failures);

void klee_init_env(int *argcPtr, char ***argvPtr);
exe_file_t* __get_file(int fd);

int __get_new_fd(exe_file_t **pf);
void __undo_get_new_fd(exe_file_t *f);

/* *** */

int __fd_open(const char *pathname, int flags, mode_t mode);
int __fd_openat(int basefd, const char *pathname, int flags, mode_t mode);
off64_t __fd_lseek(int fd, off64_t offset, int whence);
int __fd_stat(const char *path, struct stat64 *buf);
int __fd_lstat(const char *path, struct stat64 *buf);
int __fd_fstat(int fd, struct stat64 *buf);
int __fd_ftruncate(int fd, off64_t length);
int __fd_statfs(const char *path, struct statfs *buf);
int __fd_getdents(unsigned int fd, struct dirent64 *dirp, unsigned int count);
ssize_t __fd_scatter_read(exe_file_t *f, const struct iovec *iov, int iovcnt);
ssize_t __fd_gather_write(exe_file_t *f, const struct iovec *iov, int iovcnt);

int socket(int, int, int);

#endif /* __EXE_FD__ */
