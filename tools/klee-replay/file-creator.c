//===-- file-creator.c ----------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "klee-replay.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <pty.h>
#include <time.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <assert.h>

static void create_file(int target_fd, 
                       const char *target_name, 
                       exe_disk_file_t *dfile,
                       const char *tmpdir);
static void check_file(int index, exe_disk_file_t *file);
static void delete_file(const char *path, int recurse);


#define __STDIN -1
#define __STDOUT -2

static int create_link(const char *fname, 
                       exe_disk_file_t *dfile, 
                       const char *tmpdir) {
  char buf[64];
  struct stat64 *s = dfile->stat;

  // XXX Broken, we want this path to be somewhere else most likely.
  sprintf(buf, "%s.lnk", fname);
  s->st_mode = (s->st_mode & ~S_IFMT) | S_IFREG;
  create_file(-1, buf, dfile, tmpdir);
  
  int res = symlink(buf, fname);
  if (res < 0) {
    perror("symlink");
  }
  
  return open(fname, O_RDWR);
}


static int create_dir(const char *fname, exe_disk_file_t *dfile, 
                      const char *tmpdir) {
  int res = mkdir(fname, dfile->stat->st_mode);
  if (res < 0) {
    perror("mkdir");
    return -1;
  }
  return open(fname, O_RDWR);
}

double getTime() {
  struct timeval t;
  gettimeofday(&t, NULL);
  
  return (double) t.tv_sec + ((double) t.tv_usec / 1000000.0);
}

/// Return true if program exited, false if timed out.
int wait_for_timeout_or_exit(pid_t pid, const char *name, int *statusp) {
  char *t = getenv("KLEE_REPLAY_TIMEOUT");
  int timeout = t ? atoi(t) : 5;
  double wait = timeout * .5;
  double start = getTime();
  fprintf(stderr, "note: %s: waiting %.2fs\n", name, wait);
  while (getTime() - start < wait) {
    struct timespec r = {0, 1000000};
    nanosleep(&r, 0);
    int res = waitpid(pid, statusp, WNOHANG);
    if (res==pid)
      return 1;
  }
  
  return 0;
}

static int create_char_dev(const char *fname, exe_disk_file_t *dfile,
                           const char *tmpdir) {
  struct stat64 *s = dfile->stat;
  unsigned flen = dfile->size;
  char* contents = dfile->contents;

  // Assume tty, kinda broken, need an actual device id or something
  struct termios term, *ts=&term;
  struct winsize win = { 24, 80, 0, 0 };
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
  
  {
    char name[1024];
    int amaster, aslave;
    int res = openpty(&amaster, &aslave, name, &term, &win);
    if (res < 0) {
      perror("openpty");
      exit(1);
    }
    
    if (symlink(name, fname) == -1) {
      fprintf(stderr, "unable to create sym link to tty\n");
      perror("symlink");
    }
    
    // pty will not be world writeable
    s->st_mode &= ~02; 
    
    pid_t pid = fork();
    if (pid < 0) {
      perror("fork failed\n");
      exit(1);
    } else if (pid == 0) {
      close(amaster);

      fprintf(stderr, "note: pty slave: setting raw mode\n");
      {
        struct termio mode;
        
        int res = ioctl(aslave, TCGETA, &mode);
        assert(!res);
        mode.c_iflag = IGNBRK;
        mode.c_oflag &= ~(OLCUC | ONLCR | OCRNL | ONLRET);
        mode.c_lflag = 0;
        mode.c_cc[VMIN] = 1;
        mode.c_cc[VTIME] = 0;
        res = ioctl(aslave, TCSETA, &mode);
        assert(res == 0);
      }

      return aslave;
    } else {
      unsigned pos = 0;
      int status;
      fprintf(stderr, "note: pty master: starting\n");
      close(aslave);
      
      while (pos < flen) {
	int res = write(amaster, &contents[pos], flen - pos);
	if (res<0) {
	  if (errno != EINTR) {
	    fprintf(stderr, "note: pty master: write error\n");
	    perror("errno");
	    break;
	  }
	} else if (res) {
	  fprintf(stderr, "note: pty master: wrote: %d (of %d)\n", res, flen);
	  pos += res;
	}
      }

      if (wait_for_timeout_or_exit(pid, "pty master", &status))
        goto pty_exit;
      
      fprintf(stderr, "note: pty master: closing & waiting\n");
      close(amaster);
      while (1) {
	int res = waitpid(pid, &status, 0);
	if (res < 0) {
	  if (errno != EINTR)
	    break;
	} else {
	  break;
	}
      }
      
    pty_exit:
      close(amaster);
      fprintf(stderr, "note: pty master: done\n");
      process_status(status, 0, "PTY MASTER");
    }
  }
}

