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

/*	Tell the current location in a given stream
**
**	Written by Kiem-Phong Vo.
*/

#if __STD_C
Sfoff_t sftell(reg Sfio_t * f)
#else
Sfoff_t sftell(f)
reg Sfio_t *f;
#endif
{
    reg int mode;
    Sfoff_t p;

    SFMTXSTART(f, (Sfoff_t) (-1));

    /* set the stream to the right mode */
    if ((mode = f->mode & SF_RDWR) != (int) f->mode
	&& _sfmode(f, mode, 0) < 0)
	SFMTXRETURN(f, (Sfoff_t) (-1));

    /* throw away ungetc data */
    if (f->disc == _Sfudisc)
	(void) sfclose((*_Sfstack) (f, NIL(Sfio_t *)));

    if (f->flags & SF_STRING)
	SFMTXRETURN(f, (Sfoff_t) (f->next - f->data));

    /* let sfseek() handle the hard case */
    if (f->extent >= 0 && (f->flags & (SF_SHARE | SF_APPENDWR)))
	p = sfseek(f, (Sfoff_t) 0, 1);
    else
	p = f->here + ((f->mode & SF_WRITE) ? f->next - f->data : f->next -
		       f->endb);

    SFMTXRETURN(f, p);
}
