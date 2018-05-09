// RUN: %llvmgcc %s -emit-llvm -g -O0 -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee -solver-backend=z3 -search=dfs -use-cex-cache=0 -use-independent-solver=0 -use-query-log=all:kquery --output-dir=%t.klee-out %t1.bc  > %t1.log
// RUN: cat %t1.log |  wc -l | grep 2
// RUN: grep 'Bigger string' %t1.log 
// RUN: grep 'Small string' %t1.log 

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>

#include "klee/klee.h"

int main(int argc, char **argv)
{
	char *p1;
  int *ip = malloc(sizeof(int));


  p1 = malloc(5);
  markString(p1);
	
	p1[4] =  0 ;
  *ip = strlen(p1);
  if(*ip < 2) {
      printf("Small string\n");

  } else {
      printf("Bigger string\n");
  }
		 
  	
}