static int create_pipe(const char *fname, exe_disk_file_t *dfile,
                       const char *tmpdir) {
  //struct stat64 *s = dfile->stat;
  unsigned flen = dfile->size;
  char* contents = dfile->contents;

  // XXX what is direction ? need more data
  pid_t pid;
  int fds[2];
  int res = pipe(fds);
  if (res < 0) {
    perror("pipe");
    exit(1);
  }
  
  pid  = fork();
  if (pid < 0) {
    perror("fork");
    exit(1);     
  } else if (pid == 0) {
    close(fds[1]);
    return fds[0];
  } else {
    unsigned pos = 0;
    int status;
    fprintf(stderr, "note: pipe master: starting\n");
    close(fds[0]);
    
    while (pos < flen) {
      int res = write(fds[1], &contents[pos], flen - pos);
      if (res<0) {
	if (errno != EINTR)
	  break;
      } else if (res) {
	pos += res;
      }
    }

    if (wait_for_timeout_or_exit(pid, "pipe master", &status))
      goto pipe_exit;
    
    fprintf(stderr, "note: pipe master: closing & waiting\n");
    close(fds[1]);    
    while (1) {
      int res = waitpid(pid, &status, 0);
      if (res < 0) {
	if (errno != EINTR)
	  break;
      } else {
	break;
      }
    }
    
  pipe_exit:
    close(fds[1]);
    fprintf(stderr, "note: pipe master: done\n");
    process_status(status, 0, "PTY MASTER");
  }
}


static int create_reg_file(const char *fname, exe_disk_file_t *dfile,
                           const char *tmpdir) {    
  struct stat64 *s = dfile->stat;
  char* contents = dfile->contents;
  unsigned flen = dfile->size;
  unsigned mode = s->st_mode & 0777;

  //fprintf(stderr, "Creating regular file\n");
   
  // Open in RDWR just in case we have to end up using this fd.

  if (__exe_env.version == 0 && mode == 0)
    mode = 0644;
  
  
  int fd = open(fname, O_CREAT | O_RDWR, mode);
  //    int fd = open(fname, O_CREAT | O_WRONLY, s->st_mode&0777);
  if (fd < 0) {
    fprintf(stderr, "Cannot create file %s\n", fname);
    exit(1);
  }
  
  int r = write(fd, contents, flen);
  if (r < 0 || (unsigned) r != flen) {
    fprintf(stderr, "Cannot write file %s\n", fname);
    exit(1);
  }
  
  struct timeval tv[2];
  tv[0].tv_sec = s->st_atime;
  tv[0].tv_usec = 0;
  tv[1].tv_sec = s->st_mtime;
  tv[1].tv_usec = 0;
  futimes(fd, tv);
  
  // XXX: Now what we should do is reopen a new fd with the correct modes
  // as they were given to the process.
  lseek(fd, 0, SEEK_SET);
  
  return fd;
}

