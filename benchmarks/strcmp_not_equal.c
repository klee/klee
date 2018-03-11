// RUN: %llvmgcc %s -emit-llvm -O0 -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee -solver-backend=z3 -search=dfs -use-cex-cache=0 -use-independent-solver=0 --output-dir=%t.klee-out %t1.bc  > %t1.log
// RUN: cat %t1.log |  wc -l | grep 2
// RUN: grep -P 'P(.|(\\x\d\d))MD\\x00$' %t1.log | wc -l | grep 2
// RUN: cat %t1.log | cut -d ' ' -f2 |sort -u | wc -l | grep 2

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>

#include "klee/klee.h"

int main(int argc, char **argv)
{
	char *p1;
	char *p2;


   	p1 = malloc(5);
		p2 = malloc(5);
    markString(p1);
    markString(p2);
		
  	p1[0] = 'P';
		p1[2] = 'M';
		p1[3] = 'D';
		p1[4] =  0 ;
		 
  	p2[0] = 'P';
		p2[2] = 'M';
		p2[3] = 'D';
		p2[4] =  0 ;

  	if (strcmp(p2,p1) != 0)
		{
			MyPrintOutput(p1);
      printf("\n");
			MyPrintOutput(p2);
		}
}
