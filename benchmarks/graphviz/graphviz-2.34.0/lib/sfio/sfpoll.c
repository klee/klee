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

/*	Poll a set of streams to see if any is available for I/O.
**	Ready streams are moved to front of array but retain the
**	same relative order.
**
**	Written by Kiem-Phong Vo.
*/

#if __STD_C
int sfpoll(Sfio_t ** fa, reg int n, int tm)
#else
int sfpoll(fa, n, tm)
Sfio_t **fa;			/* array of streams to poll */
reg int n;			/* number of streams in array */
int tm;				/* the amount of time in ms to wait for selecting */
#endif
{
    reg int r, c, m;
    reg Sfio_t *f;
    reg Sfdisc_t *d;
    reg int *status, *check;

    if (n <= 0 || !fa)
	return -1;

    if (!(status = (int *) malloc(2 * n * sizeof(int))))
	return -1;
    else
	check = status + n;

    /* this loop partitions the streams into 3 sets: Check, Ready, Notready */
  retry:for (r = c = 0; r < n; ++r) {
	f = fa[r];

	/* this loop pops a stream stack as necessary */
	for (;;) {		/* check accessibility */
	    m = f->mode & SF_RDWR;
	    if ((int) f->mode != m && _sfmode(f, m, 0) < 0)
		goto do_never;

	    /* clearly ready */
	    if (f->next < f->endb)
		goto do_ready;

	    /* has discipline, ask its opinion */
	    for (d = f->disc; d; d = d->disc)
		if (d->exceptf)
		    break;
	    if (d) {
		if ((m = (*d->exceptf) (f, SF_DPOLL, &tm, d)) < 0)
		    goto do_never;
		else if (m > 0)
		    goto do_ready;
		/*else check file descriptor */
	    }

	    /* unseekable stream, must check for blockability */
	    if (f->extent < 0)
		goto do_check;

	    /* string/regular streams with no possibility of blocking */
	    if (!f->push)
		goto do_ready;

	    /* stacked regular file stream with I/O possibility */
	    if (!(f->flags & SF_STRING) &&
		((f->mode & SF_WRITE) || f->here < f->extent))
		goto do_ready;

	    /* at an apparent eof, pop stack if ok, then recheck */
	    SETLOCAL(f);
	    switch (_sfexcept(f, f->mode & SF_RDWR, 0, f->disc)) {
	    case SF_EDONE:
		if (f->flags & SF_STRING)
		    goto do_never;
		else
		    goto do_ready;
	    case SF_EDISC:
		if (f->flags & SF_STRING)
		    goto do_ready;
	    case SF_ESTACK:
	    case SF_ECONT:
		continue;
	    }
	}

      do_check:		/* local function to set a stream for further checking */
	{
	    status[r] = 0;
	    check[c] = r;
	    c += 1;
	    continue;
	}

      do_ready:		/* local function to set the ready streams */
	{
	    status[r] = 1;
	    continue;
	}

      do_never:		/* local function to set the not-ready streams */
	{
	    status[r] = -1;
	    continue;
	}
    }

#if _lib_poll
    if (c > 0) {
	struct pollfd *fds;

	/* construct the poll array */
	if (!(fds = (struct pollfd *) malloc(c * sizeof(struct pollfd))))
	    return -1;
	for (r = 0; r < c; r++) {
	    fds[r].fd = fa[check[r]]->file;
	    fds[r].events =
		(fa[check[r]]->mode & SF_READ) ? POLLIN : POLLOUT;
	    fds[r].revents = 0;
	}

	for (;;) {		/* this loop takes care of interrupts */
	    if ((r = SFPOLL(fds, c, tm)) == 0)
		break;
	    else if (r < 0) {
		if (errno == EINTR || errno == EAGAIN) {
		    errno = 0;
		    continue;
		} else
		    break;
	    }

	    for (r = 0; r < c; ++r) {
		f = fa[check[r]];
		if (((f->mode & SF_READ) && (fds[r].revents & POLLIN)) ||
		    ((f->mode & SF_WRITE) && (fds[r].revents & POLLOUT)))
		    status[check[r]] = 1;
	    }
	    break;
	}

	free((Void_t *) fds);
    }
#endif /*_lib_poll*/

#if _lib_select
    if (c > 0) {
	fd_set rd, wr;
	struct timeval tmb, *tmp;

	FD_ZERO(&rd);
	FD_ZERO(&wr);
	m = 0;
	for (r = 0; r < c; ++r) {
	    f = fa[check[r]];
	    if (f->file > m)
		m = f->file;
	    if (f->mode & SF_READ)
		FD_SET(f->file, &rd);
	    else
		FD_SET(f->file, &wr);
	}
	if (tm < 0)
	    tmp = NIL(struct timeval *);
	else {
	    tmp = &tmb;
	    tmb.tv_sec = tm / SECOND;
	    tmb.tv_usec = (tm % SECOND) * SECOND;
	}
	for (;;) {
	    if ((r = select(m + 1, &rd, &wr, NIL(fd_set *), tmp)) == 0)
		break;
	    else if (r < 0) {
		if (errno == EINTR)
		    continue;
		else
		    break;
	    }

	    for (r = 0; r < c; ++r) {
		f = fa[check[r]];
		if (((f->mode & SF_READ) && FD_ISSET(f->file, &rd)) ||
		    ((f->mode & SF_WRITE) && FD_ISSET(f->file, &wr)))
		    status[check[r]] = 1;
	    }
	    break;
	}
    }
#endif /*_lib_select*/

    /* call exception functions */
    for (c = 0; c < n; ++c) {
	if (status[c] <= 0)
	    continue;
	if ((d = fa[c]->disc) && d->exceptf) {
	    if ((r = (*d->exceptf) (fa[c], SF_READY, (Void_t *) 0, d)) < 0)
		goto done;
	    else if (r > 0)
		goto retry;
	}
    }

    /* move ready streams to the front */
    for (r = c = 0; c < n; ++c) {
	if (status[c] > 0) {
	    if (c > r) {
		f = fa[r];
		fa[r] = fa[c];
		fa[c] = f;
	    }
	    r += 1;
	}
    }

  done:
    free((Void_t *) status);
    return r;
}
