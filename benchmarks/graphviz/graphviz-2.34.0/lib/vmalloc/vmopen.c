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

/*	Opening a new region of allocation.
**	Note that because of possible exotic memory types,
**	all region data must be stored within the space given
**	by the discipline.
**
**	Written by Kiem-Phong Vo, kpv@research.att.com, 01/16/94.
*/

typedef struct _vminit_ {
    Vmdata_t vd;		/* space for the region itself  */
    Seg_t seg;			/* space for segment            */
    Block_t block;		/* space for a block            */
    Head_t head;		/* space for the fake header    */
    char a[3 * ALIGN];		/* extra to fuss with alignment */
} Vminit_t;

#if __STD_C
Vmalloc_t *vmopen(Vmdisc_t * disc, Vmethod_t * meth, int mode)
#else
Vmalloc_t *vmopen(disc, meth, mode)
Vmdisc_t *disc;			/* discipline to get segments   */
Vmethod_t *meth;		/* method to manage space       */
int mode;			/* type of region               */
#endif
{
    reg Vmalloc_t *vm;
    reg Vmdata_t *vd;
    reg size_t s, a, incr;
    reg Block_t *b;
    reg Seg_t *seg;
    Vmuchar_t *addr;
    reg Vmemory_f memoryf;
    reg int e;

    if (!meth || !disc || !(memoryf = disc->memoryf))
	return NIL(Vmalloc_t *);

    GETPAGESIZE(_Vmpagesize);

    /* note that Vmalloc_t space must be local to process since that's
       where the meth&disc function addresses are going to be stored */
    if (!(vm = (Vmalloc_t *) vmalloc(Vmheap, sizeof(Vmalloc_t))))
	return NIL(Vmalloc_t *);
    vm->meth = *meth;
    vm->disc = disc;
    vm->file = NIL(char *);
    vm->line = 0;

    if (disc->exceptf) {
	addr = NIL(Vmuchar_t *);
	if ((e =
	     (*disc->exceptf) (vm, VM_OPEN, (Void_t *) (&addr),
			       disc)) != 0) {
	    if (e < 0 || !addr)
		goto open_error;

	    /* align this address */
	    if ((a = (size_t) (VLONG(addr) % ALIGN)) != 0)
		addr += ALIGN - a;

	    /* see if it's a valid region */
	    vd = (Vmdata_t *) addr;
	    if ((vd->mode & meth->meth) != 0) {
		vm->data = vd;
		return vm;
	    } else {
	      open_error:
		vmfree(Vmheap, vm);
		return NIL(Vmalloc_t *);
	    }
	}
    }

    /* make sure vd->incr is properly rounded */
    incr = disc->round <= 0 ? _Vmpagesize : disc->round;
    incr = MULTIPLE(incr, ALIGN);

    /* get space for region data */
    s = ROUND(sizeof(Vminit_t), incr);
    if (!(addr = (Vmuchar_t *) (*memoryf) (vm, NIL(Void_t *), 0, s, disc))) {
	vmfree(Vmheap, vm);
	return NIL(Vmalloc_t *);
    }

    /* make sure that addr is aligned */
    if ((a = (size_t) (VLONG(addr) % ALIGN)) != 0)
	addr += ALIGN - a;

    /* initialize region */
    vd = (Vmdata_t *) addr;
    vd->mode = (mode & VM_FLAGS) | meth->meth;
    vd->incr = incr;
    vd->pool = 0;
    vd->free = vd->wild = NIL(Block_t *);

    if (vd->mode & (VM_TRACE | VM_MTDEBUG))
	vd->mode &= ~VM_TRUST;

    if (vd->mode & (VM_MTBEST | VM_MTDEBUG | VM_MTPROFILE)) {
	vd->root = NIL(Block_t *);
	for (e = S_TINY - 1; e >= 0; --e)
	    TINY(vd)[e] = NIL(Block_t *);
	for (e = S_CACHE; e >= 0; --e)
	    CACHE(vd)[e] = NIL(Block_t *);
	incr = sizeof(Vmdata_t);
    } else
	incr = OFFSET(Vmdata_t, root);

    vd->seg = (Seg_t *) (addr + ROUND(incr, ALIGN));
     /**/ ASSERT(VLONG(vd->seg) % ALIGN == 0);

    seg = vd->seg;
    seg->next = NIL(Seg_t *);
    seg->vm = vm;
    seg->addr = (Void_t *) (addr - (a ? ALIGN - a : 0));
    seg->extent = s;
    seg->baddr = addr + s - (a ? ALIGN : 0);
    seg->size = s;		/* this size is larger than usual so that the segment
				   will not be freed until the region is closed. */
    seg->free = NIL(Block_t *);

    /* make a data block out of the remainder */
    b = SEGBLOCK(seg);
    SEG(b) = seg;
    SIZE(b) = seg->baddr - (Vmuchar_t *) b - 2 * sizeof(Head_t);
    *SELF(b) = b;
     /**/ ASSERT(SIZE(b) % ALIGN == 0);
     /**/ ASSERT(VLONG(b) % ALIGN == 0);

    /* make a fake header for next block in case of noncontiguous segments */
    SEG(NEXT(b)) = seg;
    SIZE(NEXT(b)) = BUSY | PFREE;

    if (vd->mode & (VM_MTLAST | VM_MTPOOL))
	seg->free = b;
    else
	vd->wild = b;

    vm->data = vd;

    /* put into linked list of regions */
    vm->next = Vmheap->next;
    Vmheap->next = vm;

    return vm;
}
