// RUN: %clangxx %s -emit-llvm -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: klee --output-dir=%t.klee-out %t1.bc 2>&1 | FileCheck %s

// CHECK: Using zero size array fix for landingpad instruction filter

// Check that the zero size array in the landing pad filter does not crash KLEE
int p() throw() { throw 'a'; }
int main(int argc, char **) {
  if (argc < 3) {
    return 0;
  }

  try {
    return p();
  } catch (...) {
    return 1;
  }
}
