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
	int   i1;
	char *p1;
	char *p2;
	char r[5];

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
	    markString(p1);
    	markString(p2);
    	markString(r);
		
		/********************************/
		/*                              */
		/*       +---+---+---+---+---+  */
		/* p1 == | P | ? | Q | D |x00|  */
		/*       +---+---+---+---+---+  */
		/*                              */
		/********************************/
		p1[0] = 'P';
		p1[2] = 'M';
		p1[3] = 'T';
		p1[4] =  0 ;
		 
		/********************************/
		/*                              */
		/*       +---+---+---+---+---+  */
		/* p2 == | P | ? | Q | D |x00|  */
		/*       +---+---+---+---+---+  */
		/*                              */
		/********************************/
		p2[0] = 'P';
		p2[2] = 'M';
		p2[3] = 'T';
		p2[4] =  0 ;

		r[0] = p2[0];
		r[2] = p2[2];
		r[3] = p2[3];
		r[4] = p2[4];

		//if (r[2] == p1[3])
		//{
		//	MyPrintOutput("INSIDE if (...)");
		//}
		//r[2] = p2[2];
		//r[3] = p2[3];
		//r[4] = p2[4];

		/********************************/
		/*                              */
		/*                              */
		/* can p1 and r  be different ? */
		/*                              */
		/*                              */
		/********************************/
		if (strcmp(r,p1) != 0)
		{
			MyPrintOutput("INSIDE if (...)");
			assert(0);
		}
	}
}
