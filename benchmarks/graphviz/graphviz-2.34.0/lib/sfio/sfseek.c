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

/*	Set the IO pointer to a specific location in the stream
**
**	Written by Kiem-Phong Vo.
*/

#if __STD_C
static void newpos(Sfio_t * f, Sfoff_t p)
#else
static void newpos(f, p)
Sfio_t *f;
Sfoff_t p;
#endif
{
#ifdef MAP_TYPE
    if ((f->bits & SF_MMAP) && f->data) {
	SFMUNMAP(f, f->data, f->endb - f->data);
	f->data = NIL(uchar *);
    }
#endif
    f->next = f->endr = f->endw = f->data;
    f->endb = (f->mode & SF_WRITE) ? f->data + f->size : f->data;
    if ((f->here = p) < 0) {
	f->extent = -1;
	f->here = 0;
    }
}

#if __STD_C
Sfoff_t sfseek(Sfio_t * f, Sfoff_t p, int type)
#else
Sfoff_t sfseek(f, p, type)
Sfio_t *f;			/* seek to a new location in this stream */
Sfoff_t p;			/* place to seek to */
int type;			/* 0: from org, 1: from here, 2: from end */
#endif
{
    Sfoff_t r, s;
    size_t a, b, c;
    int mode, local, hardseek, mustsync;

    SFMTXSTART(f, (Sfoff_t) (-1));

    GETLOCAL(f, local);

    hardseek = (type | f->flags) & (SF_SHARE | SF_PUBLIC);

    if (hardseek && f->mode == (SF_READ | SF_SYNCED)) {
	newpos(f, f->here);
	f->mode = SF_READ;
    }

    /* set and initialize the stream to a definite mode */
    if ((int) SFMODE(f, local) != (mode = f->mode & SF_RDWR)) {
	int flags = f->flags;

	if (hardseek & SF_PUBLIC)	/* seek ptr must follow file descriptor */
	    f->flags |= SF_SHARE | SF_PUBLIC;
	mode = _sfmode(f, mode, local);
	if (hardseek & SF_PUBLIC)
	    f->flags = flags;

	if (mode < 0)
	    SFMTXRETURN(f, (Sfoff_t) (-1));
    }

    mustsync = (type & SF_SHARE) && !(type & SF_PUBLIC) &&
	(f->mode & SF_READ) && !(f->flags & SF_STRING);

    /* Xopen-compliant */
    if ((type &= (SEEK_SET | SEEK_CUR | SEEK_END)) != SEEK_SET &&
	type != SEEK_CUR && type != SEEK_END) {
	errno = EINVAL;
	SFMTXRETURN(f, (Sfoff_t) (-1));
    }

    if (f->extent < 0) {	/* let system call set errno */
	(void) SFSK(f, (Sfoff_t) 0, SEEK_CUR, f->disc);
	SFMTXRETURN(f, (Sfoff_t) (-1));
    }

    /* throw away ungetc data */
    if (f->disc == _Sfudisc)
	(void) sfclose((*_Sfstack) (f, NIL(Sfio_t *)));

    /* lock the stream for internal manipulations */
    SFLOCK(f, local);

    /* clear error and eof bits */
    f->flags &= ~(SF_EOF | SF_ERROR);

    while (f->flags & SF_STRING) {
	SFSTRSIZE(f);

	if (type == SEEK_CUR)
	    r = p + (f->next - f->data);
	else if (type == SEEK_END)
	    r = p + f->extent;
	else
	    r = p;

	if (r >= 0 && r <= f->size) {
	    p = r;
	    f->next = f->data + p;
	    f->here = p;
	    if (p > f->extent)
		memclear((char *) (f->data + f->extent),
			 (int) (p - f->extent));
	    goto done;
	}

	/* check exception handler, note that this may pop stream */
	if (SFSK(f, r, SEEK_SET, f->disc) != r) {
	    p = -1;
	    goto done;
	} else if (!(f->flags & SF_STRING)) {
	    p = r;
	    goto done;
	}
    }

    if (f->mode & SF_WRITE) {	/* see if we can avoid flushing buffer */
	if (!hardseek && type < SEEK_END && !(f->flags & SF_APPENDWR)) {
	    s = f->here + (f->next - f->data);
	    r = p + (type == SEEK_SET ? 0 : s);
	    if (r == s) {
		p = r;
		goto done;
	    }
	}

	if (f->next > f->data && SFSYNC(f) < 0) {
	    p = -1;
	    goto done;
	}
    }

    if (type == SEEK_END || (f->mode & SF_WRITE)) {
	if ((hardseek & SF_PUBLIC) || type == SEEK_END)
	    p = SFSK(f, p, type, f->disc);
	else {
	    r = p + (type == SEEK_CUR ? f->here : 0);
	    p = (hardseek
		 || r != f->here) ? SFSK(f, r, SEEK_SET, f->disc) : r;
	}
	if (p >= 0)
	    newpos(f, p);

	goto done;
    }

    /* if get here, must be a read stream */
    s = f->here - (f->endb - f->next);
    r = p + (type == SEEK_CUR ? s : 0);
    if (r <= f->here && r >= (f->here - (f->endb - f->data))) {
	if ((hardseek || (type == SEEK_CUR && p == 0))) {
	    if ((s = SFSK(f, (Sfoff_t) 0, SEEK_CUR, f->disc)) == f->here ||
		(s >= 0 && !(hardseek & SF_PUBLIC) &&
		 (s = SFSK(f, f->here, SEEK_SET, f->disc)) == f->here))
		goto near_done;
	    else if (s < 0) {
		p = -1;
		goto done;
	    } else {
		newpos(f, s);
		hardseek = 0;
	    }
	} else {
	  near_done:
	    f->next = f->endb - (f->here - r);
	    p = r;
	    goto done;
	}
    }

    /* desired position */
    if ((p += type == SEEK_CUR ? s : 0) < 0)
	goto done;

#ifdef MAP_TYPE
    if (f->bits & SF_MMAP) {	/* if mmap is not great, stop mmaping if moving around too much */
#if _mmap_worthy < 2
	if ((f->next - f->data) < ((f->endb - f->data) / 4)) {
	    SFSETBUF(f, (Void_t *) f->tiny, (size_t) SF_UNBOUND);
	    hardseek = 1;	/* this forces a hard seek below */
	} else
#endif
	{			/* for mmap, f->here can be virtual */
	    newpos(f, p);
	    goto done;
	}
    }
#endif

    b = f->endb - f->data;	/* amount of buffered data */
    c = f->next - f->data;	/* amount of data consumed */

    if (b > 0) {		/* gradually reduce wastage */
	if (b <= SF_GRAIN)
	    f->iosz = SF_GRAIN;
	else {
	    c *= 2;
	    a = c + 3 * (b - c) / 4;
	    a = ((a + SF_GRAIN - 1) / SF_GRAIN) * SF_GRAIN;

	    b = ((b + SF_GRAIN - 1) / SF_GRAIN) * SF_GRAIN;

	    f->iosz = a < b ? a : c < b / 2 ? b / 2 : b;
	}
    }
    /* else, believe previous setting of f->iosz */

    if (f->iosz >= f->size)
	f->iosz = 0;

    /* buffer is now considered empty */
    f->next = f->endr = f->endb = f->data;

    /* small backseeks often come in bunches, so seek back as far as possible */
    if (p < f->lpos && f->size > SF_GRAIN && (p + SF_GRAIN) > s) {
	if ((r = s - f->size) < 0)
	    r = 0;
    } else {
	r = p;

	/* seeking around and wasting data, be conservative */
	if (f->iosz > 0 && (p > f->lpos || p < f->lpos - f->size))
	    f->bits |= SF_JUSTSEEK;
    }

    if ((hardseek || r != f->here)
	&& (f->here = SFSK(f, r, SEEK_SET, f->disc)) != r) {
	if (r < p)		/* now try to just get to p */
	    f->here = SFSK(f, p, SEEK_SET, f->disc);
	if (f->here != p)
	    p = -1;
	goto done;
    }

    if (r < p) {		/* read to cover p */
	(void) SFRD(f, f->data, f->size, f->disc);
	if (p <= f->here && p >= (f->here - (f->endb - f->data)))
	    f->next = f->endb - (size_t) (f->here - p);
	else {			/* recover from read failure by just seeking to p */
	    f->next = f->endb = f->data;
	    if ((f->here = SFSK(f, p, SEEK_SET, f->disc)) != p)
		p = -1;
	}
    }

  done:
    if (f->here < 0) {		/* hasn't been the best of time */
	f->extent = -1;
	f->here = 0;
    }

    f->lpos = p;

    SFOPEN(f, local);

    if (mustsync)
	sfsync(f);
    SFMTXRETURN(f, p);
}
