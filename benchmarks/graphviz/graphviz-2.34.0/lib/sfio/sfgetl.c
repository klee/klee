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

/*	Read a long value coded in a portable format.
**
**	Written by Kiem-Phong Vo
*/

#if __STD_C
Sflong_t sfgetl(reg Sfio_t * f)
#else
Sflong_t sfgetl(f)
reg Sfio_t *f;
#endif
{
    Sflong_t v;
    reg uchar *s, *ends, c;
    reg int p;

    SFMTXSTART(f, (Sflong_t) (-1));

    if (f->mode != SF_READ && _sfmode(f, SF_READ, 0) < 0)
	SFMTXRETURN(f, (Sflong_t) (-1));
    SFLOCK(f, 0);

    for (v = 0;;) {
	if (SFRPEEK(f, s, p) <= 0) {
	    f->flags |= SF_ERROR;
	    v = (Sflong_t) (-1);
	    goto done;
	}
	for (ends = s + p; s < ends;) {
	    c = *s++;
	    if (c & SF_MORE)
		v = ((Sfulong_t) v << SF_UBITS) | SFUVALUE(c);
	    else {		/* special translation for this byte */
		v = ((Sfulong_t) v << SF_SBITS) | SFSVALUE(c);
		f->next = s;
		v = (c & SF_SIGN) ? -v - 1 : v;
		goto done;
	    }
	}
	f->next = s;
    }
  done:
    SFOPEN(f, 0);
    SFMTXRETURN(f, v);
}
