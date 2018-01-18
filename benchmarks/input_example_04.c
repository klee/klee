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
	if (i1 <= 30)
	{	
		/****************************************/
		/*                                      */
		/* allocate buffers of symbolic size i1 */
		/*                                      */
		/****************************************/
		p1 = malloc(i1);
		p2 = malloc(i1);
		
		if (strchr(p1,'A'))
		{
			if (strchr(p1,'B'))
			{
				if (strchr(p1,'C'))
				{
					if (strchr(p2,'D'))
					{
						if (strchr(p2,'E'))
						{
							if (strchr(p2,'F'))
							{
								if (strlen(p1) <= 6)
								{
									if (strlen(p2) <= 6)
									{
										if (strcmp(p1,p2) == 0)
										{
											MyPrintOutput("INSIDE if (...)");
											assert(0);
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}
}
