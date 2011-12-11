#include <assert.h>
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
#endif

// --sym-args 0 1 10 --sym-args 0 2 2 --sym-files 1 8 --sym-stdout
static int getint(char *i) {
  if(!i) {
    fprintf(stderr, "ran out of arguments!\n");
    assert(i);
  }
  return atoi(i);
}

#define MAX 64
static void push_obj(KTest *b, const char *name, unsigned non_zero_bytes, 
                     unsigned total_bytes) {
  KTestObject *o = &b->objects[b->numObjects++];
  assert(b->numObjects < MAX);

  o->name = strdup(name);
  o->numBytes = total_bytes;
  o->bytes = (unsigned char *)malloc(o->numBytes);

  unsigned i;
  for(i = 0; i < non_zero_bytes; i++)
    o->bytes[i] = random();

  for(i = non_zero_bytes; i < total_bytes; i++)
    o->bytes[i] = 0;
}


static void push_range(KTest *b, const char *name, unsigned value) {
  KTestObject *o = &b->objects[b->numObjects++];
  assert(b->numObjects < MAX);

  o->name = strdup(name);
  o->numBytes = 4;
  o->bytes = (unsigned char *)malloc(o->numBytes);

  *(unsigned*)o->bytes = value;
}

int main(int argc, char *argv[]) {
  unsigned i, narg;
  unsigned sym_stdout = 0;

  if (argc < 2) {
    fprintf(stderr, "Usage: %s <random-seed> <argument-types>\n", argv[0]);
    fprintf(stderr, "       If <random-seed> is 0, time(NULL)*getpid() is used as a seed\n");
    fprintf(stderr, "       <argument-types> are the ones accepted by KLEE: --sym-args, --sym-files etc.\n");
    fprintf(stderr, "   Ex: %s 100 --sym-args 0 2 2 --sym-files 1 8\n", argv[0]);
    exit(1);
  }

  unsigned seed = atoi(argv[1]);
  if (seed)
    srandom(seed);
  else srandom(time(NULL) * getpid());

  KTest b;
  b.numArgs = argc;
  b.args = argv;
  b.symArgvs = 0;
  b.symArgvLen = 0;

  b.numObjects = 0;
  b.objects = (KTestObject *)malloc(MAX * sizeof *b.objects);

  for(i = 2; i < (unsigned)argc; i++) {
    if(strcmp(argv[i], "--sym-args") == 0) {
      unsigned lb = getint(argv[++i]);
      unsigned ub = getint(argv[++i]);
      unsigned nbytes = getint(argv[++i]);

      narg = random() % (ub - lb) + lb;
      push_range(&b, "range", narg);

      while(narg-- > 0) {
        unsigned x = random() % (nbytes + 1);

        // A little different than how klee does it but more natural
        // for random.
        static int total_args;
        char arg[1024];

        sprintf(arg, "arg%d", total_args++);
        push_obj(&b, arg, x, nbytes+1);
      }
    } else if(strcmp(argv[i], "--sym-stdout") == 0) {
      if(!sym_stdout) {
        sym_stdout = 1;
        push_obj(&b, "stdout", 1024, 1024);
        push_obj(&b, "stdout-stat", sizeof(struct stat64), 
                 sizeof(struct stat64));
      }
    } else if(strcmp(argv[i], "--sym-files") == 0) {
      unsigned nfiles = getint(argv[++i]);
      unsigned nbytes = getint(argv[++i]);

      while(nfiles-- > 0) {
        push_obj(&b, "file", nbytes, nbytes);
        push_obj(&b, "file-stat", sizeof(struct stat64), sizeof(struct stat64));
      }

      push_obj(&b, "stdin", nbytes, nbytes);
      push_obj(&b, "stdin-stat", sizeof(struct stat64), sizeof(struct stat64));
    } else {
      fprintf(stderr, "unexpected option <%s>\n", argv[i]);
      assert(0);
    }
  }

  if (!kTest_toFile(&b, "file.bout"))
    assert(0);
  return 0;
}
