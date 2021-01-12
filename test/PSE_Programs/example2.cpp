// RUN: %clangxx -I../../../include -g -std=c++11 -DMAX_ELEMENTS=4 -fno-exceptions -emit-llvm -c -o %t1.bc %s
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --libc=klee --max-forks=25 --write-no-tests --exit-on-error --optimize --disable-inlining --search=nurs:depth --use-cex-cache %t1.bc

#include <algorithm>
#include <klee/klee.h>
#include <stdio.h>
#include <string>
#include <vector>

template <class T>
std::string tostring(std::vector<T> &vec) {
  std::string str("");
  for (const auto &elems : vec) {
    str.append(std::to_string(elems));
    str.append(",");
  }
  str.pop_back();
  return str;
}

template <class T>
void make_pse_symbolic(void *addr, size_t bytes, const char *name, std::vector<T> dist) {
  klee_make_symbolic(addr, bytes, name);
  klee_assume(*(T *)addr >= *std::min_element(dist.begin(), dist.end()));
  klee_assume(*(T *)addr <= *std::max_element(dist.begin(), dist.end()));
}

int main(void) {
  int c, a, b, d;

  klee_make_symbolic(&a, sizeof(a), "a_sym");
  klee_make_symbolic(&b, sizeof(b), "b_sym");
  klee_make_symbolic(&c, sizeof(c), "c_sym");

  if (a > 0 && c > 0 && c < d && d > a && d > b) {
    c = a * b;
  } else {
    d = a + b;
  }

  if (c > d) {
    a = 0;
    b = 0;
  }

  return 0;
}
