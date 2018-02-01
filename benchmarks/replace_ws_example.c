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

char *osip_clrncpy (char *dst, const char *src, size_t len)
{
	const char *pbeg;
	const char *pend;
	char *p;
	size_t spaceless_length;

	if (src == NULL)
		return NULL;

	/* find the start of relevant text */
	pbeg = src;
	while ((' ' == *pbeg) || ('\r' == *pbeg) || ('\n' == *pbeg) || ('\t' == *pbeg))
		pbeg++;


	/* find the end of relevant text */
	pend = src + len - 1;
	while ((' ' == *pend) || ('\r' == *pend) || ('\n' == *pend) || ('\t' == *pend))
	{
		pend--;
		if (pend < pbeg)
		{
			*dst = '\0';
			return dst;
		}
	}

	/* if pend == pbeg there is only one char to copy */
	spaceless_length = pend - pbeg + 1;   /* excluding any '\0' */
	memmove (dst, pbeg, spaceless_length);
	p = dst + spaceless_length;

	/* terminate the string and pad dest with zeros until len */
	do
	{
		*p = '\0';
		p++;
		spaceless_length++;
	}
	while (spaceless_length < len);

	return dst;
}

char *osip_strncpy (char *dest, const char *src, size_t length)
{
	strncpy (dest, src, length);
	dest[length] = '\0';
	return dest;
}

void MyPrintOutput(char *s)
{
	fprintf(stdout,"%s\n",s);
}
/************/
/* main ... */
/************/
int main(int argc, char **argv)
{
	int   i1;
	char *p1;
	char *p3;
	char *p4;
	char *p5;
	char *p6;
	char *p7;
	char *p8;
	char *p9;
	char *p10;
	char *p11;
	char *p12;
	char *p13;
	char *p14;
	char *p15;
	char *p16;
	char *p17;
	char *p18;
	char *q1 = "SIP/";

	char msg[700];

	///////////////////////////////
	// p1 = (char *) malloc(700);//
	///////////////////////////////
	klee_make_symbolic(msg,700,"msg");
	p1 = msg;

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
			p7 = p4;

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
	else
	{
		/* The first token is the method name: */
		p12 = strchr (p1, ' ');
		if (p12 == NULL)
		{
			MyPrintOutput("OSIP_SYNTAXERROR 5\n");
		}
		if (*(p12 + 1) == '\0' || *(p12 + 2) == '\0')
		{
			MyPrintOutput("OSIP_SYNTAXERROR 6\n");
		}
		if (p12 - p1 == 0)
		{
			MyPrintOutput("No space allowed here\n");
		}
		p13 = (char *) malloc (p12 - p1 + 1);
		osip_strncpy (p13, p1, p12 - p1);

		/* The second token is a sip-url or a uri: */
		p11 = strchr (p12 + 2, ' ');/* no space allowed inside sip-url */
		if (p11 == NULL)
		{
			MyPrintOutput"Uncompliant request-uri\n");
		}
		if (p11 - p12 < 2)
		{
			MyPrintOutput("OSIP_SYNTAXERROR 7\n");
		}

		p17 = (char *) malloc (p11 - p12);
		osip_clrncpy (p17, p12 + 1, (p11 - p12 - 1));

		/* find the the version and the beginning of headers */
		hp = p11;

		hp++;                       /* skip space */
		if (*hp == '\0'       ||
			*(hp + 1) == '\0' ||
			*(hp + 2) == '\0' ||
			*(hp + 3) == '\0' ||
			*(hp + 4) == '\0' ||
			*(hp + 5) == '\0' ||
			*(hp + 6) == '\0')
		{
			MyPrintOutput("Uncomplete request line\n");
		}
		if (((hp[0] != 'S') && (hp[0] != 's')) ||
			((hp[1] != 'I') && (hp[1] != 'i')) ||
			((hp[2] != 'P') && (hp[2] != 'p')) ||
			(hp[3] != '/'))
		{
			MyPrintOutput("No crlf found/No SIP/2.0 found\n"));
		}
		/* SIP/ */
		hp = hp + 4;

		while ((*hp != '\r') && (*hp != '\n'))
		{
			if (*hp)
			{
				if ((*hp >= '0') && (*hp <= '9'))
				{
					hp++;
				}
				else if (*hp == '.')
				{
					hp++;
				}
				else
				{
					MyPrintOutput("incorrect sip version string\n");
				}
			}
			else
			{
				MyPrintOutput("No crlf found\n");
			}
		}
		if (hp - p11 < 2)
		{
			MyPrintOutput("OSIP_SYNTAXERROR 8");
		}

		p18 = (char *) malloc (hp - p11);
		osip_strncpy (p18, p1 + 1, (hp - p1 - 1));
		if (0 != strcasecmp (p18, "SIP/2.0"))
		{
			MyPrintOutput("Wrong version number\n");
		}

		hp++;
		if ((*hp) && ('\r' == hp[-1]) && ('\n' == hp[0]))
		{
			hp++;
		}
	}
	MyPrintOutput(">> I'M OUT OF HERE !!!\n\n");
}
