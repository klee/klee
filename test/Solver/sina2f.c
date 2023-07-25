// REQUIRES: bitwuzla
// REQUIRES: not-asan
// REQUIRES: not-msan
// RUN: %clang %s -emit-llvm %O0opt -g -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --strip-unwanted-calls --delete-dead-loops=false --emit-all-errors --mock-policy=all --use-forked-solver=false --solver-backend=bitwuzla-tree --max-solvers-approx-tree-inc=16 --max-memory=6008 --optimize --skip-not-lazy-initialized --output-source=false --output-stats=false --output-istats=false --use-sym-size-alloc=true --cex-cache-validity-cores --fp-runtime --x86FP-as-x87FP80 --symbolic-allocation-threshold=8192 --allocate-determ --allocate-determ-size=3072 --allocate-determ-start-address=0x00030000000 --mem-trigger-cof --use-alpha-equivalence=true --track-coverage=all --use-iterative-deepening-search=max-cycles --max-solver-time=5s --max-cycles-before-stuck=15 --only-output-states-covering-new --dump-states-on-halt=all --search=dfs --search=random-state --cover-on-the-fly=true --delay-cover-on-the-fly=27 --max-time=29 %t1.bc 2>&1 | FileCheck %s
#include "klee-test-comp.c"

/*
 * Benchmarks contributed by Divyesh Unadkat[1,2], Supratik Chakraborty[1], Ashutosh Gupta[1]
 * [1] Indian Institute of Technology Bombay, Mumbai
 * [2] TCS Innovation labs, Pune
 *
 */

extern void abort(void);
extern void __assert_fail(const char *, const char *, unsigned int, const char *) __attribute__((__nothrow__, __leaf__)) __attribute__((__noreturn__));
void reach_error() { __assert_fail("0", "sina2f.c", 10, "reach_error"); }
extern void abort(void);
void assume_abort_if_not(int cond) {
  if (!cond) {
    abort();
  }
}
void __VERIFIER_assert(int cond) {
  if (!(cond)) {
  ERROR : {
    reach_error();
    abort();
  }
  }
}
extern int __VERIFIER_nondet_int(void);
void *malloc(unsigned int size);

int N;

int main() {
  N = __VERIFIER_nondet_int();
  if (N <= 0)
    return 1;
  assume_abort_if_not(N <= 2147483647 / sizeof(int));

  int i;
  long long sum[1];
  long long *a = malloc(sizeof(long long) * N);

  sum[0] = 1;
  for (i = 0; i < N; i++) {
    a[i] = 1;
  }

  for (i = 0; i < N; i++) {
    sum[0] = sum[0] + a[i];
  }

  for (i = 0; i < N; i++) {
    a[i] = a[i] + sum[0];
  }

  for (i = 0; i < N; i++) {
    __VERIFIER_assert(a[i] == N);
  }
  return 1;
}

// CHECK: KLEE: done
