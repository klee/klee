// RUN: %llvmgcc %s -emit-llvm -g -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out %t1.bc 2> %t.stderr.log
// RUN: FileCheck %s -check-prefix=CHECK-WRN --input-file=%t.klee-out/warnings.txt
// RUN: FileCheck %s -check-prefix=CHECK-ERR --input-file=%t.stderr.log

int main() {
  unsigned a, b, c, d, e;
  const char *invalid_pointer = 0xf;

  klee_make_symbolic(&a, sizeof(a), "");
  //CHECK-WRN: KLEE: WARNING: klee_make_symbolic: renamed empty name to "unnamed"


  klee_make_symbolic(&b, sizeof(b));
  //CHECK-WRN: KLEE: WARNING: klee_make_symbolic: deprecated number of arguments (2 instead of 3)
  //CHECK-WRN: KLEE: WARNING: klee_make_symbolic: renamed empty name to "unnamed"

  if(a == 2)
    klee_make_symbolic(&d, sizeof(e), invalid_pointer);
    //CHECK-ERR-DAG: KLEE: ERROR: {{.*}} Invalid string pointer passed to one of the klee_ functions
  if(a == 3)
    klee_make_symbolic(&d, sizeof(e), (char *) b);
    //CHECK-ERR-DAG: KLEE: ERROR: {{.*}} Symbolic string pointer passed to one of the klee_ functions

  klee_make_symbolic(&c);
  //CHECK-ERR-DAG: KLEE: ERROR: {{.*}} illegal number of arguments to klee_make_symbolic(void*, size_t, char*)
}
