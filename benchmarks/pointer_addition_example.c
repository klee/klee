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
  klee_assume(i < 5);

	q = p + i;

	r = strchr(q, '#');

	if (i < 3)
	{
		if ((r - p > 5) && (r - q < 5))
		{
			printf(">> strchr + pointer arithmetic works !!!\n");
		}
	}
}
