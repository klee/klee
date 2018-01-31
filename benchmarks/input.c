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
	char *p1 = (char *) malloc(700);
	char *p3;
	char *p4;
	char *p5;
	char *p6;
	char *p7;
	char *p8;
	char *q1 = "SIP/";

	p1[696] = '\0';
	p1[697] = '\0';
	p1[698] = '\0';
	p1[699] = '\0';

	/* skip initial \r\n */
	while (p1[0] == '\r' || p1[0] == '\n')
	{
		p1++;
	}

	for (; p1[0] != '\0'; p1 = p1+1)
	{
		if (('\0' == p1[0]) ||
			('\0' == p1[1]) ||
			('\0' == p1[2]) ||
			('\0' == p1[3]))
		{
			return 0;
		}

		if ((('\r' == p1[0]) && ('\n' == p1[1]) && ('\r' == p1[2]) && ('\n' == p1[3])) ||
			(('\r' == p1[0]) && ('\r' == p1[1]))                                       ||
			(('\n' == p1[0]) && ('\n' == p1[1])))
		{
			/* end of message */
			return 0;
		}

		if ((('\r' == p1[0]) && ('\n' == p1[1]) && ((' ' == p1[2]) || ('\t' == p1[2]))) ||
			(('\r' == p1[0]) && ((' ' == p1[1]) || ('\t' == p1[1])))                    ||
			(('\n' == p1[0]) && ((' ' == p1[1]) || ('\t' == p1[1]))))
		{
			/* replace line end and TAB symbols by SP */
			p1[0] = ' ';
			p1[1] = ' ';
			p1 = p1 + 2;
			/* replace all following TAB symbols */
			for (; ('\t' == p1[0] || ' ' == p1[0]);)
			{
				p1[0] = ' ';
				p1++;
			}
		}
	}

	if (strncmp(p1,q1,4) != 0)
	{
		p6 = p1;

		/* search for first SPACE */
		p3 = strchr (p1, ' ');
		if (p3 == NULL)
		{
			MyPrintOutput("OSIP_SYNTAXERROR 1");
		}
		p5 = (char *) malloc (p3 - p6 + 1);
		strncpy (p5, p6, p3 - p6);

		p4 = strchr (p3 + 1, ' ');
		if (p4 == NULL)
		{
			MyPrintOutput("OSIP_SYNTAXERROR 2");
		}
		if (sscanf (p3 + 1, "%d", &i1) != 1)
		{
			/* Non-numeric status code */
			MyPrintOutput("OSIP_SYNTAXERROR 3");
		}

		if (i1 == 0)
		{
			MyPrintOutput("OSIP_SYNTAXERROR 4");
		}

		{
			char *p7 = p4;

			while ((*p7 != '\r') && (*p7 != '\n'))
			{
				if (*p7)
				{
					p7++;
				}
				else
				{
					MyPrintOutput("No crlf found\n");
				}
    		}
			p8 = (char *) malloc (p7 - p4);
			strncpy (p8, p4 + 1, p7 - p4 - 1);

			p7++;
			if ((*p7) && ('\r' == p7[-1]) && ('\n' == p7[0]))
			{
				p7++;
			}
		}
	}
}
