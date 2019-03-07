// RUN: %clang %s -emit-llvm -g %O0opt -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --use-cex-cache=false %t.bc >%t1.log

// This bug is triggered when using STP up to an including 2.1.0
// See https://github.com/klee/klee/issues/308
// and https://github.com/stp/stp/issues/206

int b, a, g;

int *c = &b, *d = &b, *f = &a;

int safe_div(short p1, int p2) { 
  return p2 == 0 ? p1 : p2; 
}

int main() {
  klee_make_symbolic(&b, sizeof b, "b");
  if (safe_div(*c, 0))
    *f = (int)&b % *c;

  safe_div(a && g, *d);
}
