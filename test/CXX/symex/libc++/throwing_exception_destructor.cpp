// Testcase for proper handling of exception destructors that throw.
// REQUIRES: uclibc
// REQUIRES: libcxx
// REQUIRES: eh-cxx
// RUN: %clangxx %s -emit-llvm -O0 -std=c++11 -c %libcxx_includes -g -nostdinc++ -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --libcxx --libc=uclibc --exit-on-error  %t.bc

#include <cassert>
#include <cstdlib>
#include <exception>

bool destructor_throw = false;
bool test_catch = false;
bool int_catch = false;

struct Test {
  ~Test() noexcept(false) {
    destructor_throw = true;
    throw 5;
  }
};

int main() {
  // For some reason, printing to stdout in this test did not work when running
  // with FileCheck. I tried puts, printf and std::cout, using fflush(stdout)
  // and std::endl as well, but the output would only be there when not piping
  // to FileCheck. Therefore we use a terminate handler here, which will just
  // do the checks instead of printing strings and using FileCheck.
  std::set_terminate([]() {
    assert(destructor_throw && test_catch && !int_catch);
    std::exit(0);
  });

  try {
    try {
      throw Test();
    } catch (Test &t) {
      test_catch = true;
    }
  } catch (int i) {
    int_catch = true;
  }

  assert(false && "should call terminate due to an uncaught exception");
}
