#ifndef __TOKENS_H__
#define __TOKENS_H__

/***********************************/
/* FLEX uses 0-255 for inner stuff */
/***********************************/
#define SERIAL_NUMBER_START	256

/*********************/
/* TOKENS start here */
/*********************/
#define LPAREN		SERIAL_NUMBER_START+1
#define RPAREN		SERIAL_NUMBER_START+2
#define LBRACK		SERIAL_NUMBER_START+3
#define RBRACK		SERIAL_NUMBER_START+4
#define LBRACE		SERIAL_NUMBER_START+5
#define RBRACE		SERIAL_NUMBER_START+6
#define PLUS		SERIAL_NUMBER_START+7
#define MINUS		SERIAL_NUMBER_START+8
#define TIMES		SERIAL_NUMBER_START+9
#define DIVIDE		SERIAL_NUMBER_START+10
#define INT			SERIAL_NUMBER_START+11

/*********/
/* TYPES */
/*********/
typedef union
{
	union
	{
		int ival;
	}
	gval;
}
YYSTYPE;

/****************************/
/* EXTERNAL VARIABLE aalval */
/****************************/
extern YYSTYPE aalval;

#endif
