// RUN: %clang %s -emit-llvm %O0opt -g -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee -write-exec-tree --output-dir=%t.klee-out %t.bc
// RUN: %klee-exec-tree branches %t.klee-out/exec_tree.db | FileCheck --check-prefix=CHECK-BRANCH %s
// RUN: %klee-exec-tree depths %t.klee-out | FileCheck --check-prefix=CHECK-DEPTH %s
// RUN: %klee-exec-tree instructions %t.klee-out | FileCheck --check-prefix=CHECK-INSTR %s
// RUN: %klee-exec-tree terminations %t.klee-out | FileCheck --check-prefix=CHECK-TERM %s
// RUN: %klee-exec-tree tree-dot %t.klee-out | FileCheck --check-prefix=CHECK-DOT %s
// RUN: %klee-exec-tree tree-info %t.klee-out | FileCheck --check-prefix=CHECK-TINFO %s
// RUN: not %klee-exec-tree dot %t.klee-out/exec-tree-doesnotexist.db

#include "klee/klee.h"

#include <stddef.h>

int main(void) {
  int a = 42;
  int c0, c1, c2, c3;
  klee_make_symbolic(&c0, sizeof(c0), "c0");
  klee_make_symbolic(&c1, sizeof(c1), "c1");
  klee_make_symbolic(&c2, sizeof(c2), "c2");
  klee_make_symbolic(&c3, sizeof(c3), "c3");

  if (c0) {
    a += 17;
  } else {
    a -= 4;
  }

  if (c1) {
    klee_assume(!c1);
  } else if (c2) {
    char *p = NULL;
    p[4711] = '!';
  } else if (c3) {
    klee_silent_exit(0);
  } else {
    return a;
  }

  return 0;
}

// CHECK-BRANCH: branch type,count
// CHECK-BRANCH: Conditional,7

// CHECK-DEPTH: depth,count
// CHECK-DEPTH: 3,2
// CHECK-DEPTH: 4,2
// CHECK-DEPTH: 5,4

// CHECK-INSTR: asm line,branches,terminations,termination types
// CHECK-INSTR-DAG: {{[0-9]+}},0,2,User(2)
// CHECK-INSTR-DAG: {{[0-9]+}},0,2,Ptr(2)
// CHECK-INSTR-DAG: {{[0-9]+}},0,2,SilentExit(2)
// CHECK-INSTR-DAG: {{[0-9]+}},0,2,Exit(2)

// CHECK-TERM: termination type,count
// CHECK-TERM-DAG: Exit,2
// CHECK-TERM-DAG: Ptr,2
// CHECK-TERM-DAG: User,2
// CHECK-TERM-DAG: SilentExit,2

// CHECK-DOT: strict digraph ExecutionTree {
// CHECK-DOT: node[shape=point,width=0.15,color=darkgrey];
// CHECK-DOT: edge[color=darkgrey];
// CHECK-DOT-DAG: N{{[0-9]+}}[tooltip="Conditional\nnode: {{[0-9]+}}\nstate: 0\nasm: {{[0-9]+}}"];
// CHECK-DOT-DAG: N{{[0-9]+}}[tooltip="Exit\nnode: {{[0-9]+}}\nstate: {{[0-9]+}}\nasm: {{[0-9]+}}",color=green];
// CHECK-DOT-DAG: N{{[0-9]+}}[tooltip="SilentExit\nnode: {{[0-9]+}}\nstate: {{[0-9]+}}\nasm: {{[0-9]+}}",color=orange];
// CHECK-DOT-DAG: N{{[0-9]+}}[tooltip="Ptr\nnode: {{[0-9]+}}\nstate: {{[0-9]+}}\nasm: {{[0-9]+}}",color=red];
// CHECK-DOT-DAG: N{{[0-9]+}}[tooltip="User\nnode: {{[0-9]+}}\nstate: {{[0-9]+}}\nasm: {{[0-9]+}}",color=blue];
// CHECK-DOT-DAG: N{{[0-9]+}}->{N{{[0-9]+}} N{{[0-9]+}}};
// CHECK-DOT-DAG: }

// CHECK-TINFO: nodes: 15
// CHECK-TINFO: leaf nodes: 8
// CHECK-TINFO: max. depth: 5
// CHECK-TINFO: avg. depth: 4.2
