// RUN: %llvmgcc %s -emit-llvm -O0 -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee -solver-backend=z3 -search=dfs -use-cex-cache=0 -use-independent-solver=0 --output-dir=%t.klee-out %t1.bc  > %t1.log
// RUN: grep AB_serial %t1.log |  wc -l | grep 2 
// RUN: grep AB_serial %t1.log | cut -d ' ' -f2 |sort -u | wc -l | grep 1
// RUN: grep 'i:' %t1.log | cut -d ' ' -f2  | grep 1

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>

#include "klee/klee.h"

int main(int argc, char **argv)
{
  char *p1, *p2, *r1, *p3;
  unsigned i2, i3;
  klee_make_symbolic(&i2, sizeof(i2), "i2");
  klee_make_symbolic(&i3, sizeof(i3), "i3");
    
 	p1 = malloc(5);
	p2 = malloc(5);
	r1 = malloc(3);
	markString(p1);
	markString(p2);
	markString(r1);
	
  p1[0] = 'P';
	p1[1] = 'T';
	p1[2] = 'Q';
	p1[3] = 'D';
	p1[4] =  0 ;
	 
  p2[0] = 'P';
	p2[1] = 'F';
	p2[2] = 'M';
	p2[3] = 'D';
	p2[4] =  0 ;

  p1[i2] = 0;

  r1[0] = 'T';
	r1[1] = 'Q';
	r1[2] =  0 ;

	if (i3 <= 1)
	{
		p3 = p2+i3;
		strcpy(p3,r1);
	}

	if (strcmp(p2,p1) == 0)
	{
      printf("i: %u\n", i3);
			MyPrintOutput(p1);
      printf("\n");
			MyPrintOutput(p2);
	}
}
