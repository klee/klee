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

#include	"sfhdr.h"

/*	Write out an unsigned long value in a portable format.
**
**	Written by Kiem-Phong Vo.
*/

#if __STD_C
int _sfputm(reg Sfio_t * f, Sfulong_t v, Sfulong_t max)
#else
int _sfputm(f, v, max)
reg Sfio_t *f;			/* write a portable ulong to this stream */
Sfulong_t v;			/* the unsigned value to be written */
Sfulong_t max;			/* the max value of the range */
#endif
{
#define N_ARRAY		(2*sizeof(Sfulong_t))
    reg uchar *s, *ps;
    reg ssize_t n, p;
    uchar c[N_ARRAY];

    SFMTXSTART(f, -1);

    if (v > max || (f->mode != SF_WRITE && _sfmode(f, SF_WRITE, 0) < 0))
	SFMTXRETURN(f, -1);
    SFLOCK(f, 0);

    /* code v as integers in base SF_UBASE */
    s = ps = &(c[N_ARRAY - 1]);
    *s = (uchar) SFBVALUE(v);
    while ((max >>= SF_BBITS) > 0) {
	v >>= SF_BBITS;
	*--s = (uchar) SFBVALUE(v);
    }
    n = (ps - s) + 1;

    if (n > 8 || SFWPEEK(f, ps, p) < n)
	n = SFWRITE(f, (Void_t *) s, n);	/* write the hard way */
    else {
	switch (n) {
	case 8:
	    *ps++ = *s++;
	case 7:
	    *ps++ = *s++;
	case 6:
	    *ps++ = *s++;
	case 5:
	    *ps++ = *s++;
	case 4:
	    *ps++ = *s++;
	case 3:
	    *ps++ = *s++;
	case 2:
	    *ps++ = *s++;
	case 1:
	    *ps++ = *s++;
	}
	f->next = ps;
    }

    SFOPEN(f, 0);
    SFMTXRETURN(f, (int) n);
}
