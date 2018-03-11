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
int osip_find_next_crlf(const char *start_of_header, const char **end_of_header)
{
	const char *soh = start_of_header;

	*end_of_header = NULL;        /* AMD fix */

	while (('\r' != *soh) && ('\n' != *soh))
	{
		if (*soh)
		{
			soh++;
		}
		else
		{
			// Whatever right? this is just a log thing ...
			// OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_ERROR, NULL, "Final CRLF is missing\n"));
			return -5;
			// return OSIP_SYNTAXERROR;
		}
	}

	if (('\r' == soh[0]) && ('\n' == soh[1]))
	{
		/* case 1: CRLF is the separator
		   case 2 or 3: CR or LF is the separator */
		soh = soh + 1;
	}

	/* VERIFY if TMP is the end of header or LWS.            */
	/* LWS are extra SP, HT, CR and LF contained in headers. */
	if ((' ' == soh[1]) || ('\t' == soh[1]))
	{
		/* From now on, incoming message that potentially
		contains LWS must be processed with
		-> void osip_util_replace_all_lws(char *)
		This is because the parser methods does not
		support detection of LWS inside. */
		// OSIP_TRACE (osip_trace (__FILE__, __LINE__, OSIP_BUG, NULL, "Message that contains LWS must be processed with osip_util_replace_all_lws(char *tmp) before being parsed.\n"));
		return -2;
	}

	*end_of_header = soh + 1;
	return 0;
	// return OSIP_SUCCESS;
}

int main(int argc, char **argv)
{
	int i;
	char *hname;
	char *hvalue;
	char *colon_index;
	char *start_of_line;
	char *end_of_line;

	char *message;

	start_of_line = malloc(50);
	markString(start_of_line);

    i = osip_find_next_crlf (start_of_line, &end_of_line);
    if (i == -2)
	{
		/* do nothing ... */
	}
	else if (i != 0)
	{
		return i;                 /* error case: no end of body found */
	}

	/* find the header name */
	colon_index = strchr (start_of_line, ':');
	if (colon_index == NULL)
	{
		return -5;  /* this is also an error case */
		// return OSIP_SYNTAXERROR;  /* this is also an error case */
	}

	if (colon_index - start_of_line + 1 < 2)
	{
		return -5;
		// return OSIP_SYNTAXERROR;
	}
	hname = (char *) malloc (colon_index - start_of_line + 1);
	markString(hname);

    if (hname == NULL)
	{
		return -2;
	}
    strncpy (hname, start_of_line, colon_index - start_of_line);

	if ((end_of_line - 2) - colon_index < 2)
	{
		// osip_free (hname);
		return -5;
		//return OSIP_SYNTAXERROR;
    }
	hvalue = (char *) malloc ((end_of_line - 2) - colon_index);
	markString(hvalue);

	if (hvalue == NULL)
	{
		// osip_free (hname);
		return -4;
		// return -2;
    }
	strncpy (hvalue, colon_index + 1, (end_of_line - 2) - colon_index - 1);

	/* really store the header in the sip structure */
	message = (char *) malloc(13);
	markString(message);
	message[ 0]='c';
	message[ 1]='o';
	message[ 2]='n';
	message[ 3]='t';
	message[ 4]='e';
	message[ 5]='n';
	message[ 6]='t';
	message[ 7]='-';
	message[ 8]='t';
	message[ 9]='y';
	message[10]='p';
	message[11]='e';
	message[12]= 0 ;
	if (strncmp (hname, message, 12) == 0)
	{
		MyPrintOutput(hname);
		fprintf(stdout,">> GOT HERE !!!\n");
	}
}
