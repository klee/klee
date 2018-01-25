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

/*	Management of pools of streams.
**	If pf is not nil, f is pooled with pf and f becomes current;
**	otherwise, f is isolated from its pool. flag can be one of
**	0 or SF_SHARE.
**
**	Written by Kiem-Phong Vo.
*/

/* Note that we do not free the space for a pool once it is allocated.
** This is to prevent memory faults in calls such as sfsync(NULL) that walk the pool
** link list and during such walks may free up streams&pools. Free pools will be
** reused in newpool().
*/
#if __STD_C
static int delpool(reg Sfpool_t * p)
#else
static int delpool(p)
reg Sfpool_t *p;
#endif
{
    POOLMTXSTART(p);

    if (p->s_sf && p->sf != p->array)
	free((Void_t *) p->sf);
    p->mode = SF_AVAIL;

    POOLMTXRETURN(p, 0);
}

#if __STD_C
static Sfpool_t *newpool(reg int mode)
#else
static Sfpool_t *newpool(mode)
reg int mode;
#endif
{
    reg Sfpool_t *p, *last = &_Sfpool;

    /* look to see if there is a free pool */
    for (last = &_Sfpool, p = last->next; p; last = p, p = p->next) {
	if (p->mode == SF_AVAIL) {
	    p->mode = 0;
	    break;
	}
    }

    if (!p) {
	POOLMTXLOCK(last);

	if (!(p = (Sfpool_t *) malloc(sizeof(Sfpool_t)))) {
	    POOLMTXUNLOCK(last);
	    return NIL(Sfpool_t *);
	}

	vtmtxopen(&p->mutex, VT_INIT);	/* initialize mutex */

	p->mode = 0;
	p->n_sf = 0;
	p->next = NIL(Sfpool_t *);
	last->next = p;

	POOLMTXUNLOCK(last);
    }

    POOLMTXSTART(p);

    p->mode = mode & SF_SHARE;
    p->s_sf = sizeof(p->array) / sizeof(p->array[0]);
    p->sf = p->array;

    POOLMTXRETURN(p, p);
}

/* move a stream to head */
#if __STD_C
static int _sfphead(Sfpool_t * p, Sfio_t * f, int n)
#else
static int _sfphead(p, f, n)
Sfpool_t *p;			/* the pool                     */
Sfio_t *f;			/* the stream                   */
int n;				/* current position in pool     */
#endif
{
    reg Sfio_t *head;
    reg ssize_t k, w, v;
    reg int rv;

    POOLMTXSTART(p);

    if (n == 0)
	POOLMTXRETURN(p, 0);

    head = p->sf[0];
    if (SFFROZEN(head))
	POOLMTXRETURN(p, -1);

    SFLOCK(head, 0);
    rv = -1;

    if (!(p->mode & SF_SHARE)) {
	if (SFSYNC(head) < 0)
	    goto done;
    } else {			/* shared pool, data can be moved among streams */
	if (SFMODE(head, 1) != SF_WRITE && _sfmode(head, SF_WRITE, 1) < 0)
	    goto done;
	 /**/ ASSERT((f->mode & (SF_WRITE | SF_POOL)) ==
		     (SF_WRITE | SF_POOL));
	 /**/ ASSERT(f->next == f->data);

	v = head->next - head->data;	/* pending data         */
	if ((k = v - (f->endb - f->data)) <= 0)
	    k = 0;
	else {			/* try to write out amount exceeding f's capacity */
	    if ((w = SFWR(head, head->data, k, head->disc)) == k)
		v -= k;
	    else {		/* write failed, recover buffer then quit */
		if (w > 0) {
		    v -= w;
		    memcpy(head->data, (head->data + w), v);
		}
		head->next = head->data + v;
		goto done;
	    }
	}

	/* move data from head to f */
	if ((head->data + k) != f->data)
	    memcpy(f->data, (head->data + k), v);
	f->next = f->data + v;
    }

    f->mode &= ~SF_POOL;
    head->mode |= SF_POOL;
    head->next = head->endr = head->endw = head->data;

    p->sf[n] = head;
    p->sf[0] = f;
    rv = 0;

  done:
    head->mode &= ~SF_LOCK;	/* partially unlock because it's no longer head */

    POOLMTXRETURN(p, rv);
}

/* delete a stream from its pool */
#if __STD_C
static int _sfpdelete(Sfpool_t * p, Sfio_t * f, int n)
#else
static int _sfpdelete(p, f, n)
Sfpool_t *p;			/* the pool             */
Sfio_t *f;			/* the stream           */
int n;				/* position in pool     */
#endif
{
    POOLMTXSTART(p);

    p->n_sf -= 1;
    for (; n < p->n_sf; ++n)
	p->sf[n] = p->sf[n + 1];

    f->pool = NIL(Sfpool_t *);
    f->mode &= ~SF_POOL;

    if (p->n_sf == 0 || p == &_Sfpool) {
	if (p != &_Sfpool)
	    delpool(p);
	goto done;
    }

    /* !_Sfpool, make sure head stream is an open stream */
    for (n = 0; n < p->n_sf; ++n)
	if (!SFFROZEN(p->sf[n]))
	    break;
    if (n < p->n_sf && n > 0) {
	f = p->sf[n];
	p->sf[n] = p->sf[0];
	p->sf[0] = f;
    }

    /* head stream has SF_POOL off */
    f = p->sf[0];
    f->mode &= ~SF_POOL;
    if (!SFFROZEN(f))
	_SFOPEN(f);

    /* if only one stream left, delete pool */
    if (p->n_sf == 1) {
	_sfpdelete(p, f, 0);
	_sfsetpool(f);
    }

  done:
    POOLMTXRETURN(p, 0);
}

