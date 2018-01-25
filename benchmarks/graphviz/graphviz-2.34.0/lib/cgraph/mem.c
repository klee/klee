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

#include <cghdr.h>

/* memory management discipline and entry points */

#if (HAVE_AST || HAVE_VMALLOC)

	/* vmalloc based allocator */
static void *memopen(void)
{
#if DEBUG || MEMDEBUG
    return vmopen(Vmdcheap, Vmdebug,
		  VM_MTDEBUG | VM_DBCHECK | VM_DBABORT | VM_TRACE);
#else
    return vmopen(Vmdcheap, Vmbest, VM_MTBEST);
#endif
}

static void *memalloc(void *heap, size_t request)
{
    void *rv;
    rv = vmalloc((Vmalloc_t *) heap, request);
    if (rv) memset(rv, 0, request);
    return rv;
}

static void *memresize(void *heap, void *ptr, size_t oldsize,
		       size_t request)
{
    void *rv;

    rv = vmresize((Vmalloc_t *) heap, ptr, request, VM_RSCOPY | VM_RSZERO);
    return rv;
}

static void memfree(void *heap, void *ptr)
{
    vmfree((Vmalloc_t *) heap, ptr);
}

static void memclose(void *heap)
{
    vmclose((Vmalloc_t *) heap);
}

Agmemdisc_t AgMemDisc =
    { memopen, memalloc, memresize, memfree, memclose };

#else

	/* malloc based allocator */

static void *memopen(Agdisc_t* disc)
{
    return NIL(void *);
}

static void *memalloc(void *heap, size_t request)
{
    void *rv;

    NOTUSED(heap);
    rv = malloc(request);
    memset(rv, 0, request);
    return rv;
}

static void *memresize(void *heap, void *ptr, size_t oldsize,
		       size_t request)
{
    void *rv;

    NOTUSED(heap);
    rv = realloc(ptr, request);
    if (request > oldsize)
	memset((char *) rv + oldsize, 0, request - oldsize);
    return rv;
}

static void memfree(void *heap, void *ptr)
{
    NOTUSED(heap);
    free(ptr);
}

#ifndef WRONG
#define memclose 0
#else
static void memclose(void *heap)
{
    NOTUSED(heap);
}
#endif

Agmemdisc_t AgMemDisc =
    { memopen, memalloc, memresize, memfree, memclose };

#endif


void *agalloc(Agraph_t * g, size_t size)
{
    void *mem;

    mem = AGDISC(g, mem)->alloc(AGCLOS(g, mem), size);
    if (mem == NIL(void *))
	 agerr(AGERR,"memory allocation failure");
    return mem;
}

void *agrealloc(Agraph_t * g, void *ptr, size_t oldsize, size_t size)
{
    void *mem;

    if (size > 0) {
	if (ptr == 0)
	    mem = agalloc(g, size);
	else
	    mem =
		AGDISC(g, mem)->resize(AGCLOS(g, mem), ptr, oldsize, size);
	if (mem == NIL(void *))
	     agerr(AGERR,"memory re-allocation failure");
    } else
	mem = NIL(void *);
    return mem;
}

void agfree(Agraph_t * g, void *ptr)
{
    if (ptr)
	(AGDISC(g, mem)->free) (AGCLOS(g, mem), ptr);
}

#ifndef _VMALLOC_H
struct _vmalloc_s {
    char unused;
};
#endif
struct _vmalloc_s *agheap(Agraph_t * g)
{
    return AGCLOS(g, mem);
}
