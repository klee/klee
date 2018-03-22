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
	char c1;
	char *p1;

	p1 = malloc(8);
	
	markString(p1);
	
	if (p1[17] == '\r')
	{
		MyPrintOutput(p1);
	}
}
