//===-- fd_init.c ---------------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#define _LARGEFILE64_SOURCE
#define _FILE_OFFSET_BITS 64
#include "fd.h"
#include <klee/klee.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <unistd.h>

int klee_zest_enabled() { return 0;}

exe_file_system_t __exe_fs;

/* NOTE: It is important that these are statically initialized
   correctly, since things that run before main may hit them given the
   current way things are linked. */

/* XXX Technically these flags are initialized w.o.r. to the
   environment we are actually running in. We could patch them in
   klee_init_fds, but we still have the problem that uclibc calls
   prior to main will get the wrong data. Not such a big deal since we
   mostly care about sym case anyway. */


exe_sym_env_t __exe_env = { 
  {{ 0, eOpen | eReadable,  0, NULL, {NULL, 0}, NULL, 0},
   { 1, eOpen | eWriteable, 0, NULL, {NULL, 0}, NULL, 0},
   { 2, eOpen | eWriteable, 0, NULL, {NULL, 0}, NULL, 0}},
  022,
  0,
  0
};

/* XXX: We should provide better support for such options! */
static int __one_line_streams = 0;
static const fill_info_t *__stream_fill_info = NULL;
static unsigned __n_stream_fill_info = 0;
static const fill_info_t *__dgram_fill_info = NULL;
static unsigned __n_dgram_fill_info = 0;

void __fill_blocks(exe_disk_file_t* dfile, const fill_info_t* fill_info, unsigned n_fill_info, int do_memset) {
  unsigned i, j;
  int s;
  char *file;
  for (i = 0; i < n_fill_info; i++) {
    const fill_info_t *p = &fill_info[i];
    switch (p->fill_method) {
    case fill_set:
      /* effectively memset(dfile->contents + p->offset, p->arg.value, p->length) */
      if (do_memset) {
        memset(dfile->contents + p->offset, p->arg.value, p->length);
      } else {
        for (j = 0; j < p->length; j++) {
          klee_assume((unsigned char) dfile->contents[p->offset + j] ==
                      (unsigned char) p->arg.value);
        }
      }
      break;
    case fill_copy:
      /* effectively memcpy(dfile->contents + p->offset, p->arg.string, p->length) */
      if (do_memset) {
        memcpy(dfile->contents + p->offset, p->arg.string, p->length);
      } else {
        for (j = 0; j < p->length; j++) {
          klee_assume(dfile->contents[p->offset + j] == p->arg.string[j]);
        }
      }
      break;
    case fill_file:
      s = native_read_file(p->arg.string, 0, &file);
      assert(s >= 0 && "unable to open fill file");
      assert(dfile->size >= p->offset + s && "fill file to big for socket");
      if (do_memset) {
        memcpy(dfile->contents + p->offset, file, s);
      } else {
        for (j = 0; j < (unsigned)s; j++) {
          klee_assume(dfile->contents[p->offset + j] == file[j]);
        }
      }
      break;
    default:
      assert(0 && "unknown fill method");
    }
  }
}


