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
	int    i1;
	char  *p1;
	char  *p2;
	char r[5];

	klee_make_symbolic(&i1,sizeof(i1),"i1");

	/****************************************/
	/*                                      */
	/* allocate buffers of symbolic size i1 */
	/*                                      */
	/****************************************/
	p1 = malloc(10);
	p2 = malloc(10);
    markString(p1);
   	markString(p2);
   	markString(r);
		
	/********************************/
	/*                              */
	/*       +---+---+---+---+---+  */
	/* p1 == | L | ? | + | @ |x00|  */
	/*       +---+---+---+---+---+  */
	/*                              */
	/********************************/
	p1[0] = 'P';
	p1[2] = '+';
	p1[3] = '@';
	p1[4] =  0 ;
		 
	/********************************/
	/*                              */
	/*       +---+---+---+---+---+  */
	/* p2 == | P | ? | J | r |x00|  */
	/*       +---+---+---+---+---+  */
	/*                              */
	/********************************/
	p2[0] = 'P';
	p2[2] = 'J';
	p2[3] = 'r';
	p2[4] =  0 ;

	r[0] = p2[0];
	r[2] = p2[2];
	r[3] = p2[3];
	r[4] = p2[4];

	/*******************************/
	/*                             */
	/*                             */
	/* can p1 and r be different ? */
	/*                             */
	/*                             */
	/*******************************/
	if (strcmp(r,p1) != 0)
	{
		MyPrintOutput(p1);
		MyPrintOutput(p2);
		MyPrintOutput(r);
		assert(0);
	}
}