static int delete_dir(const char *path, int recurse) {
  if (recurse) {
    DIR *d = opendir(path);
    struct dirent *de;

    if (d) {
      while ((de = readdir(d))) {
        if (strcmp(de->d_name, ".")!=0 && strcmp(de->d_name, "..")!=0) {
          char tmp[PATH_MAX];
          sprintf(tmp, "%s/%s", path, de->d_name);
          delete_file(tmp, 0);
        }
      }

      closedir(d);
    }
  }
 
  if (rmdir(path) == -1) {
    fprintf(stderr, "Cannot create file %s (exists, is dir, can't remove)\n", path);
    perror("rmdir");
    return -1;
  }

  return 0;
}

static void delete_file(const char *path, int recurse) {
  if (unlink(path) < 0 && errno != ENOENT) {
    if (errno == EISDIR) {
      delete_dir(path, 1);
    } else {
      fprintf(stderr, "Cannot create file %s (already exists)\n", path);
      perror("unlink");
    }
  }
}

static void create_file(int target_fd,
                        const char *target_name, 
                        exe_disk_file_t *dfile,
                        const char *tmpdir) {
  struct stat64 *s = dfile->stat;
  char tmpname[PATH_MAX];
  const char *target;
  int fd;

  assert((target_fd == -1) ^ (target_name == NULL));

  if (target_name) {
    target = target_name;
  } else {
    sprintf(tmpname, "%s/fd%d", tmpdir, target_fd);
    target = tmpname;
  }

  delete_file(target, 1);

  // XXX get rid of me once a reasonable solution is found
  s->st_uid = geteuid();
  s->st_gid = getegid();

  if (S_ISLNK(s->st_mode)) {
    fd = create_link(target, dfile, tmpdir);
  } 
  else if (S_ISDIR(s->st_mode)) {
    fd = create_dir(target, dfile, tmpdir);
  } 
  else if (S_ISCHR(s->st_mode)) {
    fd = create_char_dev(target, dfile, tmpdir);
  } 
  else if (S_ISFIFO(s->st_mode) ||
           (target_fd==0 && (s->st_mode & S_IFMT) == 0)) { // XXX hack
    fd = create_pipe(target, dfile, tmpdir);
  }
  else {
    fd = create_reg_file(target, dfile, tmpdir);
  }

  if (fd >= 0) {
    if (target_fd != -1) {
      close(target_fd);
      if (dup2(fd, target_fd) < 0) {
        fprintf(stderr, "note: dup2 failed for target: %d\n", target_fd);
        perror("dup2");
      }
      close(fd);
    } else {
      // Only worry about 1 vs !1
      if (s->st_nlink > 1) {
        char tmp2[PATH_MAX];
        sprintf(tmp2, "%s/%s.link2", tmpdir, target_name);
        if (link(target_name, tmp2) < 0) {
          perror("link");
          exit(1);
        }
      }

      close(fd);
    }
  }
}

void replay_create_files(exe_file_system_t *exe_fs) {
  char tmpdir[PATH_MAX];
  unsigned k;

  if (!getcwd(tmpdir, PATH_MAX)) {
    perror("getcwd");
    exit(1);
  }

  strcat(tmpdir, ".temps");
  delete_file(tmpdir, 1);
  mkdir(tmpdir, 0755);  
  
  umask(0);
  for (k=0; k < exe_fs->n_sym_files; k++) {
    char name[2];
    sprintf(name, "%c", 'A' + k);
    create_file(-1, name, &exe_fs->sym_files[k], tmpdir);
  }

  if (exe_fs->sym_stdin)
    create_file(0, NULL, exe_fs->sym_stdin, tmpdir);

  if (exe_fs->sym_stdout)
    create_file(1, NULL, exe_fs->sym_stdout, tmpdir);

  if (exe_fs->sym_stdin)
    check_file(__STDIN, exe_fs->sym_stdin);

  if (exe_fs->sym_stdout)
    check_file(__STDOUT, exe_fs->sym_stdout);
  
  for (k=0; k<exe_fs->n_sym_files; ++k)
    check_file(k, &exe_fs->sym_files[k]);
}

