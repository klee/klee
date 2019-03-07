// Check that we properly associate instruction level statistics with source
// file and line.
//
// RUN: %clang %s -emit-llvm -g %O0opt -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --exit-on-error %t1.bc
// RUN: FileCheck < %t.klee-out/run.istats %s

// CHECK: positions: instr line
// CHECK: ob={{.*}}/SourceMapping.c{{.*}}/assembly.ll

// Assuming the compiler doesn't reorder things, f0 should be first, and it
// should immediately follow the first file name marker.
// CHECK: fl={{.*}}/SourceMapping.c
// CHECK-NEXT: fn=f0

// Ensure we have a known position for the first instruction (instr and line
// should be non-zero).

// CHECK-NEXT: {{[1-9][0-9]*}} {{[1-9][0-9]*}}

int f0(int a, int b) {
  return a + b;
}

int f1(int a, int b) {
  // Ensure we have the right line number (and check called function markers).
  // CHECK: fn=f1
  // CHECK: cfn=f0
  // CHECK-NEXT: calls=1 {{[1-9][0-9]*}}
  // CHECK-NEXT: {{[1-9][0-9]*}} [[@LINE+1]] {{.*}}
  return f0(a, b);
}

// This check just brackets the checks above, to make sure they fall in the
// appropriate region of the run.istats file.
//
// CHECK: fn=main
int main() {
  int x = f1(1, 2);

  return x;
}
