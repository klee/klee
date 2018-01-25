/* $Id$ $Revision$ */
/* vim:set shiftwidth=4 ts=8: */

/*************************************************************************
 * Copyright (c) 2011 AT&T Intellectual Property 
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors: See CVS logs. Details at http://www.graphviz.org/
 *************************************************************************/

#include <stdio.h>
#include <cghdr.h>

#define MAX(a,b)	((a)>(b)?(a):(b))
static agerrlevel_t agerrno;		/* Last error level */
static agerrlevel_t agerrlevel = AGWARN;	/* Report errors >= agerrlevel */
static int agmaxerr;

static long aglast;		/* Last message */
static FILE *agerrout;		/* Message file */
static agusererrf usererrf;     /* User-set error function */

agusererrf
agseterrf (agusererrf newf)
{
    agusererrf oldf = usererrf;
    usererrf = newf;
    return oldf;
}

agerrlevel_t agseterr(agerrlevel_t lvl)
{
    agerrlevel_t oldv = agerrlevel;
    agerrlevel = lvl;
    return oldv;
}

char *aglasterr()
{
    long endpos;
    long len;
    char *buf;

    if (!agerrout)
	return 0;
    fflush(agerrout);
    endpos = ftell(agerrout);
    len = endpos - aglast;
    buf = (char*)malloc(len + 1);
    fseek(agerrout, aglast, SEEK_SET);
    fread(buf, sizeof(char), len, agerrout);
    buf[len] = '\0';
    fseek(agerrout, endpos, SEEK_SET);

    return buf;
}

/* userout:
 * Report messages using a user-supplied write function 
 */
static void
userout (agerrlevel_t level, const char *fmt, va_list args)
{
    static char* buf;
    static int bufsz = 1024;
    char* np;
    int n;

    if (!buf) {
	buf = (char*)malloc(bufsz);
	if (!buf) {
	    fputs("userout: could not allocate memory\n", stderr );
	    return;
	}
    }

    if (level != AGPREV) {
	usererrf ((level == AGERR) ? "Error" : "Warning");
	usererrf (": ");
    }

    while (1) {
	n = vsnprintf(buf, bufsz, fmt, args);
	if ((n > -1) && (n < bufsz)) {
	    usererrf (buf);
	    break;
	}
	bufsz = MAX(bufsz*2,n+1);
	if ((np = (char*)realloc(buf, bufsz)) == NULL) {
	    fputs("userout: could not allocate memory\n", stderr );
	    return;
	}
    }
    va_end(args);
}

static int agerr_va(agerrlevel_t level, const char *fmt, va_list args)
{
    agerrlevel_t lvl;

    /* Use previous error level if continuation message;
     * Convert AGMAX to AGERROR;
     * else use input level
     */
    lvl = (level == AGPREV ? agerrno : (level == AGMAX) ? AGERR : level);

    /* store this error level */
    agerrno = lvl;
    agmaxerr = MAX(agmaxerr, agerrno);

    /* We report all messages whose level is bigger than the user set agerrlevel
     * Setting agerrlevel to AGMAX turns off immediate error reporting.
     */
    if (lvl >= agerrlevel) {
	if (usererrf)
	    userout (level, fmt, args);
	else {
	    if (level != AGPREV)
		fprintf(stderr, "%s: ", (level == AGERR) ? "Error" : "Warning");
	    vfprintf(stderr, fmt, args);
	    va_end(args);
	}
	return 0;
    }

    if (!agerrout) {
	agerrout = tmpfile();
	if (!agerrout)
	    return 1;
    }

    if (level != AGPREV)
	aglast = ftell(agerrout);
    vfprintf(agerrout, fmt, args);
    va_end(args);
    return 0;
}

int agerr(agerrlevel_t level, const char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    return agerr_va(level, fmt, args);
}

void agerrorf(const char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    agerr_va(AGERR, fmt, args);
}

void agwarningf(const char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    agerr_va(AGWARN, fmt, args);
}

int agerrors() { return agmaxerr; }

int agreseterrors() 
{ 
    int rc = agmaxerr;
    agmaxerr = 0;
    return rc; 
}

