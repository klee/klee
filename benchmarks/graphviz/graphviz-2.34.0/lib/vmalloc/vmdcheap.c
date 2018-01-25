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

#include	"vmhdr.h"

/*	A discipline to get memory from the heap.
**
**	Written by Kiem-Phong Vo, kpv@research.att.com, 01/16/94.
*/
#if __STD_C
static Void_t *heapmem(Vmalloc_t * vm, Void_t * caddr,
		       size_t csize, size_t nsize, Vmdisc_t * disc)
#else
static Void_t *heapmem(vm, caddr, csize, nsize, disc)
Vmalloc_t *vm;			/* region doing allocation from         */
Void_t *caddr;			/* current low address                  */
size_t csize;			/* current size                         */
size_t nsize;			/* new size                             */
Vmdisc_t *disc;			/* discipline structure                 */
#endif
{
    NOTUSED(vm);
    NOTUSED(disc);

    if (csize == 0)
	return vmalloc(Vmheap, nsize);
    else if (nsize == 0)
	return vmfree(Vmheap, caddr) >= 0 ? caddr : NIL(Void_t *);
    else
	return vmresize(Vmheap, caddr, nsize, 0);
}

static Vmdisc_t _Vmdcheap = { heapmem, NIL(Vmexcept_f), 0 };

__DEFINE__(Vmdisc_t *, Vmdcheap, &_Vmdcheap);
