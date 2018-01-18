/***********************/
/* FILE NAME: driver.c */
/***********************/

/*************************/
/* GENERAL INCLUDE FILES */
/*************************/
#include <stdio.h>

/*************************/
/* PROJECT INCLUDE FILES */
/*************************/
#include "util.h"
#include "ErrorMsg.h"
#include "Tokens.h"

/*******************/
/* LEXING FUNCTION */
/*******************/
int aalex(void);

/*********/
/* USAGE */
/*********/
void usage(int argc, char **argv)
{
	if (argc != 3)
	{
		fprintf(stderr,"usage: a.out filename\n");
		return 0;
	}
}

/*****************/
/* START OF CODE */
/*****************/
int main(int argc, char **argv)
{
	/********************************/
	/* [1] Input & Output filenames */
	/********************************/
	string Input_Filename =argv[1];
	string Output_Filename=argv[2];
		
	/*****************/
	/* [2] Usage ... */
	/*****************/
	usage(argc,argv);
	
	/***************************************************/
	/* [3] Set Lex Logs and open input file for lexing */
	/***************************************************/
	ErrorMsg_Set_Log_Filename(Output_Filename);	
	ErrorMsg_Reset(Input_Filename);

	/****************************************/
	/* [4] Lex me all the way down baby !!! */
	/****************************************/
	while (aalex());
				
	/******************/
	/* [5] return ... */
	/******************/
	return 0;
}

