// RUN: %llvmgcc %s -emit-llvm -O0 -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --exit-on-error --libc=uclibc %t1.bc

// just make sure atexit works ok

void boo() {
}

int main() {
  atexit(boo);
  return 0;
}
