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
	char *colon_index, *p, *hname;
  int i;

	klee_make_symbolic(&i,sizeof( i ), "i");
  
	p = malloc(5);
	markString(p);

  p[0] = 'l';
  p[1] = ':';
  p[2] = 'e';
  p[4] = 0;
  colon_index = strchr (p, ':');

  hname = (char *) malloc (colon_index - p + 1);
  markString(hname);
  if(strlen(hname) > 0) {
      MyPrintOutput(p);
  }

}