#if __STD_C
static int _sfpmove(reg Sfio_t * f, reg int type)
#else
static int _sfpmove(f, type)
reg Sfio_t *f;
reg int type;			/* <0 : deleting, 0: move-to-front, >0: inserting */
#endif
{
    reg Sfpool_t *p;
    reg int n;

    if (type > 0)
	return _sfsetpool(f);
    else {
	if (!(p = f->pool))
	    return -1;
	for (n = p->n_sf - 1; n >= 0; --n)
	    if (p->sf[n] == f)
		break;
	if (n < 0)
	    return -1;

	return type == 0 ? _sfphead(p, f, n) : _sfpdelete(p, f, n);
    }
}

#if __STD_C
Sfio_t *sfpool(reg Sfio_t * f, reg Sfio_t * pf, reg int mode)
#else
Sfio_t *sfpool(f, pf, mode)
reg Sfio_t *f;
reg Sfio_t *pf;
reg int mode;
#endif
{
    reg Sfpool_t *p;
    reg Sfio_t *rv;

    _Sfpmove = _sfpmove;

    if (!f) {			/* return head of pool of pf regardless of lock states */
	if (!pf)
	    return NIL(Sfio_t *);
	else if (!pf->pool || pf->pool == &_Sfpool)
	    return pf;
	else
	    return pf->pool->sf[0];
    }

    if (f) {			/* check for permissions */
	SFMTXLOCK(f);
	if ((f->mode & SF_RDWR) != f->mode && _sfmode(f, 0, 0) < 0) {
	    SFMTXUNLOCK(f);
	    return NIL(Sfio_t *);
	}
	if (f->disc == _Sfudisc)
	    (void) sfclose((*_Sfstack) (f, NIL(Sfio_t *)));
    }
    if (pf) {
	SFMTXLOCK(pf);
	if ((pf->mode & SF_RDWR) != pf->mode && _sfmode(pf, 0, 0) < 0) {
	    if (f)
		SFMTXUNLOCK(f);
	    SFMTXUNLOCK(pf);
	    return NIL(Sfio_t *);
	}
	if (pf->disc == _Sfudisc)
	    (void) sfclose((*_Sfstack) (pf, NIL(Sfio_t *)));
    }

    /* f already in the same pool with pf */
    if (f == pf || (pf && f->pool == pf->pool && f->pool != &_Sfpool)) {
	if (f)
	    SFMTXUNLOCK(f);
	if (pf)
	    SFMTXUNLOCK(pf);
	return pf;
    }

    /* lock streams before internal manipulations */
    rv = NIL(Sfio_t *);
    SFLOCK(f, 0);
    if (pf)
	SFLOCK(pf, 0);

    if (!pf) {			/* deleting f from its current pool */
	if (!(p = f->pool) || p == &_Sfpool ||
	    _sfpmove(f, -1) < 0 || _sfsetpool(f) < 0)
	    goto done;

	if ((p = f->pool) == &_Sfpool || p->n_sf <= 0)
	    rv = f;
	else
	    rv = p->sf[0];	/* return head of pool */
	goto done;
    }

    if (pf->pool && pf->pool != &_Sfpool)	/* always use current mode */
	mode = pf->pool->mode;

    if (mode & SF_SHARE) {	/* can only have write streams */
	if (SFMODE(f, 1) != SF_WRITE && _sfmode(f, SF_WRITE, 1) < 0)
	    goto done;
	if (SFMODE(pf, 1) != SF_WRITE && _sfmode(pf, SF_WRITE, 1) < 0)
	    goto done;
	if (f->next > f->data && SFSYNC(f) < 0)	/* start f clean */
	    goto done;
    }

    if (_sfpmove(f, -1) < 0)	/* isolate f from current pool */
	goto done;

    if (!(p = pf->pool) || p == &_Sfpool) {	/* making a new pool */
	if (!(p = newpool(mode)))
	    goto done;
	if (_sfpmove(pf, -1) < 0)	/* isolate pf from its current pool */
	    goto done;
	pf->pool = p;
	p->sf[0] = pf;
	p->n_sf += 1;
    }

    f->pool = p;		/* add f to pf's pool */
    if (_sfsetpool(f) < 0)
	goto done;

     /**/ ASSERT(p->sf[0] == pf && p->sf[p->n_sf - 1] == f);
    SFOPEN(pf, 0);
    SFOPEN(f, 0);
    if (_sfpmove(f, 0) < 0)	/* make f head of pool */
	goto done;
    rv = pf;

  done:
    if (f) {
	SFOPEN(f, 0);
	SFMTXUNLOCK(f);
    }
    if (pf) {
	SFOPEN(pf, 0);
	SFMTXUNLOCK(pf);
    }
    return rv;
}
