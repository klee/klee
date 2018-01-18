%{
/*************************/
/* GENERAL INCLUDE FILES */
/*************************/
#include <math.h>
#include <string.h>
#include <stdlib.h>

/*************************/
/* PROJECT INCLUDE FILES */
/*************************/
#include "ErrorMsg.h"
#include "Tokens.h"
#include "util.h"

/**************************/
/* CONTROL ERROR MESSAGES */
/**************************/
static int charPos=1;

/******************/
/* PROVIDE aaWRAP */
/******************/
int aawrap(void)
{
	charPos=1;
	return 1;
}

/**************************/
/* CONTROL ERROR MESSAGES */
/**************************/
static void adjust(void)
{
	ErrorMsg_tokPos = charPos;
	charPos += aaleng;
}

/***********/
/* YYSTYPE */
/***********/
YYSTYPE aalval;

%}

/*****************/
/* UNIQUE PREFIX */
/*****************/
%option prefix="aa"

/********************/
/* COMMON REGEXP(s) */
/********************/

/*******/
/* INT */
/*******/
INT	[0-9]+

/**************/
/* Z3 QUERIES */
/**************/
START_Z3_QUERY	"; start Z3 query"
END___Z3_QUERY	"; end Z3 query"
		
/*********/
/* RULES */
/*********/
%%
{START_Z3_QUERY}	{adjust(); ErrorMsg_OpenLog();   continue; }
{END___Z3_QUERY}	{adjust(); ErrorMsg_CloseLog();  continue; }
[^;]*				{adjust(); ErrorMsg_Log(aatext); continue; }
