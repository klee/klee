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
	int   i2;
	char *p1;
	char *p2;

	/**********************************/
	/* [7] And make them symbolic ... */
	/**********************************/
	klee_make_symbolic(&i1,sizeof(i1),"i1");
	klee_make_symbolic(&i2,sizeof(i2),"i2");

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
		/* p1 == | P | R | Q | D |x00|  */
		/*       +---+---+---+---+---+  */
		/*                              */
		/********************************/
		p1[0] = 'P';
		p1[1] = 'R';
		p1[2] = 'Q';
		p1[3] = 'D';
		p1[4] =  0 ;
		 
		/********************************/
		/*                              */
		/*       +---+---+---+---+---+  */
		/* p2 == | P | ? | Q | D |x00|  */
		/*       +---+---+---+---+---+  */
		/*                              */
		/********************************/
		p2[0] = 'P';
		p2[2] = 'Q';
		p2[3] = 'D';
		p2[4] =  0 ;

		/************************************************************/
		/*                            Write x00:                    */
		/*                                                          */
		/*                 +-------either      or -----+            */
		/*                 |                           |            */
		/*                 |                           |            */
		/*             inside AB:                   outside AB:     */
		/*                 .                            ...         */
		/*                 .                           .....        */
		/*             .   .   .                     ........       */
		/*         .   .   .   .   .               ...........      */
		/*         ?   ?   ?   ?   ?             ..............     */
		/*         +---+---+---+---+           .................    */
		/*         |   |   |   |   |         ....................   */
		/*         V   V   V   V   V       .......................  */
		/*       +---+---+---+---+---+    ......................... */
		/* p1 == | P | R | Q | D |x00|    ERROR OUT OF BOUND ACCESS */
		/*       +---+---+---+---+---+                              */
		/*                                                          */
		/************************************************************/
		p1[i2] = 0;

		if ((1 < strlen(p1)) && (strlen(p1) < strlen(p2)))
		{
			MyPrintOutput("INSIDE if (...)");
			assert(0);
		}
	}
}
