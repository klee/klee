//===-- gen-random-bout.cpp -------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include <assert.h>
#include <fcntl.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "klee/Internal/ADT/KTest.h"

#if defined(__FreeBSD__) || defined(__minix)
#define stat64 stat
#define fstat64 fstat
#endif

#define SMALL_BUFFER_SIZE 64 // To hold "arg<N>" string, temporary
                             // filename, etc.

#define MAX_FILE_SIZES 256

static void error_exit(const char *fmt, ...) {
  va_list args;

  va_start(args, fmt);
  vfprintf(stderr, fmt, args);
  va_end(args);

  exit(1);
}

static unsigned get_unsigned(char *i) {
  if (!i) {
    error_exit("ran out of arguments!\n");
  }
  long int n = strtol(i, NULL, 10);
  if (n < 0 || n == LONG_MIN || n == LONG_MAX) {
    error_exit("%s:%d: Error in conversion to unsigned: %s\n", __FILE__,
               __LINE__, i);
  }
  return (unsigned)n;
}

#define MAX 64
static void push_random_obj(KTest *b, const char *name, unsigned non_zero_bytes,
                            unsigned total_bytes) {
  KTestObject *o = &b->objects[b->numObjects++];
  assert(b->numObjects < MAX);

  if ((o->name = strdup(name)) == NULL) {
    error_exit("%s:%d: strdup() failure\n", __FILE__, __LINE__);
  }
  o->numBytes = total_bytes;
  if ((o->bytes = (unsigned char *)malloc(o->numBytes)) == NULL) {
    error_exit("%s:%d: malloc() failure\n", __FILE__, __LINE__);
  }

  unsigned i;
  for (i = 0; i < non_zero_bytes; i++) {
    o->bytes[i] = random() % 255 + 1;
  }

  for (i = non_zero_bytes; i < total_bytes; i++)
    o->bytes[i] = 0;
}

static void push_obj(KTest *b, const char *name, unsigned total_bytes,
                     unsigned char *content) {
  KTestObject *o = &b->objects[b->numObjects++];
  assert(b->numObjects < MAX);

  if ((o->name = strdup(name)) == NULL) {
    error_exit("%s:%d: strdup() failure\n", __FILE__, __LINE__);
  }
  o->numBytes = total_bytes;
  if ((o->bytes = (unsigned char *)malloc(total_bytes)) == NULL) {
    error_exit("%s:%d: malloc() failure\n", __FILE__, __LINE__);
  }
  memcpy(o->bytes, content, total_bytes);
}

static void push_range(KTest *b, const char *name, unsigned value) {
  push_obj(b, name, 4, (unsigned char *)&value);
}

void create_stat(size_t size, struct stat64 *s) {
  char filename_template[] = "/tmp/klee-gen-random-bout-XXXXXX";
  char *filename;
  int fd;
  unsigned char *buf;

  if ((filename = strdup(filename_template)) == NULL) {
    error_exit("%s:%d: strdup() failure\n", __FILE__, __LINE__);
  }

  if (((fd = mkstemp(filename)) < 0) ||
      ((fchmod(fd, S_IRWXU | S_IRWXG | S_IRWXO)) < 0)) {
    error_exit("%s:%d: Failure creating %s\n", __FILE__, __LINE__, filename);
  }

  if ((buf = (unsigned char *)malloc(size)) == NULL) {
    error_exit("%s:%d: malloc() failure\n", __FILE__, __LINE__);
  }

  if (write(fd, memset(buf, 0, size), size) != (int)size) {
    free(buf);
    free(filename);
    error_exit("%s:%d: Error writing %s\n", __FILE__, __LINE__, filename);
  }

  fstat64(fd, s);

  close(fd);

  unlink(filename);

  free(filename);
  free(buf);
}

