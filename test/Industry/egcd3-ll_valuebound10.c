// RUN: %clang %s -emit-llvm %O0opt -g -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --delete-dead-loops=false --emit-all-errors --mock-all-externals --use-forked-solver=false --max-memory=6008 --optimize --skip-not-lazy-initialized --output-source=false --output-stats=true --output-istats=true -istats-write-interval=90s --use-sym-size-alloc=true --cex-cache-validity-cores --symbolic-allocation-threshold=8192 --max-solver-time=5s --track-coverage=branches --use-iterative-deepening-search=max-cycles --cover-on-the-fly=false --delay-cover-on-the-fly=400000 --only-output-states-covering-new --dump-states-on-halt=true --search=dfs --search=random-state %t1.bc
// RUN: %klee-stats --print-columns 'BCov(%)' --table-format=csv %t.klee-out > %t.stats
// RUN: FileCheck -input-file=%t.stats %s
// CHECK: BCov(%)
// CHECK-NEXT: {{(8[5-9]|9[0-9]|100)\.}}

#include "klee-test-comp.c"
/* extended Euclid's algorithm */
extern void abort(void);
extern void __assert_fail(const char *, const char *, unsigned int, const char *) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__noreturn__));
void reach_error() { __assert_fail("0", "egcd3-ll.c", 4, "reach_error"); }
extern int __VERIFIER_nondet_int(void);
extern void abort(void);
void assume_abort_if_not(int cond) {
  if(!cond) {abort();}
}
void __VERIFIER_assert(int cond) {
    if (!(cond)) {
    ERROR:
        {reach_error();}
    }
    return;
}

int main() {
    int x, y;
    long long a, b, p, q, r, s;
    x = __VERIFIER_nondet_int();
    assume_abort_if_not(x>=0 && x<=10);
    y = __VERIFIER_nondet_int();
    assume_abort_if_not(y>=0 && y<=10);
    assume_abort_if_not(x >= 1);
    assume_abort_if_not(y >= 1);

    a = x;
    b = y;
    p = 1;
    q = 0;
    r = 0;
    s = 1;

    while (1) {
        if (!(b != 0))
            break;
        long long c, k;
        c = a;
        k = 0;

        while (1) {
            if (!(c >= b))
                break;
            long long d, v;
            d = 1;
            v = b;

            while (1) {
                __VERIFIER_assert(a == y * r + x * p);
                __VERIFIER_assert(b == x * q + y * s);
                __VERIFIER_assert(a == k * b + c);
                __VERIFIER_assert(v == b * d);

                if (!(c >= 2 * v))
                    break;
                d = 2 * d;
                v = 2 * v;
            }
            c = c - v;
            k = k + d;
        }

        a = b;
        b = c;
        long long temp;
        temp = p;
        p = q;
        q = temp - q * k;
        temp = r;
        r = s;
        s = temp - s * k;
    }
    __VERIFIER_assert(p*x - q*x + r*y - s*y  == a);
    return 0;
}
