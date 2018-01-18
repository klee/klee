/*********************/
/* FILE NAME: util.c */
/*********************/

/****************************************/
/* WARNING DISABLE: sprintf - I love it */
/****************************************/
#pragma warning (disable: 4996)

/*************************/
/* GENERAL INCLUDE FILES */
/*************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*****************/
/* INCLUDE FILES */
/*****************/
#include "util.h"

void *checked_malloc(int len)
{
	void *p = malloc(len);
	if (!p)
	{
		fprintf(stderr,"\nRan out of memory!\n");
		exit(1);
	}
	
	return p;
}

string String(char *s)
{
	string p = (string) checked_malloc(strlen(s)+1);
	strcpy(p,s);
		
	return p;
}