int main(int argc, char *argv[]) {
  unsigned i, narg;
  unsigned sym_stdout = 0;
  unsigned stdin_size = 0;
  int total_args = 0;
  unsigned total_files = 0;
  unsigned file_sizes[MAX_FILE_SIZES];
  char **argv_copy;
  char *bout_file = NULL;

  if (argc < 2) {
    error_exit(
        "Usage: %s <random-seed> <argument-types>\n"
        "       If <random-seed> is 0, time(NULL)*getpid() is used as a seed\n"
        "       <argument-types> are the ones accepted by KLEE: --sym-args, "
        "--sym-files etc. and --bout-file <filename> for the output file (default: random.bout).\n"
        "   Ex: %s 100 --sym-args 0 2 2 --sym-files 1 8\n",
        argv[0], argv[0]);
  }

  unsigned seed = atoi(argv[1]);
  if (seed)
    srandom(seed);
  else
    srandom(time(NULL) * getpid());

  if ((argv_copy = (char **) malloc((argc - 1) * sizeof(char *))) == NULL) {
    error_exit("%s:%d: malloc() failure\n", __FILE__, __LINE__);
  }
  argv_copy[0] = argv[0];
  for (i = 2; i < (unsigned)argc; ++i) {
    argv_copy[i - 1] = argv[i];
  }

  KTest b;
  b.numArgs = argc - 1;
  b.args = argv_copy;
  b.symArgvs = 0;
  b.symArgvLen = 0;

  b.numObjects = 0;
  b.objects = (KTestObject *)malloc(MAX * sizeof *b.objects);

  for (i = 2; i < (unsigned)argc; i++) {
    if (strcmp(argv[i], "--sym-arg") == 0 || strcmp(argv[i], "-sym-arg") == 0) {
      unsigned nbytes = get_unsigned(argv[++i]);

      // A little different than how klee does it but more natural for random.
      char arg[SMALL_BUFFER_SIZE];
      unsigned x = random() % (nbytes + 1);

      snprintf(arg, SMALL_BUFFER_SIZE, "arg%d", total_args++);
      push_random_obj(&b, arg, x, nbytes + 1);
    } else if (strcmp(argv[i], "--sym-args") == 0 ||
               strcmp(argv[i], "-sym-args") == 0) {
      unsigned lb = get_unsigned(argv[++i]);
      unsigned ub = get_unsigned(argv[++i]);
      unsigned nbytes = get_unsigned(argv[++i]);

      if (ub < lb) {
        error_exit("--sym-args first argument should be no more than its "
                   "second argument\n");
      }

      narg = random() % (ub - lb + 1) + lb;
      push_range(&b, "n_args", narg);

      while (narg-- > 0) {
        unsigned x = random() % (nbytes + 1);

        // A little different than how klee does it but more natural
        // for random.
        char arg[SMALL_BUFFER_SIZE];

        snprintf(arg, SMALL_BUFFER_SIZE, "arg%d", total_args++);
        push_random_obj(&b, arg, x, nbytes + 1);
      }
    } else if (strcmp(argv[i], "--sym-stdout") == 0 ||
               strcmp(argv[i], "-sym-stdout") == 0) {
      sym_stdout = 1;
    } else if (strcmp(argv[i], "--sym-stdin") == 0 ||
               strcmp(argv[i], "-sym-stdin") == 0) {
      stdin_size = get_unsigned(argv[++i]);
    } else if (strcmp(argv[i], "--sym-files") == 0 ||
               strcmp(argv[i], "-sym-files") == 0) {
      unsigned nfiles = get_unsigned(argv[++i]);
      unsigned nbytes = get_unsigned(argv[++i]);

      total_files = 0;
      while (nfiles-- > 0) {
        if (total_files >= MAX_FILE_SIZES) {
          error_exit("%s:%d: Maximum number of file sizes exceeded (%d)\n",
                     __FILE__, __LINE__, MAX_FILE_SIZES);
        }
        file_sizes[total_files++] = nbytes;
      }

    } else if (strcmp(argv[i], "--bout-file") == 0 ||
               strcmp(argv[i], "-bout-file") == 0) {
      if ((unsigned)argc == ++i)
        error_exit("Missing file name for --bout-file");

      bout_file = argv[i];
    } else {
      error_exit("Unexpected option <%s>\n", argv[i]);
    }
  }

  for (i = 0; i < total_files; ++i) {
    char filename[] = "A-data";
    char file_stat[] = "A-data-stat";
    unsigned nbytes;
    struct stat64 s;

    if (i >= MAX_FILE_SIZES) {
      fprintf(stderr, "%s:%d: Maximum number of file sizes exceeded (%d)\n",
              __FILE__, __LINE__, MAX_FILE_SIZES);
      exit(1);
    }
    nbytes = file_sizes[i];

    filename[0] += i;
    file_stat[0] += i;

    create_stat(nbytes, &s);

    push_random_obj(&b, filename, nbytes, nbytes);
    push_obj(&b, file_stat, sizeof(struct stat64), (unsigned char *)&s);
  }

  if (stdin_size) {
    struct stat64 s;

    // Using disk file works well with klee-replay.
    create_stat(stdin_size, &s);

    push_random_obj(&b, "stdin", stdin_size, stdin_size);
    push_obj(&b, "stdin-stat", sizeof(struct stat64), (unsigned char *)&s);
  }
  if (sym_stdout) {
    struct stat64 s;

    // Using disk file works well with klee-replay.
    create_stat(1024, &s);

    push_random_obj(&b, "stdout", 1024, 1024);
    push_obj(&b, "stdout-stat", sizeof(struct stat64), (unsigned char *)&s);
  }
  push_range(&b, "model_version", 1);

  if (!kTest_toFile(&b, bout_file ? bout_file : "random.bout")) {
    error_exit("Error in storing data into random.bout\n");
  }

  for (i = 0; i < b.numObjects; ++i) {
    free(b.objects[i].name);
    free(b.objects[i].bytes);
  }

  free(b.objects);

  free(argv_copy);
  return 0;
}

