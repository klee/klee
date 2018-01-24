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

/************/
/* main ... */
/************/
int main(int argc, char **argv)
{
	char  c1;
	char  c2;
	int   i1;
	int   i2 = 3;
	char *p1;
	char *p2;

	/**********************************/
	/* [7] And make them symbolic ... */
	/**********************************/
	klee_make_symbolic(&i1,sizeof(i1),"i1");

	/********************************/
	/* [8] example tests to try out */
	/********************************/
	if ((5 <= i1) && (i1 <= 6))
	{	
		/****************************************/
		/*                                      */
		/* allocate buffers of symbolic size i1 */
		/*                                      */
		/****************************************/
		p1 = malloc(i1);
		p2 = malloc(i1);
		
		/********************************/
		/*                              */
		/*       +---+---+---+---+---+  */
		/* p1 == | P | ? | Q | D |x00|  */
		/*       +---+---+---+---+---+  */
		/*                              */
		/********************************/
		p1[0] = 'T';
		p1[1] = 'M';
		p1[2] = 'P';
		p1[3] = 'G';
		p1[4] =  0 ;
		 
		/********************************/
		/*                              */
		/*       +---+---+---+---+---+  */
		/* p2 == | P | ? | Q | D |x00|  */
		/*       +---+---+---+---+---+  */
		/*                              */
		/********************************/
		p2[0] = 'T';
		p2[1] = 'M';
		// p2[2] = 'P';
		p2[3] = 'G';
		p2[4] =  0 ;

		/********************************/
		/*                              */
		/*                              */
		/* can p1 and p2 be different ? */
		/*                              */
		/*                              */
		/********************************/
		c1 = p1[2];

		if (c1 > 'Q')
		{
			MyPrintOutput("INSIDE if (...)");
			assert(0);
		}
	}
}