static void check_file(int index, exe_disk_file_t *dfile) {
  struct stat s;
  int res;
  char name[32];

  switch (index) {
  case __STDIN:
    strcpy(name, "stdin"); 
    res = fstat(0, &s);    
    break;
  case __STDOUT: 
    strcpy(name, "stdout");
    res = fstat(1, &s);
    break;
  default: 
    name[0] = 'A' + index; 
    name[1] = '\0'; 
    res = stat(name, &s);
    break;
  }
  
  if (res < 0) {
    fprintf(stderr, "warning: check_file %d: stat failure\n", index);
    return;
  }

  if (s.st_dev != dfile->stat->st_dev) {
    fprintf(stderr, "warning: check_file %s: dev mismatch: %d vs %d\n", 
            name, (int) s.st_dev, (int) dfile->stat->st_dev);    
  }
/*   if (s.st_ino != dfile->stat->st_ino) { */
/*     fprintf(stderr, "warning: check_file %s: ino mismatch: %d vs %d\n",  */
/*             name, (int) s.st_ino, (int) dfile->stat->st_ino);     */
/*   } */
  if (s.st_mode != dfile->stat->st_mode) {
    fprintf(stderr, "warning: check_file %s: mode mismatch: %#o vs %#o\n", 
            name, s.st_mode, dfile->stat->st_mode);    
  }
  if (s.st_nlink != dfile->stat->st_nlink) {
    fprintf(stderr, "warning: check_file %s: nlink mismatch: %d vs %d\n", 
            name, (int) s.st_nlink, (int) dfile->stat->st_nlink);    
  }
  if (s.st_uid != dfile->stat->st_uid) {
    fprintf(stderr, "warning: check_file %s: uid mismatch: %d vs %d\n", 
            name, s.st_uid, dfile->stat->st_uid);    
  }
  if (s.st_gid != dfile->stat->st_gid) {
    fprintf(stderr, "warning: check_file %s: gid mismatch: %d vs %d\n", 
            name, s.st_gid, dfile->stat->st_gid);    
  }
  if (s.st_rdev != dfile->stat->st_rdev) {
    fprintf(stderr, "warning: check_file %s: rdev mismatch: %d vs %d\n", 
            name, (int) s.st_rdev, (int) dfile->stat->st_rdev);    
  }
  if (s.st_size != dfile->stat->st_size) {
    fprintf(stderr, "warning: check_file %s: size mismatch: %d vs %d\n", 
            name, (int) s.st_size, (int) dfile->stat->st_size);    
  }
  if (s.st_blksize != dfile->stat->st_blksize) {
    fprintf(stderr, "warning: check_file %s: blksize mismatch: %d vs %d\n", 
            name, (int) s.st_blksize, (int) dfile->stat->st_blksize);    
  }
  if (s.st_blocks != dfile->stat->st_blocks) {
    fprintf(stderr, "warning: check_file %s: blocks mismatch: %d vs %d\n", 
            name, (int) s.st_blocks, (int) dfile->stat->st_blocks);    
  }
/*   if (s.st_atime != dfile->stat->st_atime) { */
/*     fprintf(stderr, "warning: check_file %s: atime mismatch: %d vs %d\n",  */
/*             name, (int) s.st_atime, (int) dfile->stat->st_atime); */
/*   } */
/*   if (s.st_mtime != dfile->stat->st_mtime) { */
/*     fprintf(stderr, "warning: check_file %s: mtime mismatch: %d vs %d\n",  */
/*             name, (int) s.st_mtime, (int) dfile->stat->st_mtime);     */
/*   } */
/*   if (s.st_ctime != dfile->stat->st_ctime) { */
/*     fprintf(stderr, "warning: check_file %s: ctime mismatch: %d vs %d\n",  */
/*             name, (int) s.st_ctime, (int) dfile->stat->st_ctime);     */
/*   } */
}
