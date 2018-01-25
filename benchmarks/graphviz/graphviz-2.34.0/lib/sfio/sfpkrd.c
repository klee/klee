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
#if !_PACKAGE_ast
#ifndef FIONREAD
#if _sys_ioctl
#include	<sys/ioctl.h>
#endif
#endif
#endif

/*	Read/Peek a record from an unseekable device
**
**	Written by Kiem-Phong Vo.
*/

#define STREAM_PEEK	001
#define SOCKET_PEEK	002

#if __STD_C
ssize_t sfpkrd(int fd, Void_t * argbuf, size_t n, int rc, long tm,
	       int action)
#else
ssize_t sfpkrd(fd, argbuf, n, rc, tm, action)
int fd;				/* file descriptor */
Void_t *argbuf;			/* buffer to read data */
size_t n;			/* buffer size */
int rc;				/* record character */
long tm;			/* time-out */
int action;			/* >0: peeking, if rc>=0, get action records,
				   <0: no peeking, if rc>=0, get -action records,
				   =0: no peeking, if rc>=0, must get a single record
				 */
#endif
{
    reg ssize_t r;
    reg int ntry, t;
    reg char *buf = (char *) argbuf, *endbuf;

    if (rc < 0 && tm < 0 && action <= 0)
	return read(fd, buf, n);

    t = (action > 0 || rc >= 0) ? (STREAM_PEEK | SOCKET_PEEK) : 0;
#if !_stream_peek
    t &= ~STREAM_PEEK;
#endif
#if !_socket_peek
    t &= ~SOCKET_PEEK;
#endif

    for (ntry = 0; ntry < 2; ++ntry) {
	r = -1;
#if _stream_peek
	if ((t & STREAM_PEEK) && (ntry == 1 || tm < 0)) {
	    struct strpeek pbuf;
	    pbuf.flags = 0;
	    pbuf.ctlbuf.maxlen = -1;
	    pbuf.ctlbuf.len = 0;
	    pbuf.ctlbuf.buf = NIL(char *);
	    pbuf.databuf.maxlen = n;
	    pbuf.databuf.buf = buf;
	    pbuf.databuf.len = 0;

	    if ((r = ioctl(fd, I_PEEK, &pbuf)) < 0) {
		if (errno == EINTR)
		    return -1;
		t &= ~STREAM_PEEK;
	    } else {
		t &= ~SOCKET_PEEK;
		if (r > 0 && (r = pbuf.databuf.len) <= 0) {
		    if (action <= 0)	/* read past eof */
			r = read(fd, buf, 1);
		    return r;
		}
		if (r == 0)
		    r = -1;
		else if (r > 0)
		    break;
	    }
	}
#endif				/* stream_peek */

	if (ntry == 1)
	    break;

	/* poll or select to see if data is present.  */
	while (tm >= 0 || action > 0 ||
	       /* block until there is data before peeking again */
	       ((t & STREAM_PEEK) && rc >= 0) ||
	       /* let select be interrupted instead of recv which autoresumes */
	       (t & SOCKET_PEEK)) {
	    r = -2;
#if _lib_poll
	    if (r == -2) {
		struct pollfd po;
		po.fd = fd;
		po.events = POLLIN;
		po.revents = 0;

		if ((r = SFPOLL(&po, 1, tm)) < 0) {
		    if (errno == EINTR)
			return -1;
		    else if (errno == EAGAIN) {
			errno = 0;
			continue;
		    } else
			r = -2;
		} else
		    r = (po.revents & POLLIN) ? 1 : -1;
	    }
#endif /*_lib_poll*/
#if _lib_select
	    if (r == -2) {
#if _hpux_threads && vt_threaded
#define fd_set	int
#endif
		fd_set rd;
		struct timeval tmb, *tmp;
		FD_ZERO(&rd);
		FD_SET(fd, &rd);
		if (tm < 0)
		    tmp = NIL(struct timeval *);
		else {
		    tmp = &tmb;
		    tmb.tv_sec = tm / SECOND;
		    tmb.tv_usec = (tm % SECOND) * SECOND;
		}
		r = select(fd + 1, &rd, NIL(fd_set *), NIL(fd_set *), tmp);
		if (r < 0) {
		    if (errno == EINTR)
			return -1;
		    else if (errno == EAGAIN) {
			errno = 0;
			continue;
		    } else
			r = -2;
		} else
		    r = FD_ISSET(fd, &rd) ? 1 : -1;
	    }
#endif /*_lib_select*/
	    if (r == -2) {
#if !_lib_poll && !_lib_select	/* both poll and select cann't be used */
#ifdef FIONREAD			/* quick and dirty check for availability */
		long nsec = tm < 0 ? 0 : (tm + 999) / 1000;
		while (nsec > 0 && r < 0) {
		    long avail = -1;
		    if ((r = ioctl(fd, FIONREAD, &avail)) < 0) {
			if (errno == EINTR)
			    return -1;
			else if (errno == EAGAIN) {
			    errno = 0;
			    continue;
			} else {	/* ioctl failed completely */
			    r = -2;
			    break;
			}
		    } else
			r = avail <= 0 ? -1 : (ssize_t) avail;

		    if (r < 0 && nsec-- > 0)
			sleep(1);
		}
#endif
#endif
	    }

	    if (r > 0) {	/* there is data now */
		if (action <= 0 && rc < 0)
		    return read(fd, buf, n);
		else
		    r = -1;
	    } else if (tm >= 0)	/* timeout exceeded */
		return -1;
	    else
		r = -1;
	    break;
	}

#if _socket_peek
	if (t & SOCKET_PEEK) {
	    while ((r = recv(fd, (char *) buf, n, MSG_PEEK)) < 0) {
		if (errno == EINTR)
		    return -1;
		else if (errno == EAGAIN) {
		    errno = 0;
		    continue;
		}
		t &= ~SOCKET_PEEK;
		break;
	    }
	    if (r >= 0) {
		t &= ~STREAM_PEEK;
		if (r > 0)
		    break;
		else {		/* read past eof */
		    if (action <= 0)
			r = read(fd, buf, 1);
		    return r;
		}
	    }
	}
#endif
    }

    if (r < 0) {
	if (tm >= 0 || action > 0)
	    return -1;
	else {			/* get here means: tm < 0 && action <= 0 && rc >= 0 */
	    /* number of records read at a time */
	    if ((action = action ? -action : 1) > (int) n)
		action = n;
	    r = 0;
	    while ((t = read(fd, buf, action)) > 0) {
		r += t;
		for (endbuf = buf + t; buf < endbuf;)
		    if (*buf++ == rc)
			action -= 1;
		if (action == 0 || (int) (n - r) < action)
		    break;
	    }
	    return r == 0 ? t : r;
	}
    }

    /* successful peek, find the record end */
    if (rc >= 0) {
	reg char *sp;

	t = action == 0 ? 1 : action < 0 ? -action : action;
	for (endbuf = (sp = buf) + r; sp < endbuf;)
	    if (*sp++ == rc)
		if ((t -= 1) == 0)
		    break;
	r = sp - buf;
    }

    /* advance */
    if (action <= 0)
	r = read(fd, buf, r);

    return r;
}
