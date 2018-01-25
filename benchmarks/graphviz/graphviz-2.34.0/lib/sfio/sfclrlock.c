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

/*	Function to clear a locked stream.
**	This is useful for programs that longjmp from the mid of an sfio function.
**	There is no guarantee on data integrity in such a case.
**
**	Written by Kiem-Phong Vo
*/
#if __STD_C
int sfclrlock(reg Sfio_t * f)
#else
int sfclrlock(f)
reg Sfio_t *f;
#endif
{
    int rv;

    /* already closed */
    if (f && (f->mode & SF_AVAIL))
	return 0;

    SFMTXSTART(f, 0);

    /* clear error bits */
    f->flags &= ~(SF_ERROR | SF_EOF);

    /* clear peek locks */
    if (f->mode & SF_PKRD) {
	f->here -= f->endb - f->next;
	f->endb = f->next;
    }

    SFCLRBITS(f);

    /* throw away all lock bits except for stacking state SF_PUSH */
    f->mode &=
	(SF_RDWR | SF_INIT | SF_POOL | SF_PUSH | SF_SYNCED | SF_STDIO);

    rv = (f->mode & SF_PUSH) ? 0 : (f->flags & SF_FLAGS);

    SFMTXRETURN(f, rv);
}
