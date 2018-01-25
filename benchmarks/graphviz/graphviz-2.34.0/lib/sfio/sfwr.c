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

/*	Write with discipline.
**
**	Written by Kiem-Phong Vo.
*/

/* hole preserving writes */
#if __STD_C
static ssize_t sfoutput(Sfio_t * f, reg char *buf, reg size_t n)
#else
static ssize_t sfoutput(f, buf, n)
Sfio_t *f;
reg char *buf;
reg size_t n;
#endif
{
    reg char *sp, *wbuf, *endbuf;
    reg ssize_t s, w, wr;

    s = w = 0;
    wbuf = buf;
    endbuf = buf + n;
    while (n > 0) {
	if ((ssize_t) n < _Sfpage) {	/* no hole possible */
	    buf += n;
	    s = n = 0;
	} else
	    while ((ssize_t) n >= _Sfpage) {	/* see if a hole of 0's starts here */
		sp = buf + 1;
		if (buf[0] == 0 && buf[_Sfpage - 1] == 0) {	/* check byte at a time until int-aligned */
		    while (((ulong) sp) % sizeof(int)) {
			if (*sp != 0)
			    goto chk_hole;
			sp += 1;
		    }

		    /* check using int to speed up */
		    while (sp < endbuf) {
			if (*((int *) sp) != 0)
			    goto chk_hole;
			sp += sizeof(int);
		    }

		    /* check the remaining bytes */
		    if (sp > endbuf) {
			sp -= sizeof(int);
			while (sp < endbuf) {
			    if (*sp != 0)
				goto chk_hole;
			    sp += 1;
			}
		    }
		}

	      chk_hole:
		if ((s = sp - buf) >= _Sfpage)	/* found a hole */
		    break;

		/* skip a dirty page */
		n -= _Sfpage;
		buf += _Sfpage;
	    }

	/* write out current dirty pages */
	if (buf > wbuf) {
	    if ((ssize_t) n < _Sfpage) {
		buf = endbuf;
		n = s = 0;
	    }
	    if ((wr = write(f->file, wbuf, buf - wbuf)) > 0) {
		w += wr;
		f->bits &= ~SF_HOLE;
	    }
	    if (wr != (buf - wbuf))
		break;
	    wbuf = buf;
	}

	/* seek to a rounded boundary within the hole */
	if (s >= _Sfpage) {
	    s = (s / _Sfpage) * _Sfpage;
	    if (SFSK(f, (Sfoff_t) s, SEEK_CUR, NIL(Sfdisc_t *)) < 0)
		break;
	    w += s;
	    n -= s;
	    wbuf = (buf += s);
	    f->bits |= SF_HOLE;

	    if (n > 0) {	/* next page must be dirty */
		s = (ssize_t) n <= _Sfpage ? 1 : _Sfpage;
		buf += s;
		n -= s;
	    }
	}
    }

    return w > 0 ? w : -1;
}

#if __STD_C
ssize_t sfwr(reg Sfio_t * f, reg const Void_t * buf, reg size_t n,
	     reg Sfdisc_t * disc)
#else
ssize_t sfwr(f, buf, n, disc)
reg Sfio_t *f;
reg Void_t *buf;
reg size_t n;
reg Sfdisc_t *disc;
#endif
{
    reg ssize_t w;
    reg Sfdisc_t *dc;
    reg int local, oerrno;

    SFMTXSTART(f, (ssize_t) (-1));

    GETLOCAL(f, local);
    if (!local && !(f->bits & SF_DCDOWN)) {	/* an external user's call */
	if (f->mode != SF_WRITE && _sfmode(f, SF_WRITE, 0) < 0)
	    SFMTXRETURN(f, (ssize_t) (-1));
	if (f->next > f->data && SFSYNC(f) < 0)
	    SFMTXRETURN(f, (ssize_t) (-1));
    }

    for (;;) {			/* stream locked by sfsetfd() */
	if (!(f->flags & SF_STRING) && f->file < 0)
	    SFMTXRETURN(f, (ssize_t) 0);

	/* clear current error states */
	f->flags &= ~(SF_EOF | SF_ERROR);

	dc = disc;
	if (f->flags & SF_STRING)	/* total required buffer */
	    w = n + (f->next - f->data);
	else {			/* warn that a write is about to happen */
	    SFDISC(f, dc, writef);
	    if (dc && dc->exceptf && (f->flags & SF_IOCHECK)) {
		reg int rv;
		if (local)
		    SETLOCAL(f);
		if ((rv = _sfexcept(f, SF_WRITE, n, dc)) > 0)
		    n = rv;
		else if (rv < 0) {
		    f->flags |= SF_ERROR;
		    SFMTXRETURN(f, rv);
		}
	    }

	    if (f->extent >= 0) {	/* make sure we are at the right place to write */
		if (f->flags & SF_APPENDWR) {
		    if (f->here != f->extent || (f->flags & SF_SHARE)) {
			f->here = SFSK(f, (Sfoff_t) 0, SEEK_END, dc);
			f->extent = f->here;
		    }
		} else if ((f->flags & SF_SHARE)
			   && !(f->flags & SF_PUBLIC))
		    f->here = SFSK(f, f->here, SEEK_SET, dc);
	    }

	    oerrno = errno;
	    errno = 0;

	    if (dc && dc->writef) {
		SFDCWR(f, buf, n, dc, w);
	    } else if (SFISNULL(f))
		w = n;
	    else if (f->flags & SF_WHOLE)
		goto do_write;
	    else if ((ssize_t) n >= _Sfpage &&
		     !(f->flags & (SF_SHARE | SF_APPENDWR)) &&
		     f->here == f->extent && (f->here % _Sfpage) == 0) {
		if ((w = sfoutput(f, (char *) buf, n)) <= 0)
		    goto do_write;
	    } else {
	      do_write:
		if ((w = write(f->file, (char *) buf, n)) > 0)
		    f->bits &= ~SF_HOLE;
	    }

	    if (errno == 0)
		errno = oerrno;

	    if (w > 0) {
		if (!(f->bits & SF_DCDOWN)) {
		    if (f->flags & (SF_APPENDWR | SF_PUBLIC))
			f->here = SFSK(f, (Sfoff_t) 0, SEEK_CUR, dc);
		    else
			f->here += w;
		    if (f->extent >= 0 && f->here > f->extent)
			f->extent = f->here;
		}

		SFMTXRETURN(f, (ssize_t) w);
	    }
	}

	if (local)
	    SETLOCAL(f);
	switch (_sfexcept(f, SF_WRITE, w, dc)) {
	case SF_ECONT:
	    goto do_continue;
	case SF_EDONE:
	    w = local ? 0 : w;
	    SFMTXRETURN(f, (ssize_t) w);
	case SF_EDISC:
	    if (!local && !(f->flags & SF_STRING))
		goto do_continue;
	    /* else fall thru */
	case SF_ESTACK:
	    SFMTXRETURN(f, (ssize_t) (-1));
	}

      do_continue:
	for (dc = f->disc; dc; dc = dc->disc)
	    if (dc == disc)
		break;
	disc = dc;
    }
}
