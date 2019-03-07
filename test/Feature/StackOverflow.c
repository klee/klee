// RUN: %clang -emit-llvm -g -c -o %t.bc %s
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out %t.bc > %t.output.log 2>&1
// RUN: FileCheck -input-file=%t.output.log %s

void recursive(unsigned nr){
  if (nr == 0)
    return;
  recursive(nr-1);
}

int main() {
  recursive(10000);
  return 0;
}
// CHECK: Maximum stack size reached
