// REQUIRES: not-msan && not-asan
// RUN: %clang %s -emit-llvm -g -c -o %t.bc
// RUN: rm -rf %t.klee-out %t.log
// RUN: %klee -kdalloc -kdalloc-quarantine=-1 -output-dir=%t.klee-out %t.bc -exit-on-error 2>&1 | tee %t.log
// RUN: FileCheck %s -input-file=%t.log

// This test is disabled for asan and msan because they create additional page faults

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <sys/resource.h>

#include "klee/klee.h"

size_t maxrss() {
  struct rusage usage;
  int res = getrusage(RUSAGE_SELF, &usage);
  assert(!res && "getrusage succeeded");
  return usage.ru_maxrss;
}

int main(void) {
  size_t baseline = maxrss();
#if defined(__APPLE__)
  size_t limit = baseline + 100 * 1024 * 1024; // limit is 100 MiB above baseline
#else
  size_t limit = baseline + 100 * 1024; // limit is 100 MiB above baseline
#endif

  // CHECK: Deterministic allocator: Using unlimited quarantine

  size_t bins[] = {1, 4, 8, 16, 32, 64, 256, 2048};
  for (int i = 0; i < 1000; ++i) {
    for (size_t j = 0; j < sizeof(bins) / sizeof(*bins); ++j) {
      void *volatile p = malloc(bins[j]);
      void *volatile p2 = malloc(4096); // for faster growth

      // CHECK: calling external: getrusage
      // CHECK-NOT: ASSERTION FAIL
      assert(maxrss() < limit && "MaxRSS is below limit");

      free(p);
      free(p2);
    }
  }

  return 0;
}