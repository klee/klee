// Check that we properly associate instruction level statistics with source
// file and line.
//
// RUN: %llvmgcc %s -emit-llvm -g -O0 -c -o %t1.bc
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


// Ensure we have the right line number (and check called function markers).
// CHECK: fn=f1
// CHECK: cfn=f0
// CHECK-NEXT: calls=1 {{[1-9][0-9]*}}
// CHECK-NEXT: {{[1-9][0-9]*}} 48 {{.*}}

// This check just brackets the checks above, to make sure they fall in the
// appropriate region of the run.istats file.
//
// CHECK: fn=main







// KEEP THIS AS LINE 40

int f0(int a, int b) {
  return a + b;
}

int f1(int a, int b) {
  // f0 is called on line 48
  return f0(a, b);
}

int main() {
  int x = f1(1, 2);
  
  return x;
}
