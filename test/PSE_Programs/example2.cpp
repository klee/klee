// RUN: %clangxx -I../../../include -g -DMAX_ELEMENTS=4 -fno-exceptions -emit-llvm -c -o %t1.bc %s
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --libc=klee --max-forks=25 --write-no-tests --exit-on-error --optimize --disable-inlining --search=nurs:depth --use-cex-cache %t1.bc

#include <assert.h>
#include <klee/klee.h>
#include <random>
#include <stdio.h>

std::default_random_engine generator;
std::uniform_int_distribution<int> distribution(0, 10);

int main(void) {
  int a, b, c, t;

  float _distribution1[] = {1.0, 2.0, 3.0, 4.0, 5.0, 6.0};
  float _probabilities1[] = {1 / 10, 0.1, 0.2, 0.3, 0.1, 0.2};

  float _distribution2[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
  float _probabilities2[] = {0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1};

  klee_make_pse_symbolic(&a, sizeof(a), "a_pse_sym", _distribution1, _probabilities1); // PSE Variable
  klee_make_pse_symbolic(&b, sizeof(b), "b_pse_sym", _distribution2, _probabilities2); // PSE Variable
  klee_make_symbolic(&c, sizeof(c), "c_sym");                                          // ForAll Variable

  if ((a > b + c) && a >= 90) {
    t = a + b;
    a = b + c;
    b = a - c;
    klee_dump_symbolic_details(&t, "t");
    klee_dump_symbolic_details(&a, "a");
    klee_dump_symbolic_details(&b, "b");
    klee_dump_kquery_state();
  } else if (b > a + c) {
    a = b - c;
    b = a + c;
    klee_dump_symbolic_details(&b, "b");
    klee_dump_symbolic_details(&t, "t");
  } else {
    assert(1);
    klee_dump_kquery_state();
  }

  return 0;
}
