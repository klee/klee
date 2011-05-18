// RUN: %llvmgxx -g -fno-exceptions -emit-llvm -O0 -c -o %t.bc %s
// RUN: %klee --libc=klee --no-output --exit-on-error %t.bc > %t.log

/* Tests the ability to call external functions which return large values
 * (i.e. structs).  In this test case, fstream::ftellg() returns a
 * streampos (an {i64, i64} pair) which is implicitly converted to a size_t. */

#include <fstream>

using namespace std;

size_t fileSize(const char *filename) {
  fstream f(filename, fstream::in | fstream::binary);

  if (f.is_open()) {
    f.seekg(0, fstream::end);
    size_t fileSize = f.tellg();
    return fileSize;
  }

  return (size_t) -1;
}

int main(void) {
  size_t size = fileSize("/bin/sh");
  return (size != (size_t) -1) ? 0 : 1;
}
