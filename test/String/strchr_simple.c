// RUN: %llvmgcc %s -emit-llvm -O0 -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee -solver-backend=z3 -search=dfs -use-cex-cache=0 -use-independent-solver=0 --output-dir=%t.klee-out %t1.bc  > %t1.log
// RUN: grep -P 'M.*\\x00$' %t1.log

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>

#include "klee/klee.h"

int main(int argc, char **argv)
{
	char *p1;
  p1 = malloc(5);
  markString(p1);
  p1[4] = 0;
  
  if (strchr(p1,'M'))
  {
      MyPrintOutput(p1);
  }
}
