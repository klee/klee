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

/*	Write out a floating point value in a portable format
**
**	Written by Kiem-Phong Vo.
*/

#if __STD_C
int _sfputd(Sfio_t * f, Sfdouble_t v)
#else
int _sfputd(f, v)
Sfio_t *f;
Sfdouble_t v;
#endif
{
#define N_ARRAY		(16*sizeof(Sfdouble_t))
    reg ssize_t n, w;
    reg uchar *s, *ends;
    int exp;
    uchar c[N_ARRAY];
    double x;

    SFMTXSTART(f, -1);

    if (f->mode != SF_WRITE && _sfmode(f, SF_WRITE, 0) < 0)
	SFMTXRETURN(f, -1);
    SFLOCK(f, 0);

    /* get the sign of v */
    if (v < 0.) {
	v = -v;
	n = 1;
    } else
	n = 0;

#if !_ast_fltmax_double		/* don't know how to do these yet */
    if (v > SF_MAXDOUBLE && !_has_expfuncs) {
	SFOPEN(f, 0);
	SFMTXRETURN(f, -1);
    }
#endif

    /* make the magnitude of v < 1 */
    if (v != 0.)
	v = frexp(v, &exp);
    else
	exp = 0;

    /* code the sign of v and exp */
    if ((w = exp) < 0) {
	n |= 02;
	w = -w;
    }

    /* write out the signs and the exp */
    SFOPEN(f, 0);
    if (sfputc(f, n) < 0 || (w = sfputu(f, w)) < 0)
	SFMTXRETURN(f, -1);
    SFLOCK(f, 0);
    w += 1;

    s = (ends = &c[0]) + sizeof(c);
    while (s > ends) {		/* get 2^SF_PRECIS precision at a time */
	n = (int) (x = ldexp(v, SF_PRECIS));
	*--s = n | SF_MORE;
	v = x - n;
	if (v <= 0.)
	    break;
    }

    /* last byte is not SF_MORE */
    ends = &c[0] + sizeof(c) - 1;
    *ends &= ~SF_MORE;

    /* write out coded bytes */
    n = ends - s + 1;
    w = SFWRITE(f, (Void_t *) s, n) == n ? w + n : -1;

    SFOPEN(f, 0);
    SFMTXRETURN(f, w);
}
