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
void MyWriteCharToStringAtOffset(char *p,int offset2,char c);
int  MyReadCharAtConstOffset_Is_EQ_ToConstChar(char *p,int offset,char c);
int  MyReadCharAtConstOffset_Is_NEQ_ToConstChar(char *p,int offset,char c);

void MyMyPrintOutput(char *s)
{
	fprintf(stdout,"%s\n",s);
}

/************/
/* main ... */
/************/
int main(int argc, char **argv)
{
	int   i1;
	int   i2;
	int   i3;
	int   i4;
	int   i5;
	int   i6;
	int   i7;
	int   i8;
	int   i9;
	int  i10;
	int  i11;
	int  i12;
	int  i13;
	char * p1;
	char * p3;
	char * p4;
	char * p5;
	char * p6;
	char * p7;
	char * p8;
	char * p9;
	char * p10;
	char * p11;
	char * p12;
	char * p13;
	char * p14;
	char * p15;
	char * p16;
	char * p17;
	char * p18;
	char * p20;
	char * p21;
	char * p22;
	char * p23;
	char * q1 = "SIP/";

	// char msg[700];

	///////////////////////////////
	//klee_make_symbolic(msg,700,"msg");
	//p1 = msg;
	///////////////////////////////
	klee_make_symbolic( &i1, sizeof(i1), "i1" );
	klee_make_symbolic( &i1, sizeof(i2), "i2" );
	klee_make_symbolic( &i1, sizeof(i3), "i3" );
	klee_make_symbolic( &i1, sizeof(i4), "i4" );
	klee_make_symbolic( &i1, sizeof(i5), "i5" );
	klee_make_symbolic( &i1, sizeof(i6), "i6" );
	klee_make_symbolic( &i1, sizeof(i7), "i7" );
	klee_make_symbolic( &i1, sizeof(i8), "i8" );
	klee_make_symbolic( &i1, sizeof(i9), "i9" );
	klee_make_symbolic( &i1, sizeof(i10),"i10");
	klee_make_symbolic( &i1, sizeof(i11),"i11");
	klee_make_symbolic( &i1, sizeof(i12),"i12");
	klee_make_symbolic( &i1, sizeof(i13),"i13");

	p1 = malloc(32);

	p1[31] = 0;

	/* skip initial \r\n */
	while (p1[0] == '\r' || p1[0] == '\n')
	{
		p1++;
	}

	for (; p1[0] != 0; p1 = p1+1)
	{
		if ((0 == p1[0]) ||
			(0 == p1[1]) ||
			(0 == p1[2]) ||
			(0 == p1[3]))
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
			MyMyPrintOutput("OSIP_SYNTAXERROR 1");
			return 0;
		}
		i1 = p3 - p1;
		i2 = p3 - p6;
		i3 = i1 +  1;
		p5 = malloc(i3);
		strncpy (p5, p6, i2);
		p14 = p3 + 1;
		p4 = strchr (p14, ' ');
		if (p4 == NULL)
		{
			MyMyPrintOutput("OSIP_SYNTAXERROR 2");
			return 0;
		}
		{
			p7 = p4;

			while ((p7[0] != '\r') && (p7[0] != '\n'))
			{
				if (p7[0] != 0)
				{
					p7++;
				}
				else
				{
					MyMyPrintOutput("No crlf found\n");
				}
    		}
			i4 = p7 - p4;
			i5 = i4 - 1;
			p8 = malloc(i4);
			p15 = p4 + 1;
			strncpy (p8, p15, i5);

			p7++;
			if ((p7[0] != 0) && ('\r' == p7[-1]) && ('\n' == p7[0]))
			{
				p7++;
			}
		}
	}
	else
	{
		/* The first token is the method name: */
		p12 = strchr (p1, ' ');
		if (p12 == NULL)
		{
			MyMyPrintOutput("OSIP_SYNTAXERROR 5\n");
		}
		if (p12[1] == 0 || p12[2] == 0)
		{
			MyMyPrintOutput("OSIP_SYNTAXERROR 6\n");
		}
		i6 = p12 - p1;
		if (i6 == 0)
		{
			MyMyPrintOutput("No space allowed here\n");
		}
		i7 = i6 + 1;
		p13 = malloc(i7);
		strncpy (p13, p1, i6);
		p13[i6] = 0;

		/* The second token is a sip-url or a uri: */
		p21 = p12 + 2;
		p11 = strchr (p21, ' ');/* no space allowed inside sip-url */
		if (p11 == NULL)
		{
			MyMyPrintOutput("Uncompliant request-uri\n");
		}
		i10 = p11 - p12;
		if (i10 < 2)
		{
			MyMyPrintOutput("OSIP_SYNTAXERROR 7\n");
		}

		/* find the the version and the beginning of headers */
		p20 = p11;

		p20++;                       /* skip space */
		if (p20[0] == 0 ||
			p20[1] == 0 ||
			p20[2] == 0 ||
			p20[3] == 0 ||
			p20[4] == 0 ||
			p20[5] == 0 ||
			p20[6] == 0)
		{
			MyMyPrintOutput("Uncomplete request line\n");
		}
		if (((p20[0] != 'S') && (p20[0] != 's')) ||
			((p20[1] != 'I') && (p20[1] != 'i')) ||
			((p20[2] != 'P') && (p20[2] != 'p')) ||
			(p20[3] != '/'))
		{
			MyMyPrintOutput("No crlf found/No SIP/2.0 found\n");
		}

		/* SIP/ */
		p20 = p20 + 4;

		while ((p20[0] != '\r') && (p20[0] != '\n'))
		{
			if (p20[0] != 0)
			{
				if ((p20[0] >= '0') && (p20[0] <= '9'))
				{
					p20++;
				}
				else if (p20[0] == '.')
				{
					p20++;
				}
				else
				{
					MyMyPrintOutput("incorrect sip version string\n");
				}
			}
			else
			{
				MyMyPrintOutput("No crlf found\n");
			}
		}
		i8 = p20 - p11;
		if (i8 < 2)
		{
			MyMyPrintOutput("OSIP_SYNTAXERROR 8");
		}

		p18 = malloc(i8);
		i9 = i8-1;
		p23 = p1 + 1;
		strncpy (p18, p23, i9);
		p18[i9] = 0;

		if (0 != strcasecmp (p18, "SIP/2.0"))
		{
			MyMyPrintOutput("Wrong version number\n");
		}

		p20++;
		if ((p20[0] != 0) && ('\r' == p20[-1]) && ('\n' == p20[0]))
		{
			p20++;
		}
	}
	MyMyPrintOutput(">> I'M OUT OF HERE !!!\n\n");
}
