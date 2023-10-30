// RUN: %clang -std=c89 %s -emit-llvm %O0opt -g -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee -output-dir=%t.klee-out --search=bfs --max-instructions=1000 %t.bc
b;
a(c) {
  if (!c)
    abort();
}
main() {
  int e;
  short d;
  klee_make_symbolic(&d, sizeof(d), "__sym___VERIFIER_nondet_short");
  b = d;
  a(b > 0);
  int f[b];
  int g[b];
  for (;;) {
    short d;
    klee_make_symbolic(&d, sizeof(d), "__sym___VERIFIER_nondet_short");
    a(d >= 0 && d < b);
    f[d];
    g[b - 1 - d];
  }
}
