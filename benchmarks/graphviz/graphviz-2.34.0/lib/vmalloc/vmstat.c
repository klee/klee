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

/*	Get statistics from a region.
**
**	Written by Kiem-Phong Vo, kpv@research.att.com, 01/16/94.
*/

#if __STD_C
int vmstat(Vmalloc_t * vm, Vmstat_t * st)
#else
int vmstat(vm, st)
Vmalloc_t *vm;
Vmstat_t *st;
#endif
{
    reg Seg_t *seg;
    reg Block_t *b, *endb;
    reg size_t s = 0;
    reg Vmdata_t *vd = vm->data;

    if (!st)
	return -1;
    if (!(vd->mode & VM_TRUST)) {
	if (ISLOCK(vd, 0))
	    return -1;
	SETLOCK(vd, 0);
    }

    st->n_busy = st->n_free = 0;
    st->s_busy = st->s_free = st->m_busy = st->m_free = 0;
    st->n_seg = 0;
    st->extent = 0;

    if (vd->mode & VM_MTLAST)
	st->n_busy = 0;
    else if ((vd->mode & VM_MTPOOL) && (s = vd->pool) > 0) {
	s = ROUND(s, ALIGN);
	for (b = vd->free; b; b = SEGLINK(b))
	    st->n_free += 1;
    }

    for (seg = vd->seg; seg; seg = seg->next) {
	st->n_seg += 1;
	st->extent += seg->extent;

	b = SEGBLOCK(seg);
	endb = BLOCK(seg->baddr);

	if (vd->mode & (VM_MTDEBUG | VM_MTBEST | VM_MTPROFILE)) {
	    while (b < endb) {
		s = SIZE(b) & ~BITS;
		if (ISJUNK(SIZE(b)) || !ISBUSY(SIZE(b))) {
		    if (s > st->m_free)
			st->m_free = s;
		    st->s_free += s;
		    st->n_free += 1;
		} else {	/* get the real size */
		    if (vd->mode & VM_MTDEBUG)
			s = DBSIZE(DB2DEBUG(DATA(b)));
		    else if (vd->mode & VM_MTPROFILE)
			s = PFSIZE(DATA(b));
		    if (s > st->m_busy)
			st->m_busy = s;
		    st->s_busy += s;
		    st->n_busy += 1;
		}

		b = (Block_t *) ((Vmuchar_t *) DATA(b) +
				 (SIZE(b) & ~BITS));
	    }
	} else if (vd->mode & VM_MTLAST) {
	    if ((s =
		 seg->free ? (SIZE(seg->free) + sizeof(Head_t)) : 0) > 0) {
		st->s_free += s;
		st->n_free += 1;
	    }
	    if ((s = ((char *) endb - (char *) b) - s) > 0) {
		st->s_busy += s;
		st->n_busy += 1;
	    }
	} else if ((vd->mode & VM_MTPOOL) && s > 0) {
	    if (seg->free)
		st->n_free += (SIZE(seg->free) + sizeof(Head_t)) / s;
	    st->n_busy +=
		((seg->baddr - (Vmuchar_t *) b) - sizeof(Head_t)) / s;
	}
    }

    if ((vd->mode & VM_MTPOOL) && s > 0) {
	st->n_busy -= st->n_free;
	if (st->n_busy > 0)
	    st->s_busy = (st->m_busy = vd->pool) * st->n_busy;
	if (st->n_free > 0)
	    st->s_free = (st->m_free = vd->pool) * st->n_free;
    }

    CLRLOCK(vd, 0);
    return 0;
}
