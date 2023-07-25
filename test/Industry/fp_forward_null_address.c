#include <stdio.h>

#define MAX_LENGTH 100
typedef unsigned int U32;

int g_arr[MAX_LENGTH];

void foo()
{
  int *p = NULL;
  U32 loopCntMax = MAX_LENGTH;
  U32 i;
  for (i = 0; i < loopCntMax; i++) {
    if (g_arr[i] > 10) {
      p = &g_arr[i];
      break;
    }
  }
  if (i >= loopCntMax) {
    return;
  }
  int v = *p; // CHECK: KLEE: WARNING: 100.00% NullPointerException False Positive at trace 1
}

// RUN: %clang %s -emit-llvm -c -g -O0 -Xclang -disable-O0-optnone -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --max-cycles-before-stuck=150 --use-guided-search=error --annotations=%annotations --mock-policy=all --skip-not-symbolic-objects --skip-not-lazy-initialized --use-lazy-initialization=only --analysis-reproduce=%s.json %t1.bc
// RUN: FileCheck -input-file=%t.klee-out/warnings.txt %s
