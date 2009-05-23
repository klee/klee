//===-- intrinsics.c ------------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

/* Straight C for linking simplicity */

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <time.h>

#include "klee/klee.h"

#include "klee/Internal/ADT/KTest.h"

static KTest *testData = 0;
static unsigned testPosition = 0;

static unsigned char rand_byte(void) {
  unsigned x = rand();
  x ^= x>>16;
  x ^= x>>8;
  return x & 0xFF;
}

void klee_make_symbolic(void *array, unsigned nbytes, const char *name) {
  static int rand_init = -1;

  if (rand_init == -1) {
    if (getenv("KLEE_RANDOM")) {
      struct timeval tv;
      gettimeofday(&tv, 0);
      rand_init = 1;
      srand(tv.tv_sec ^ tv.tv_usec);
    } else {
      rand_init = 0;
    }
  }

  if (rand_init) {
    if (!strcmp(name,"syscall_a0")) {
      unsigned long long *v = array;
      assert(nbytes == 8);
      *v = rand() % 69;
    } else {
      char *c = array;
      unsigned i;
      for (i=0; i<nbytes; i++)
        c[i] = rand_byte();
    }
    return;
  }

  if (!testData) {
    char tmp[256];
    char *name = getenv("KTEST_FILE");

    if (!name) {
      fprintf(stdout, "KLEE-RUNTIME: KTEST_FILE not set, please enter .ktest path: ");
      fflush(stdout);
      name = tmp;
      if (!fgets(tmp, sizeof tmp, stdin) || !strlen(tmp)) {
        fprintf(stderr, "KLEE-RUNTIME: cannot replay, no KTEST_FILE or user input\n");
        exit(1);
      }
      tmp[strlen(tmp)-1] = '\0'; /* kill newline */
    }
    testData = kTest_fromFile(name);
    if (!testData) {
      fprintf(stderr, "KLEE-RUNTIME: unable to open .ktest file\n");
      exit(1);
    }
  }

  if (testPosition >= testData->numObjects) {
    fprintf(stderr, "ERROR: out of inputs, using zero\n");
    memset(array, 0, nbytes);
  } else {
    KTestObject *o = &testData->objects[testPosition++];
    memcpy(array, o->bytes, nbytes<o->numBytes ? nbytes : o->numBytes);
    if (nbytes != o->numBytes) {
      fprintf(stderr, "ERROR: object sizes differ\n");
      if (o->numBytes < nbytes) 
        memset((char*) array + o->numBytes, 0, nbytes - o->numBytes);
    }
  }
}

void *klee_malloc_n(unsigned nelems, unsigned size, unsigned alignment) {
#if 1
  return mmap((void*) 0x90000000, nelems*size, PROT_READ|PROT_WRITE, 
              MAP_PRIVATE
#ifdef MAP_ANONYMOUS
              |MAP_ANONYMOUS
#endif
              , 0, 0);
#else
  char *buffer = malloc(nelems*size + alignment - 1);
  buffer += (alignment - (long)buffer % alignment);
  return buffer;
#endif
}

void klee_silent_exit(int x) {
  exit(x);
}

unsigned klee_choose(unsigned n) {
  unsigned x;
  klee_make_symbolic(&x, sizeof x, "klee_choose");
  if(x >= n)
    fprintf(stderr, "ERROR: max = %d, got = %d\n", n, x);
  assert(x < n);
  return x;
}

void klee_assume(unsigned x) {
  if (!x) {
    fprintf(stderr, "ERROR: invalid klee_assume\n");
  }
}

unsigned klee_get_value(unsigned x) {
  return x;
}

int klee_range(int begin, int end, const char* name) {
  int x;
  klee_make_symbolic(&x, sizeof x, name);
  if (x<begin || x>=end) {
    fprintf(stderr, 
            "KLEE: ERROR: invalid klee_range(%u,%u,%s) value, got: %u\n", 
            begin, end, name, x);
    abort();
  }
  return x;
}

/* not sure we should even define.  is for debugging. */
void klee_print_expr(const char *msg, ...) { }

void klee_set_forking(unsigned enable) { }
