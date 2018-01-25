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

#include	"vmhdr.h"

/*	Turn on tracing for regions
**
**	Written by Kiem-Phong Vo, kpv@research.att.com, 01/16/94.
*/

static int Trfile = -1;
static char Trbuf[128];

#if __STD_C
static char *trstrcpy(char *to, char *from, int endc)
#else
static char *trstrcpy(to, from, endc)
char *to;
char *from;
int endc;
#endif
{
    reg int n;

    n = strlen(from);
    memcpy(to, from, n);
    to += n;
    if ((*to = endc))
	to += 1;
    return to;
}

/* convert a long value to an ascii representation */
#if __STD_C
static char *tritoa(Vmulong_t v, int type)
#else
static char *tritoa(v, type)
Vmulong_t v;			/* value to convert                                     */
int type;			/* =0 base-16, >0: unsigned base-10, <0: signed base-10 */
#endif
{
    char *s;

    s = &Trbuf[sizeof(Trbuf) - 1];
    *s-- = '\0';

    if (type == 0) {		/* base-16 */
	reg char *digit = "0123456789abcdef";
	do {
	    *s-- = digit[v & 0xf];
	    v >>= 4;
	} while (v);
	*s-- = 'x';
	*s-- = '0';
    } else if (type > 0) {	/* unsigned base-10 */
	do {
	    *s-- = (char) ('0' + (v % 10));
	    v /= 10;
	} while (v);
    } else {			/* signed base-10 */
	int sign = ((long) v < 0);
	if (sign)
	    v = (Vmulong_t) (-((long) v));
	do {
	    *s-- = (char) ('0' + (v % 10));
	    v /= 10;
	} while (v);
	if (sign)
	    *s-- = '-';
    }

    return s + 1;
}

/* generate a trace of some call */
#if __STD_C
static void trtrace(Vmalloc_t * vm,
		    Vmuchar_t * oldaddr, Vmuchar_t * newaddr, size_t size,
		    size_t align)
#else
static void trtrace(vm, oldaddr, newaddr, size, align)
Vmalloc_t *vm;			/* region call was made from    */
Vmuchar_t *oldaddr;		/* old data address             */
Vmuchar_t *newaddr;		/* new data address             */
size_t size;			/* size of piece                */
size_t align;			/* alignment                    */
#endif
{
    char buf[1024], *bufp, *endbuf;
    reg Vmdata_t *vd = vm->data;
    reg char *file = NIL(char *);
    reg int line = 0;
    int type;
#define SLOP	32

    if (oldaddr == (Vmuchar_t *) (-1)) {	/* printing busy blocks */
	type = 0;
	oldaddr = NIL(Vmuchar_t *);
    } else {
	type = vd->mode & VM_METHODS;
	VMFILELINE(vm, file, line);
    }

    if (Trfile < 0)
	return;

    bufp = buf;
    endbuf = buf + sizeof(buf);
    bufp = trstrcpy(bufp, tritoa(oldaddr ? VLONG(oldaddr) : 0L, 0), ':');
    bufp = trstrcpy(bufp, tritoa(newaddr ? VLONG(newaddr) : 0L, 0), ':');
    bufp = trstrcpy(bufp, tritoa((Vmulong_t) size, 1), ':');
    bufp = trstrcpy(bufp, tritoa((Vmulong_t) align, 1), ':');
    bufp = trstrcpy(bufp, tritoa(VLONG(vm), 0), ':');
    if (type & VM_MTBEST)
	bufp = trstrcpy(bufp, "best", ':');
    else if (type & VM_MTLAST)
	bufp = trstrcpy(bufp, "last", ':');
    else if (type & VM_MTPOOL)
	bufp = trstrcpy(bufp, "pool", ':');
    else if (type & VM_MTPROFILE)
	bufp = trstrcpy(bufp, "profile", ':');
    else if (type & VM_MTDEBUG)
	bufp = trstrcpy(bufp, "debug", ':');
    else
	bufp = trstrcpy(bufp, "busy", ':');
    if (file && file[0] && line > 0
	&& (bufp + strlen(file) + SLOP) < endbuf) {
	bufp = trstrcpy(bufp, file, ',');
	bufp = trstrcpy(bufp, tritoa((Vmulong_t) line, 1), ':');
    }
    *bufp++ = '\n';
    *bufp = '\0';

    write(Trfile, buf, (bufp - buf));
}

#if __STD_C
int vmtrace(int file)
#else
int vmtrace(file)
int file;
#endif
{
    int fd;

    _Vmstrcpy = trstrcpy;
    _Vmitoa = tritoa;
    _Vmtrace = trtrace;

    fd = Trfile;
    Trfile = file;
    return fd;
}

#if __STD_C
int vmtrbusy(Vmalloc_t * vm)
#else
int vmtrbusy(vm)
Vmalloc_t *vm;
#endif
{
    Seg_t *seg;
    Vmdata_t *vd = vm->data;

    if (Trfile < 0
	|| !(vd->mode & (VM_MTBEST | VM_MTDEBUG | VM_MTPROFILE)))
	return -1;

    for (seg = vd->seg; seg; seg = seg->next) {
	Block_t *b, *endb;
	Vmuchar_t *data;
	size_t s;

	for (b = SEGBLOCK(seg), endb = BLOCK(seg->baddr); b < endb;) {
	    if (ISJUNK(SIZE(b)) || !ISBUSY(SIZE(b)))
		continue;

	    data = DATA(b);
	    if (vd->mode & VM_MTDEBUG) {
		data = DB2DEBUG(data);
		s = DBSIZE(data);
	    } else if (vd->mode & VM_MTPROFILE)
		s = PFSIZE(data);
	    else
		s = SIZE(b) & ~BITS;

	    trtrace(vm, (Vmuchar_t *) (-1), data, s, 0);

	    b = (Block_t *) ((Vmuchar_t *) DATA(b) + (SIZE(b) & ~BITS));
	}
    }

    return 0;
}
