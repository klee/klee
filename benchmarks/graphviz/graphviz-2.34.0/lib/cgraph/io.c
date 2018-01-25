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
#if WIN32
#include <io.h>
#endif

/* experimental ICONV code - probably should be removed - JCE */
#undef HAVE_ICONV

#ifdef HAVE_ICONV
#include <iconv.h>
#include <langinfo.h>
#include <errno.h>
#endif

#ifdef HAVE_ICONV
static int iofreadiconv(void *chan, char *buf, int bufsize)
{
#define CHARBUFSIZE 30
    static char charbuf[CHARBUFSIZE];
    static iconv_t cd = NULL;
    char *inbuf, *outbuf, *readbuf;
    size_t inbytesleft, outbytesleft, readbytesleft, resbytes, result;
    int fd;

    if (!cd) {
	cd = iconv_open(nl_langinfo(CODESET), "UTF-8");
    }
    fd = fileno((FILE *) chan);
    readbuf = inbuf = charbuf;
    readbytesleft = CHARBUFSIZE;
    inbytesleft = 0;
    outbuf = buf;
    outbytesleft = bufsize - 1;
    while (1) {
	if ((result = read(fd, readbuf++, 1)) != 1)
	    break;
	readbytesleft--;
	inbytesleft++;
	result = iconv(cd, &inbuf, &inbytesleft, &outbuf, &outbytesleft);
	if (result != -1) {
	    readbuf = inbuf = charbuf;
	    readbytesleft = CHARBUFSIZE;
	    inbytesleft = 0;
	} else if (errno != EINVAL)
	    break;
    }
    *outbuf = '\0';
    resbytes = bufsize - 1 - outbytesleft;
    if (resbytes)
	result = resbytes;
    return result;
}
#endif

static int iofread(void *chan, char *buf, int bufsize)
{
    if (fgets(buf, bufsize, (FILE*)chan))
	return strlen(buf);
    else
	return 0;
    /* return read(fileno((FILE *) chan), buf, bufsize); */
    /* return fread(buf, 1, bufsize, (FILE*)chan); */
}

/* default IO methods */
static int ioputstr(void *chan, const char *str)
{
    return fputs(str, (FILE *) chan);
}

static int ioflush(void *chan)
{
    return fflush((FILE *) chan);
}

/* Agiodisc_t AgIoDisc = { iofreadiconv, ioputstr, ioflush }; */
Agiodisc_t AgIoDisc = { iofread, ioputstr, ioflush };

typedef struct {
    const char *data;
    int len;
    int cur;
} rdr_t;

static int
memiofread(void *chan, char *buf, int bufsize)
{
    const char *ptr;
    char *optr;
    char c;
    int l;
    rdr_t *s;

    if (bufsize == 0) return 0;
    s = (rdr_t *) chan;
    if (s->cur >= s->len)
        return 0;
    l = 0;
    ptr = s->data + s->cur;
    optr = buf;
    /* We know we have at least one character */
    c = *ptr++;
    do {
        *optr++ = c;
        l++;
	/* continue if c is not newline, we have space in buffer,
	 * and next character is non-null (we are working with
	 * null-terminated strings.
	 */
    } while ((c != '\n') && (l < bufsize) && (c = *ptr++));
    s->cur += l;
    return l;
}

static Agiodisc_t memIoDisc = {memiofread, 0, 0};

Agraph_t *agmemread(const char *cp)
{
    Agraph_t* g;
    rdr_t rdr;
    Agdisc_t disc;

    memIoDisc.putstr = AgIoDisc.putstr;
    memIoDisc.flush = AgIoDisc.flush;
    rdr.data = cp;
    rdr.len = strlen(cp);
    rdr.cur = 0;

    disc.mem = &AgMemDisc;
    disc.id = &AgIdDisc;
    disc.io = &memIoDisc;
    g = agread (&rdr, &disc);
    return g;
}

