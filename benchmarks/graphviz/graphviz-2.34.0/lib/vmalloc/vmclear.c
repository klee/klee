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

/*	Clear out all allocated space.
**
**	Written by Kiem-Phong Vo, kpv@research.att.com, 01/16/94.
*/
#if __STD_C
int vmclear(Vmalloc_t * vm)
#else
int vmclear(vm)
Vmalloc_t *vm;
#endif
{
    reg Seg_t *seg;
    reg Seg_t *next;
    reg Block_t *tp;
    reg size_t size, s;
    reg Vmdata_t *vd = vm->data;

    if (!(vd->mode & VM_TRUST)) {
	if (ISLOCK(vd, 0))
	    return -1;
	SETLOCK(vd, 0);
    }

    vd->free = vd->wild = NIL(Block_t *);
    vd->pool = 0;

    if (vd->mode & (VM_MTBEST | VM_MTDEBUG | VM_MTPROFILE)) {
	vd->root = NIL(Block_t *);
	for (s = 0; s < S_TINY; ++s)
	    TINY(vd)[s] = NIL(Block_t *);
	for (s = 0; s <= S_CACHE; ++s)
	    CACHE(vd)[s] = NIL(Block_t *);
    }

    for (seg = vd->seg; seg; seg = next) {
	next = seg->next;

	tp = SEGBLOCK(seg);
	size = seg->baddr - ((Vmuchar_t *) tp) - 2 * sizeof(Head_t);

	SEG(tp) = seg;
	SIZE(tp) = size;
	if ((vd->mode & (VM_MTLAST | VM_MTPOOL)))
	    seg->free = tp;
	else {
	    SIZE(tp) |= BUSY | JUNK;
	    LINK(tp) = CACHE(vd)[C_INDEX(SIZE(tp))];
	    CACHE(vd)[C_INDEX(SIZE(tp))] = tp;
	}

	tp = BLOCK(seg->baddr);
	SEG(tp) = seg;
	SIZE(tp) = BUSY;
    }

    CLRLOCK(vd, 0);
    return 0;
}
