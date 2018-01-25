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

/*	Reserve a segment of data or buffer.
**
**	Written by Kiem-Phong Vo.
*/

#if __STD_C
Void_t *sfreserve(reg Sfio_t * f, ssize_t size, int type)
#else
Void_t *sfreserve(f, size, type)
reg Sfio_t *f;			/* file to peek */
ssize_t size;			/* size of peek */
int type;			/* LOCKR: lock stream, LASTR: last record */
#endif
{
    reg ssize_t n, sz;
    reg Sfrsrv_t *rsrv;
    reg Void_t *data;
    reg int mode;

    SFMTXSTART(f, NIL(Void_t *));

    /* initialize io states */
    rsrv = NIL(Sfrsrv_t *);
    _Sfi = f->val = -1;

    /* return the last record */
    if (type == SF_LASTR) {
	if ((rsrv = f->rsrv) && (n = -rsrv->slen) > 0) {
	    rsrv->slen = 0;
	    _Sfi = f->val = n;
	    SFMTXRETURN(f, (Void_t *) rsrv->data);
	} else
	    SFMTXRETURN(f, NIL(Void_t *));
    }

    if (type > 0 && !(type == SF_LOCKR || type == 1))
	SFMTXRETURN(f, NIL(Void_t *));

    if ((sz = size) == 0 && type != 0) {	/* only return the current status and possibly lock stream */
	if ((f->mode & SF_RDWR) != f->mode && _sfmode(f, 0, 0) < 0)
	    SFMTXRETURN(f, NIL(Void_t *));

	SFLOCK(f, 0);
	if ((n = f->endb - f->next) < 0)
	    n = 0;

	if (!f->data && type > 0)
	    rsrv = _sfrsrv(f, 0);

	goto done;
    }
    if (sz < 0)
	sz = -sz;

    /* iterate until get to a stream that has data or buffer space */
    for (;;) {			/* prefer read mode so that data is always valid */
	if (!(mode = (f->flags & SF_READ)))
	    mode = SF_WRITE;
	if ((int) f->mode != mode && _sfmode(f, mode, 0) < 0) {
	    n = -1;
	    goto done;
	}

	SFLOCK(f, 0);

	if ((n = f->endb - f->next) < 0)	/* possible for string+rw */
	    n = 0;

	if (n > 0 && n >= sz)	/* all done */
	    break;

	/* do a buffer refill or flush */
	if (f->mode & SF_WRITE)
	    (void) SFFLSBUF(f, -1);
	else if (type > 0 && f->extent < 0 && (f->flags & SF_SHARE)) {
	    if (n == 0) {	/* peek-read only if there is no buffered data */
		f->mode |= SF_RV;
		(void) SFFILBUF(f, sz == 0 ? -1 : (sz - n));
	    }
	    if ((n = f->endb - f->next) < sz) {
		if (f->mode & SF_PKRD) {
		    f->endb = f->endr = f->next;
		    f->mode &= ~SF_PKRD;
		}
		goto done;
	    }
	} else
	    (void) SFFILBUF(f, sz == 0 ? -1 : (sz - n));

	/* now have data */
	if ((n = f->endb - f->next) > 0)
	    break;
	else if (n < 0)
	    n = 0;

	/* this test fails only if unstacked to an opposite stream */
	if ((f->mode & mode) != 0)
	    break;
    }

    if (n > 0 && n < sz && (f->mode & mode) != 0) {	/* try to accomodate request size */
	if (f->flags & SF_STRING) {
	    if ((f->mode & SF_WRITE) && (f->flags & SF_MALLOC)) {
		(void) SFWR(f, f->next, sz, f->disc);
		n = f->endb - f->next;
	    }
	} else if (f->mode & SF_WRITE) {
	    if (type > 0 && (rsrv = _sfrsrv(f, sz)))
		n = sz;
	} else {		/*if(f->mode&SF_READ) */
	    if (type <= 0 && (rsrv = _sfrsrv(f, sz)) &&
		(n = SFREAD(f, (Void_t *) rsrv->data, sz)) < sz)
		rsrv->slen = -n;
	}
    }

  done:
    /* return true buffer size */
    _Sfi = f->val = n;

    SFOPEN(f, 0);

    if ((sz > 0 && n < sz) || (n == 0 && type <= 0))
	SFMTXRETURN(f, NIL(Void_t *));

    if ((data = rsrv ? (Void_t *) rsrv->data : (Void_t *) f->next)) {
	if (type > 0) {
	    f->mode |= SF_PEEK;
	    f->endr = f->endw = f->data;
	} else if (data == (Void_t *) f->next)
	    f->next += (size >= 0 ? size : n);
    }

    SFMTXRETURN(f, data);
}
