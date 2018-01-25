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

/*	Invoke event handlers for a stream
**
**	Written by Kiem-Phong Vo.
*/
#if __STD_C
int sfraise(Sfio_t * f, int type, Void_t * data)
#else
int sfraise(f, type, data)
Sfio_t *f;			/* stream               */
int type;			/* type of event        */
Void_t *data;			/* associated data      */
#endif
{
    reg Sfdisc_t *disc, *next, *d;
    reg int local, rv;

    SFMTXSTART(f, -1);

    GETLOCAL(f, local);
    if (!SFKILLED(f) &&
	!(local &&
	  (type == SF_NEW || type == SF_CLOSING ||
	   type == SF_FINAL || type == SF_ATEXIT)) &&
	SFMODE(f, local) != (f->mode & SF_RDWR)
	&& _sfmode(f, 0, local) < 0)
	SFMTXRETURN(f, -1);
    SFLOCK(f, local);

    for (disc = f->disc; disc;) {
	next = disc->disc;

	if (disc->exceptf) {
	    SFOPEN(f, 0);
	    if ((rv = (*disc->exceptf) (f, type, data, disc)) != 0)
		SFMTXRETURN(f, rv);
	    SFLOCK(f, 0);
	}

	if ((disc = next)) {	/* make sure that "next" hasn't been popped */
	    for (d = f->disc; d; d = d->disc)
		if (d == disc)
		    break;
	    if (!d)
		disc = f->disc;
	}
    }

    SFOPEN(f, local);
    SFMTXRETURN(f, 0);
}