static void __create_new_dfile(exe_disk_file_t *dfile, unsigned size, char* contents, 
                               const char *name,
                               const fill_info_t* fill_info, unsigned n_fill_info,
                               struct stat64 *defaults, int is_foreign) {
  struct stat64 *s = malloc(sizeof(*s));
  const char *sp;
  char sname[256], src_name[256];
  for (sp=name; *sp; ++sp)
    sname[sp-name] = *sp;
  memcpy(&sname[sp-name], "-stat", 6);

  assert(contents || size);

  dfile->size = size;
  char* original_file = NULL;
  if (contents) {
    original_file = malloc(size);
    memcpy(original_file, contents, size);
    dfile->contents = contents;
  } else {
    dfile->contents = malloc(dfile->size);
  }

  if (n_fill_info && klee_zest_enabled()) {
    __fill_blocks(dfile, fill_info, n_fill_info, 1);
    klee_make_symbolic(dfile->contents, dfile->size, name);
  } else {
    klee_make_symbolic(dfile->contents, dfile->size, name);
    __fill_blocks(dfile, fill_info, n_fill_info, 0);
  }

  // need to do the same for fill_blocks data
  if (contents) {
    unsigned i;
    for (i = 0; i < dfile->size; ++i) {
      klee_prefer_cex(dfile->contents, dfile->contents[i] == original_file[i]);
    }
    free(original_file);
  }

  s->st_size = size;
  klee_make_symbolic(s, sizeof(*s), sname);

  /* For broken tests */
  if (!klee_is_symbolic(s->st_ino) && 
      (s->st_ino & 0x7FFFFFFF) == 0)
    s->st_ino = defaults->st_ino;
  
  /* Important since we copy this out through getdents, and readdir
     will otherwise skip this entry. For same reason need to make sure
     it fits in low bits. */
  klee_assume((s->st_ino & 0x7FFFFFFF) != 0);

  /* uclibc opendir uses this as its buffer size, try to keep
     reasonable. */
  klee_assume((s->st_blksize & ~0xFFFF) == 0);

  klee_prefer_cex(s, !(s->st_mode & ~(S_IFMT | 0777)));
  klee_prefer_cex(s, s->st_dev == defaults->st_dev);
  klee_prefer_cex(s, s->st_rdev == defaults->st_rdev);
  klee_prefer_cex(s, (s->st_mode&0700) == 0600);
  klee_prefer_cex(s, (s->st_mode&0070) == 0020);
  klee_prefer_cex(s, (s->st_mode&0007) == 0002);
  klee_prefer_cex(s, (s->st_mode&S_IFMT) == S_IFREG);
  klee_prefer_cex(s, s->st_nlink == 1);
  klee_prefer_cex(s, s->st_uid == defaults->st_uid);
  klee_prefer_cex(s, s->st_gid == defaults->st_gid);
  klee_prefer_cex(s, s->st_blksize == 4096);
  klee_prefer_cex(s, s->st_atime == defaults->st_atime);
  klee_prefer_cex(s, s->st_mtime == defaults->st_mtime);
  klee_prefer_cex(s, s->st_ctime == defaults->st_ctime);

  // allow variable file size (contents should be set only in zest mode)
  if (contents) {
    klee_assume(0 <= s->st_size && s->st_size <= size);
  } else {
    s->st_size = dfile->size;
  }
  s->st_blocks = 8;
  dfile->stat = s;

  dfile->src = NULL;
  if (is_foreign) {
    struct sockaddr_storage *ss;

    klee_assume((dfile->stat->st_mode & S_IFMT) == S_IFSOCK);

    for (sp=name; *sp; ++sp)
      src_name[sp-name] = *sp;
    strcpy(&src_name[sp-name], "-src");

    dfile->src = calloc(1, sizeof(*(dfile->src)));
    dfile->src->addr = ss = malloc(sizeof(*(dfile->src->addr)));
    klee_make_symbolic(dfile->src->addr, sizeof(*dfile->src->addr), src_name);
    /* Assume the port number is non-zero for PF_INET and PF_INET6. */
    /* Since the address family will be assigned later, we
       conservatively assume that the port number is non-zero for
       every address family supported. */
    klee_assume(/* ss->ss_family != PF_INET  || */ ((struct sockaddr_in  *)ss)->sin_port  != 0);
    klee_assume(/* ss->ss_family != PF_INET6 || */ ((struct sockaddr_in6 *)ss)->sin6_port != 0);
  }
}

static unsigned __sym_uint32(const char *name) {
  unsigned x;
  klee_make_symbolic(&x, sizeof x, name);
  return x;
}

#define CP_FILES 10

/* n_files: number of symbolic input files, excluding stdin
   file_length: size in bytes of each symbolic file, including stdin
   sym_stdout_flag: 1 if stdout should be symbolic, 0 otherwise
   save_all_writes_flag: 1 if all writes are executed as expected, 0 if 
                         writes past the initial file size are discarded 
			 (file offset is always incremented)
   max_failures: maximum number of system call failures */
