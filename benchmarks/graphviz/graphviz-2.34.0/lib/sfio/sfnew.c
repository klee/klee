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

/*	Fundamental function to create a new stream.
**	The argument flags defines the type of stream and the scheme
**	of buffering.
**
**	Written by Kiem-Phong Vo.
*/

#if __STD_C
Sfio_t *sfnew(Sfio_t * oldf, Void_t * buf, size_t size, int file,
	      int flags)
#else
Sfio_t *sfnew(oldf, buf, size, file, flags)
Sfio_t *oldf;			/* old stream to be reused */
Void_t *buf;			/* a buffer to read/write, if NULL, will be allocated */
size_t size;			/* buffer size if buf is given or desired buffer size */
int file;			/* file descriptor to read/write from */
int flags;			/* type of file stream */
#endif
{
    reg Sfio_t *f;
    reg int sflags;

    SFONCE();			/* initialize mutexes */

    if (!(flags & SF_RDWR))
	return NIL(Sfio_t *);

    sflags = 0;
    if ((f = oldf)) {
	if (flags & SF_EOF) {
	    if (f != sfstdin && f != sfstdout && f != sfstderr)
		f->mutex = NIL(Vtmutex_t *);
	    SFCLEAR(f, f->mutex);
	    oldf = NIL(Sfio_t *);
	} else if (f->mode & SF_AVAIL) {	/* only allow SF_STATIC to be already closed */
	    if (!(f->flags & SF_STATIC))
		return NIL(Sfio_t *);
	    sflags = f->flags;
	    oldf = NIL(Sfio_t *);
	} else {		/* reopening an open stream, close it first */
	    sflags = f->flags;

	    if (((f->mode & SF_RDWR) != f->mode && _sfmode(f, 0, 0) < 0) ||
		SFCLOSE(f) < 0)
		return NIL(Sfio_t *);

	    if (f->data
		&& ((flags & SF_STRING) || size != (size_t) SF_UNBOUND)) {
		if (sflags & SF_MALLOC)
		    free((Void_t *) f->data);
		f->data = NIL(uchar *);
	    }
	    if (!f->data)
		sflags &= ~SF_MALLOC;
	}
    }

    if (!f) {			/* reuse a standard stream structure if possible */
	if (!(flags & SF_STRING) && file >= 0 && file <= 2) {
	    f = file == 0 ? sfstdin : file == 1 ? sfstdout : sfstderr;
	    if (f) {
		if (f->mode & SF_AVAIL) {
		    sflags = f->flags;
		    SFCLEAR(f, f->mutex);
		} else
		    f = NIL(Sfio_t *);
	    }
	}

	if (!f) {
	    if (!(f = (Sfio_t *) malloc(sizeof(Sfio_t))))
		return NIL(Sfio_t *);
	    SFCLEAR(f, NIL(Vtmutex_t *));
	}
    }

    /* create a mutex */
#if vt_threaded
    if (!f->mutex)
	f->mutex = vtmtxopen(NIL(Vtmutex_t *), VT_INIT);
#endif

    /* stream type */
    f->mode = (flags & SF_READ) ? SF_READ : SF_WRITE;
    f->flags = (flags & SF_FLAGS) | (sflags & (SF_MALLOC | SF_STATIC));
    f->bits = (flags & SF_RDWR) == SF_RDWR ? SF_BOTH : 0;
    f->file = file;
    f->here = f->extent = 0;
    f->getr = f->tiny[0] = 0;

    f->mode |= SF_INIT;
    if (size != (size_t) SF_UNBOUND) {
	f->size = size;
	f->data = size <= 0 ? NIL(uchar *) : (uchar *) buf;
    }
    f->endb = f->endr = f->endw = f->next = f->data;

    if (_Sfnotify)
	(*_Sfnotify) (f, SF_NEW, f->file);

    if (f->flags & SF_STRING)
	(void) _sfmode(f, f->mode & SF_RDWR, 0);

    return f;
}
