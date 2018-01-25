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

/*	Synchronize data in buffers with the file system.
**	If f is nil, all streams are sync-ed
**
**	Written by Kiem-Phong Vo.
*/

#if __STD_C
static int _sfall(void)
#else
static int _sfall()
#endif
{
    reg Sfpool_t *p, *next;
    reg Sfio_t *f;
    reg int n, rv;
    reg int nsync, count, loop;
#define MAXLOOP 3

    for (loop = 0; loop < MAXLOOP; ++loop) {
	rv = nsync = count = 0;
	for (p = &_Sfpool; p; p = next) {	/* find the next legitimate pool */
	    for (next = p->next; next; next = next->next)
		if (next->n_sf > 0)
		    break;

	    /* walk the streams for _Sfpool only */
	    for (n = 0; n < ((p == &_Sfpool) ? p->n_sf : 1); ++n) {
		count += 1;
		f = p->sf[n];

		if (f->flags & SF_STRING)
		    goto did_sync;
		if (SFFROZEN(f))
		    continue;
		if ((f->mode & SF_READ) && (f->mode & SF_SYNCED))
		    goto did_sync;
		if ((f->mode & SF_READ) && !(f->bits & SF_MMAP) &&
		    f->next == f->endb)
		    goto did_sync;
		if ((f->mode & SF_WRITE) && !(f->bits & SF_HOLE) &&
		    f->next == f->data)
		    goto did_sync;

		if (sfsync(f) < 0)
		    rv = -1;

	      did_sync:
		nsync += 1;
	    }
	}

	if (nsync == count)
	    break;
    }
    return rv;
}

#if __STD_C
int sfsync(reg Sfio_t * f)
#else
int sfsync(f)
reg Sfio_t *f;			/* stream to be synchronized */
#endif
{
    int local, rv, mode;
    Sfio_t *origf;

    if (!(origf = f))
	return _sfall();

    SFMTXSTART(origf, -1);

    GETLOCAL(origf, local);

    if (origf->disc == _Sfudisc)	/* throw away ungetc */
	(void) sfclose((*_Sfstack) (origf, NIL(Sfio_t *)));

    rv = 0;

    if ((origf->mode & SF_RDWR) != SFMODE(origf, local)
	&& _sfmode(origf, 0, local) < 0) {
	rv = -1;
	goto done;
    }

    for (; f; f = f->push) {
	if ((f->flags & SF_IOCHECK) && f->disc && f->disc->exceptf)
	    (void) (*f->disc->exceptf) (f, SF_SYNC, (Void_t *) ((int) 1),
					f->disc);

	SFLOCK(f, local);

	/* pretend that this stream is not on a stack */
	mode = f->mode & SF_PUSH;
	f->mode &= ~SF_PUSH;

	/* these streams do not need synchronization */
	if ((f->flags & SF_STRING) || (f->mode & SF_SYNCED))
	    goto next;

	if ((f->mode & SF_WRITE) && (f->next > f->data || (f->bits & SF_HOLE))) {	/* sync the buffer, make sure pool don't move */
	    reg int pool = f->mode & SF_POOL;
	    f->mode &= ~SF_POOL;
	    if (f->next > f->data && (SFWRALL(f), SFFLSBUF(f, -1)) < 0)
		rv = -1;
	    if (!SFISNULL(f) && (f->bits & SF_HOLE)) {	/* realize a previously created hole of 0's */
		if (SFSK(f, (Sfoff_t) (-1), SEEK_CUR, f->disc) >= 0)
		    (void) SFWR(f, "", 1, f->disc);
		f->bits &= ~SF_HOLE;
	    }
	    f->mode |= pool;
	}

	if ((f->mode & SF_READ) && f->extent >= 0 && ((f->bits & SF_MMAP) || f->next < f->endb)) {	/* make sure the file pointer is at the right place */
	    f->here -= (f->endb - f->next);
	    f->endr = f->endw = f->data;
	    f->mode = SF_READ | SF_SYNCED | SF_LOCK;
	    (void) SFSK(f, f->here, SEEK_SET, f->disc);

	    if ((f->flags & SF_SHARE) && !(f->flags & SF_PUBLIC) &&
		!(f->bits & SF_MMAP)) {
		f->endb = f->next = f->data;
		f->mode &= ~SF_SYNCED;
	    }
	}

      next:
	f->mode |= mode;
	SFOPEN(f, local);

	if ((f->flags & SF_IOCHECK) && f->disc && f->disc->exceptf)
	    (void) (*f->disc->exceptf) (f, SF_SYNC, (Void_t *) ((int) 0),
					f->disc);
    }

  done:
    if (!local && f && (f->mode & SF_POOL) && f->pool
	&& f != f->pool->sf[0])
	SFSYNC(f->pool->sf[0]);

    SFMTXRETURN(origf, rv);
}
