
// RUN: %clangxx -I../../../include -g -DMAX_ELEMENTS=4 -fno-exceptions -emit-llvm -c -o %t1.bc %s
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --libc=klee --max-forks=25 --write-no-tests --exit-on-error --optimize --disable-inlining --search=nurs:depth --use-cex-cache %t1.bc

#include <assert.h>
#include <klee/klee.h>
#include <random>
#include <stdio.h>

std::default_random_engine generator;
std::uniform_int_distribution<int> distribution(10, 150);

int weird_func(int a, int b, int c) {
  int t = 0;
  if (a > b + c) {
    t = a + b + c;
    klee_dump_kquery_state();
    klee_dump_symbolic_details(&a, "a");
    klee_dump_symbolic_details(&b, "b");
    klee_dump_symbolic_details(&t, "t");
    return a + b + c;
  } else {
    t = a - b - c;
    klee_dump_kquery_state();
    klee_dump_symbolic_details(&a, "a");
    klee_dump_symbolic_details(&b, "b");
    klee_dump_symbolic_details(&t, "t");
    return t;
  }
}

int main(void) {
  int a, b, c;

  float _distribution1[] = {1, 2, 3, 4, 5, 6};
  float _probabilities1[] = {0.1, 0.1, 0.2, 0.3, 0.1, 0.2};

  float _distribution2[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
  float _probabilities2[] = {0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1};

  klee_make_pse_symbolic(&a, sizeof(a), "a_pse_sym", _distribution1, _probabilities1);
  klee_make_pse_symbolic(&b, sizeof(b), "b_pse_sym", _distribution2, _probabilities2);

  a = _distribution1[2];       // PSE Variable
  b = _distribution2[3];       // PSE Variable
  c = distribution(generator); // ForAll Variable

  klee_make_symbolic(&c, sizeof(c), "c_sym");
  klee_dump_symbolic_details(&c, "c");
  klee_dump_symbolic_details(&a, "a");
  return weird_func(a, b, c);
}