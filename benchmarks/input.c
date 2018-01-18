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
#include "/home/oren/GIT/LatestKlee/myStrKlee/include/klee/klee.h"

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
		p1[4] =  0 ;
		p1[0] = 'P';
		p1[2] = 'Q';
		p1[3] = 'D';
		 
		/********************************/
		/*                              */
		/*       +---+---+---+---+---+  */
		/* p2 == | P | ? | Q | D |x00|  */
		/*       +---+---+---+---+---+  */
		/*                              */
		/********************************/
		p2[4] =  0 ;
		p2[0] = 'P';
		p2[2] = 'Q';
		p2[3] = 'D';

		/********************************/
		/*                              */
		/*                              */
		/* can p1 and p2 be different ? */
		/*                              */
		/*                              */
		/********************************/
		if (strcmp(p2,p1) != 0)
		{
			MyPrintOutput("INSIDE if (...)");
			assert(0);
		}
	}
}
