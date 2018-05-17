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

static void __create_new_dfile(exe_disk_file_t *dfile, unsigned size, 
                               const char *name, struct stat64 *defaults, int is_foreign) {
  struct stat64 *s = malloc(sizeof(*s));
  const char *sp;
  char sname[256], src_name[256];
  for (sp=name; *sp; ++sp)
    sname[sp-name] = *sp;
  memcpy(&sname[sp-name], "-stat", 6);

  assert(size);

  dfile->size = size;
  dfile->contents = malloc(dfile->size);
  klee_make_symbolic(dfile->contents, dfile->size, name);
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
  klee_prefer_cex(s, (s->st_mode&0070) == 0040);
  klee_prefer_cex(s, (s->st_mode&0007) == 0004);
  klee_prefer_cex(s, (s->st_mode&S_IFMT) == S_IFREG);
  klee_prefer_cex(s, s->st_nlink == 1);
  klee_prefer_cex(s, s->st_uid == defaults->st_uid);
  klee_prefer_cex(s, s->st_gid == defaults->st_gid);
  klee_prefer_cex(s, s->st_blksize == 4096);
  klee_prefer_cex(s, s->st_atime == defaults->st_atime);
  klee_prefer_cex(s, s->st_mtime == defaults->st_mtime);
  klee_prefer_cex(s, s->st_ctime == defaults->st_ctime);

  s->st_size = dfile->size;
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

/* n_files: number of symbolic input files, excluding stdin
   file_length: size in bytes of each symbolic file, including stdin
   sym_stdout_flag: 1 if stdout should be symbolic, 0 otherwise
   save_all_writes_flag: 1 if all writes are executed as expected, 0 if 
                         writes past the initial file size are discarded 
			 (file offset is always incremented)
   max_failures: maximum number of system call failures */
void klee_init_fds(unsigned n_files, unsigned file_length, unsigned stdin_length,
		   int sym_stdout_flag, int save_all_writes_flag,
                   unsigned n_streams, unsigned stream_len,
                   unsigned n_dgrams, unsigned dgram_len,
		   unsigned max_failures) {
  unsigned k;
  char name[7] = "?-data";
  char sname[] = "STREAM?";
  char dname[] = "DGRAM?";

  struct stat64 s;

  stat64(".", &s);

  __exe_fs.n_sym_files = n_files;
  __exe_fs.sym_files = malloc(sizeof(*__exe_fs.sym_files) * n_files);
  __exe_fs.n_sym_files_used = 0;

  for (k=0; k < n_files; k++) {
    name[0] = 'A' + k;
    __create_new_dfile(&__exe_fs.sym_files[k], file_length, name, &s, 0);
  }
  
  /* setting symbolic stdin */
  if (stdin_length) {
    __exe_fs.sym_stdin = malloc(sizeof(*__exe_fs.sym_stdin));
    __create_new_dfile(__exe_fs.sym_stdin, stdin_length, "stdin", &s, 0);
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
    __create_new_dfile(__exe_fs.sym_stdout, 1024, "stdout", &s, 0);
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
    __create_new_dfile(&__exe_fs.sym_streams[k], stream_len, sname, &s, 1);
  }
  __exe_fs.n_sym_streams_used = 0;


  /** Datagrams **/
  __exe_fs.n_sym_dgrams = n_dgrams;
  __exe_fs.sym_dgrams = malloc(sizeof(*__exe_fs.sym_dgrams) * n_dgrams);
  if (!__exe_fs.sym_dgrams)
    klee_warning("malloc returned NULL for sym_dgrams");
  for (k=0; k < n_dgrams; k++) {
    dname[strlen(dname)-1] = '1' + k;
    __create_new_dfile(&__exe_fs.sym_dgrams[k], dgram_len, dname, &s, 1);
  }
  __exe_fs.n_sym_dgrams_used = 0;

  /* patch __exe_env file offsets. otherwise we get unexpected behavior with shared files (e.g., klee ls.bc >> out) */
  /*unsigned i;
  off_t offset;
  for (i = 0; i < 3; ++i) {
    offset = syscall(__NR_lseek, i, (off_t)0, SEEK_CUR);
    if (-1 != offset)
      __exe_env.fds[i].off = offset;
  } */
}
