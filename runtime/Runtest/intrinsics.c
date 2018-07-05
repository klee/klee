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
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
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

static void report_internal_error(const char *msg, ...)
    __attribute__((format(printf, 1, 2)));
static void report_internal_error(const char *msg, ...) {
  fprintf(stderr, "KLEE_RUN_TEST_ERROR: ");
  va_list ap;
  va_start(ap, msg);
  vfprintf(stderr, msg, ap);
  va_end(ap);
  fprintf(stderr, "\n");
  char *testErrorsNonFatal = getenv("KLEE_RUN_TEST_ERRORS_NON_FATAL");
  if (testErrorsNonFatal) {
    fprintf(stderr, "KLEE_RUN_TEST_ERROR: Forcing execution to continue\n");
  } else {
    exit(1);
  }
}

void klee_make_symbolic(void *array, size_t nbytes, const char *name) {

  if (!name)
    name = "unnamed";

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
      size_t i;
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

  for (;; ++testPosition) {
    if (testPosition >= testData->numObjects) {
      report_internal_error("out of inputs. Will use zero if continuing.");
      memset(array, 0, nbytes);
      break;
    } else {
      KTestObject *o = &testData->objects[testPosition];
      if (strcmp("model_version", o->name) == 0 &&
          strcmp("model_version", name) != 0) {
        // Skip over this KTestObject because we've hit
        // `model_version` which is from the POSIX runtime
        // and the caller didn't ask for it.
        continue;
      }
      if (strcmp(name, o->name) != 0) {
        report_internal_error(
            "object name mismatch. Requesting \"%s\" but returning \"%s\"",
            name, o->name);
      }
      memcpy(array, o->bytes, nbytes < o->numBytes ? nbytes : o->numBytes);
      if (nbytes != o->numBytes) {
        report_internal_error("object sizes differ. Expected %zu but got %u",
                              nbytes, o->numBytes);
        if (o->numBytes < nbytes)
          memset((char *)array + o->numBytes, 0, nbytes - o->numBytes);
      }
      ++testPosition;
      break;
    }
  }
}

void klee_silent_exit(int x) {
  exit(x);
}

uintptr_t klee_choose(uintptr_t n) {
  uintptr_t x;
  klee_make_symbolic(&x, sizeof x, "klee_choose");
  if(x >= n)
    report_internal_error("klee_choose failure. max = %ld, got = %ld\n", n, x);
  return x;
}

void klee_assume(uintptr_t x) {
  if (!x) {
    report_internal_error("invalid klee_assume");
  }
}

#define KLEE_GET_VALUE_STUB(suffix, type)	\
	type klee_get_value##suffix(type x) { \
		return x; \
	}

KLEE_GET_VALUE_STUB(f, float)
KLEE_GET_VALUE_STUB(d, double)
KLEE_GET_VALUE_STUB(l, long)
KLEE_GET_VALUE_STUB(ll, long long)
KLEE_GET_VALUE_STUB(_i32, int32_t)
KLEE_GET_VALUE_STUB(_i64, int64_t)

#undef KLEE_GET_VALUE_STUB

int klee_range(int begin, int end, const char* name) {
  int x;
  klee_make_symbolic(&x, sizeof x, name);
  if (x<begin || x>=end) {
    report_internal_error("invalid klee_range(%u,%u,%s) value, got: %u\n",
                          begin, end, name, x);
  }
  return x;
}

void klee_prefer_cex(void *object, uintptr_t condition) { }

void klee_abort() {
  abort();
}

/* not sure we should even define.  is for debugging. */
void klee_print_expr(const char *msg, ...) { }

void klee_set_forking(unsigned enable) { }
