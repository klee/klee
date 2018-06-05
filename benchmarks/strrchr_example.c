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

char *f1(char *p)
{
	while (p[0] == 'a' || p[0] == 'b')
	{
		p++;
	}
	return p;	
}

/************/
/* main ... */
/************/
int main(int argc, char **argv)
{
	char *p;
	
	p = malloc(20);
	klee_make_symbolic(p,20,"p");
	markString(p);

	p[ 0] = 'a';
	p[ 1] = 'b';
	p[ 2] = 'a';
	p[ 3] = 'b';
	// p[ 4] = 'M';
	p[ 5] = 'a';
	p[ 6] = 'b';
	// p[ 7] = 'Q';
	p[ 8] = 'a';
	p[ 9] = 0;
	p[10] = 'a';
	p[11] = 'b';
	p[12] = 'b';
	p[13] = 'R';
	p[14] = '8';
	p[15] = '6';
	p[16] =  0 ;
	p[17] =  0 ;
	p[18] =  0 ;
	p[19] =  0 ;

	if (strrchr(p,'a'))
	{
		assert(0);
	}
}
