// RUN: %clang %s -emit-llvm %O0opt -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out %t1.bc

unsigned sym() {
  unsigned x;
  klee_make_symbolic(&x, sizeof x, "x");
  return x;
}

int main() {
  unsigned x, y;

  // sym returns a symbolic object, but because it is
  // alloca'd it is freed on sym()s return. thats fine,
  // but the problem is that IVC is going to try to write
  // into the object right here.
  //
  // to support this we need to have a facility for making
  // state local copies of a freed object.
  if (sym() == 0) 
    printf("ok\n");

  return 0;
}