void klee_init_fds(unsigned n_files, unsigned file_length, 
		   int sym_stdout_flag, int save_all_writes_flag,
                   unsigned n_streams, unsigned stream_len,
                   unsigned n_dgrams, unsigned dgram_len,
		   unsigned max_failures,
                   int one_line_streams,
                   const fill_info_t stream_fill_info[], unsigned n_stream_fill_info,
                   const fill_info_t dgram_fill_info[], unsigned n_dgram_fill_info) {
  unsigned k;
  char name[7] = "?-data";
  char sname[] = "STREAM?";
  char dname[] = "DGRAM?";

  struct stat64 s;

  stat64(".", &s);

  __one_line_streams = one_line_streams;
  __stream_fill_info = stream_fill_info;
  __n_stream_fill_info = n_stream_fill_info;
  __dgram_fill_info = dgram_fill_info;
  __n_dgram_fill_info = n_dgram_fill_info;


  __exe_fs.n_sym_files = n_files;
  __exe_fs.sym_files = malloc(sizeof(*__exe_fs.sym_files) * n_files);
  __exe_fs.n_sym_files_used = 0;

  for (k=0; k < n_files; k++) {
    name[0] = 'A' + k;
    __create_new_dfile(&__exe_fs.sym_files[k], file_length, NULL, name, NULL, 0, &s, 0);
  }
  __exe_fs.n_cp_files = CP_FILES;
  __exe_fs.cp_files = malloc(sizeof(*__exe_fs.cp_files) * CP_FILES);
  memset(__exe_fs.cp_files, 0, sizeof(*__exe_fs.cp_files) * CP_FILES);

  /* setting symbolic stdin */
  if (file_length) {
    __exe_fs.sym_stdin = malloc(sizeof(*__exe_fs.sym_stdin));
    __create_new_dfile(__exe_fs.sym_stdin, file_length, NULL, "stdin", NULL, 0, &s, 0);
    __exe_env.fds[0].dfile = __exe_fs.sym_stdin;
  }
  else __exe_fs.sym_stdin = NULL;

  __exe_fs.max_failures = max_failures;
  if (__exe_fs.max_failures) {
    __exe_fs.read_fail = malloc(sizeof(*__exe_fs.read_fail));
    __exe_fs.write_fail = malloc(sizeof(*__exe_fs.write_fail));
    __exe_fs.close_fail = malloc(sizeof(*__exe_fs.close_fail));
    __exe_fs.ftruncate_fail = malloc(sizeof(*__exe_fs.ftruncate_fail));
    __exe_fs.getcwd_fail = malloc(sizeof(*__exe_fs.getcwd_fail));

    klee_make_symbolic(__exe_fs.read_fail, sizeof(*__exe_fs.read_fail), "read_fail");
    klee_make_symbolic(__exe_fs.write_fail, sizeof(*__exe_fs.write_fail), "write_fail");
    klee_make_symbolic(__exe_fs.close_fail, sizeof(*__exe_fs.close_fail), "close_fail");
    klee_make_symbolic(__exe_fs.ftruncate_fail, sizeof(*__exe_fs.ftruncate_fail), "ftruncate_fail");
    klee_make_symbolic(__exe_fs.getcwd_fail, sizeof(*__exe_fs.getcwd_fail), "getcwd_fail");
  }

  /* setting symbolic stdout */
  if (sym_stdout_flag) {
    __exe_fs.sym_stdout = malloc(sizeof(*__exe_fs.sym_stdout));
    __create_new_dfile(__exe_fs.sym_stdout, 1024, NULL, "stdout", NULL, 0, &s, 0);
    __exe_env.fds[1].dfile = __exe_fs.sym_stdout;
    __exe_fs.stdout_writes = 0;
  }
  else __exe_fs.sym_stdout = NULL;
  
  __exe_env.save_all_writes = save_all_writes_flag;
  __exe_env.version = __sym_uint32("model_version");
  klee_assume(__exe_env.version == 1);

  /** Streams **/
  __exe_fs.n_sym_streams = n_streams;
  __exe_fs.sym_streams = malloc(sizeof(*__exe_fs.sym_streams) * n_streams);
  if (!__exe_fs.sym_streams)
    klee_warning("malloc returned NULL for sym_streams");
  for (k=0; k < n_streams; k++) {
    sname[strlen(sname)-1] = '1' + k;
    __create_new_dfile(&__exe_fs.sym_streams[k], stream_len, NULL, sname, __stream_fill_info, __n_stream_fill_info, &s, 1);
  }
  __exe_fs.n_sym_streams_used = 0;


  /** Datagrams **/
  __exe_fs.n_sym_dgrams = n_dgrams;
  __exe_fs.sym_dgrams = malloc(sizeof(*__exe_fs.sym_dgrams) * n_dgrams);
  if (!__exe_fs.sym_dgrams)
    klee_warning("malloc returned NULL for sym_dgrams");
  for (k=0; k < n_dgrams; k++) {
    dname[strlen(dname)-1] = '1' + k;
    __create_new_dfile(&__exe_fs.sym_dgrams[k], dgram_len, NULL, dname, __dgram_fill_info, __n_dgram_fill_info, &s, 1);
  }
  __exe_fs.n_sym_dgrams_used = 0;

  /* patch __exe_env file offsets. otherwise we get unexpected behavior with shared files (e.g., klee ls.bc >> out) */
  unsigned i;
  off_t offset;
  for (i = 0; i < 3; ++i) {
    offset = syscall(__NR_lseek, i, (off_t)0, SEEK_CUR);
    if (-1 != offset)
      __exe_env.fds[i].off = offset;
  }
}

int native_read_file(const char* path, int flags, char** _buf) {
  struct stat64 s;
  char *buf;
  int fd;
  if (-1 == stat64(path, &s))
    return -1;
  buf = *_buf = malloc(s.st_size);
  fd = syscall(__NR_open, path, flags, 0);
  if (-1 == fd)
    return -1;
  int rd, cnt = 0;
  while(cnt < s.st_size) {
    rd = syscall(__NR_read, fd, buf+cnt, s.st_size);
    if (rd <= 0)
      break;
    cnt += rd;
  }
  syscall(__NR_close, fd);
  return s.st_size;
}

exe_disk_file_t* klee_init_cp_file(const char* path, int flags) {
  unsigned k;
  int fsize;
  struct stat64 def;
  char *buf;

  fsize = native_read_file(path, flags, &buf);
  if (fsize < 0)
    return NULL;
  stat64(".", &def);
  for (k=0; k < __exe_fs.n_cp_files; k++) {
    if (!__exe_fs.cp_files[k].stat) {
      __create_new_dfile(&__exe_fs.cp_files[k], fsize, buf, path, NULL, 0, &def, 0);
      
      __exe_fs.cp_files[k].path = (char*)malloc(strlen(path)+1);
      strcpy(__exe_fs.cp_files[k].path, path);
      return &__exe_fs.cp_files[k];
    }
  }
  return NULL;
}

