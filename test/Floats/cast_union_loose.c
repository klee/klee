// It requires bitwuzla because the script currently runs with bitwuzla solver backend
// REQUIRES: bitwuzla
// RUN: %kleef --property-file=%S/coverage-error-call.prp --max-memory=7000000000 --max-cputime-soft=30 --32 %s

extern void abort(void);
extern void __assert_fail(const char *, const char *, unsigned int, const char *) __attribute__((__nothrow__, __leaf__)) __attribute__((__noreturn__));
void reach_error() { __assert_fail("0", "cast_union_loose.c", 3, "reach_error"); }
/* Example from "Abstract Domains for Bit-Level Machine Integer and
   Floating-point Operations" by Miné, published in WING 12.
*/

extern int __VERIFIER_nondet_int(void);
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
  return;
}

union u {
  int i[2];
  double d;
};

double cast(int i) {
  union u x, y;
  x.i[0] = 0x43300000;
  y.i[0] = x.i[0];
  x.i[1] = 0x80000000;
  y.i[1] = i ^ x.i[1];
  return y.d - x.d;
}

int main() {
  int a;
  double r;

  a = __VERIFIER_nondet_int();
  assume_abort_if_not(a >= -10000 && a <= 10000);

  r = cast(a);
  __VERIFIER_assert(r >= -10000. && r <= 10000.);
  return 0;
}
