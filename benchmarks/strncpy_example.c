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
	char *p1;
	char *p2;
	char *p3;
	char *r1;
	int   i1;
	int   i2;
	int   i3;

	klee_make_symbolic(&i1,sizeof(i1),"i1");
	klee_make_symbolic(&i2,sizeof(i2),"i2");
	klee_make_symbolic(&i3,sizeof(i3),"i3");

	/****************************************/
	/*                                      */
	/* allocate buffers of symbolic size i1 */
	/*                                      */
	/****************************************/		
	p1 = malloc(7);
	p2 = malloc(7);
	r1 = malloc(5);

	/*****************/
	/*               */
	/* mark strings  */
	/*               */
	/*****************/
	markString(p1);
	markString(p2);
	markString(r1);
		
	/****************************************/
	/*                                      */
	/*       +---+---+---+---+---+---+---+  */
	/* p1 == | P | T | Q | D | ? | ? |x00|  */
	/*       +---+---+---+---+---+---+---+  */
	/*                                      */
	/****************************************/
	p1[0] = 'P';
	p1[1] = 'T';
	p1[2] = 'Q';
	p1[3] = 'D';
	p1[6] =  0 ;
		 
	/****************************************/
	/*                                      */
	/*       +---+---+---+---+---+---+---+  */
	/* p2 == | P | F | F | F |x00| ? | ? |  */
	/*       +---+---+---+---+---+---+---+  */
	/*                                      */
	/****************************************/
	p2[0] = 'P';
	p2[1] = 'F';
	p2[2] = 'F';
	p2[3] = 'F';
	p2[4] =  0 ;

	if (i3 > 0)
	{
		r1[4]=0;
		p3 = p1+1;
		strncpy(p3,r1,2);
	}
	if (strcmp(p2,p1) == 0)
	{
		/****************************************/
		/* In order for this if to be taken,    */
		/*                                      */
		/* i3 == 1                              */
		/*                                      */
		/*       +---+---+---+---+---+          */
		/* r1 == | F | F | F |x00| ? |          */
		/*       +---+---+---+---+---+          */
		/*                                      */
		/****************************************/
		MyPrintOutput(p1);
		MyPrintOutput(p2);
		MyPrintOutput(r1);
	}
}


