// RUN: %llvmgcc %s -emit-llvm -O0 -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee -solver-backend=z3 -search=dfs -use-cex-cache=0 -use-independent-solver=0 --output-dir=%t.klee-out %t1.bc  > %t1.log
// RUN: cat %t1.log |  wc -l | grep 2
// RUN: cat %t1.log | cut -d ' ' -f2 | grep A | grep C | grep D | grep E | wc -l |grep 2
// RUN: grep -P '[ACDE]{4}\\x00$' %t1.log | wc -l | grep 2
// RUN: cat %t1.log | cut -d ' ' -f2 |sort -u | wc -l | grep 1
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>

#include "klee/klee.h"

int main(int argc, char **argv)
{
	int   i1;
	char *p1;
	char *p2;

	p1 = malloc(5);
	p2 = malloc(5);
	markString(p1);
	markString(p2);

	//p1[4]=0;  
	//p2[4]=0;  

	if (strchr(p1,'M'))
	{
		if (strchr(p2,'C'))
		{
			if (strchr(p2,'D'))
			{
				if (strcmp(p1,p2) == 0)
				{
					printf(">> 1111\n");
					MyPrintOutput(p1);
					printf("\n");
					MyPrintOutput(p2);
				}
				else
				{
					printf(">> 1110\n");
					// MyPrintOutput(p1);
					// printf("\n");
					// MyPrintOutput(p2);
				}
			}
			else
			{
				printf(">> 110\n");
				// MyPrintOutput(p1);
				// printf("\n");
				// MyPrintOutput(p2);
			}
		}
		else
		{
			printf(">> 10\n");
			// MyPrintOutput(p1);
			// printf("\n");
			// MyPrintOutput(p2);
		}
	}
	else
	{
		printf(">> 0\n");
		// MyPrintOutput(p1);
		// printf("\n");
		// MyPrintOutput(p2);
	}
}
