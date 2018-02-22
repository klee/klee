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

	/****************************************/
	/*                                      */
	/* allocate buffers of symbolic size i1 */
	/*                                      */
	/****************************************/
	p1 = malloc(10);
	p2 = malloc(10);

	markString(p1);
	markString(p2);

	p1[9]=0;
	p2[9]=0;
		
	if (strchr(p1,'A'))
	{
		if (strchr(p1,'B'))
		{
			if (strchr(p2,'D'))
			{
				if (strchr(p2,'M'))
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


