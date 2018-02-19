/*********************/
/* FILENAME: input.c */
/*********************/

/*************************/
/* GENERAL INCLUDE FILES */
/*************************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>

/**********************/
/* KLEE INCLUDE FILES */
/**********************/
#include "klee/klee.h"

void MyMalloc(char *p, int size);
void MyConstStringAssign(char *p,const char *q);
void MyStringAssignWithOffset(char *p, char *q,int offset1);
void MyWriteCharToStringAtOffset(char *p,int offset2,char c);

void MyPrintOutput(char *s)
{
	fprintf(stdout,"%s\n",s);
}

/************/
/* main ... */
/************/
int main(int argc, char **argv)
{
	char *p;
	char *q;
	char *r;

	unsigned int i;

	klee_make_symbolic(&i,sizeof( i ), "i");

	p = malloc(10);
	markString(p);

	q = p + i;

	// r = strchr(q, '#');

	klee_assume(i < 1);
	{
		if (q - p > 2)
		{
//MyPrintOutput("GOT IT !!! \n");
			fprintf(stdout,">> GOT IT !!! \n");
			assert(0);
		}
	}
}
