// RUN: %llvmgcc %s -emit-llvm -O0 -c -o %t1.bc
// RUN: %klee --exit-on-error --libc=uclibc %t1.bc

// just make sure atexit works ok

void boo() {
}

int main() {
  atexit(boo);
  return 0;
}
