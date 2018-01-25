/* $Id$Revision: */
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

#include <memory.h>
#include <stdio.h>

/* Priority queue:
 * To work, the following have to be defined before this file is
 * included:
 *   - PQTYPE   : type of object stored in the queue
 *   - PQVTYPE  : type of priority value
 *   - N_VAL(pq,n) : macro for (negative) priority value of object n in pq
 *   - N_IDX(pq,n) : macro for integer index > 0 of n in pq
 *   - guard, type PQTYPE, with N_VAL(guard) == 0
 *
 * Priorities are stored as negative numbers, with the item with the least
 * negative priority at the head (just after the guard).
 */

#ifdef PQ_TYPES
typedef struct {
    PQTYPE*  pq;
    int     PQcnt;
    int     PQsize;
} PQ;
#endif

#ifdef PQ_CODE
static void
PQgen(PQ* pq, int sz, PQTYPE guard)
{
    pq->pq = N_NEW(sz+1,PQTYPE);
    pq->pq[0] = guard;
    pq->PQsize = sz;
    pq->PQcnt = 0;
}

static void
PQfree(PQ* pq, int freeAll)
{
    free (pq->pq);
    if (freeAll) free (pq);
}

static void
PQinit(PQ* pq)
{
    pq->PQcnt = 0;
}

#ifdef PQCHECK
static int
PQcheck (PQ* pq)
{
    int i;
 
    for (i = 1; i <= pq->PQcnt; i++) {
	if (N_IDX(pq,pq->pq[i]) != i) {
	    return 1;
	}
    }
    return 0;
}
#endif

static void
PQupheap(PQ* ppq, int k)
{
    PQTYPE* pq = ppq->pq;
    PQTYPE x = pq[k];
    PQVTYPE v = N_VAL(ppq,x);
    int	 next = k/2;
    PQTYPE  n;
    
    while (N_VAL(ppq,n = pq[next]) < v) {
	pq[k] = n;
	N_IDX(ppq,n) = k;
	k = next;
	next /= 2;
    }
    pq[k] = x;
    N_IDX(ppq,x) = k;
}

static int
PQinsert(PQ* pq, PQTYPE np)
{
    if (pq->PQcnt == pq->PQsize) {
	agerr (AGERR, "Heap overflow\n");
	return (1);
    }
    pq->PQcnt++;
    pq->pq[pq->PQcnt] = np;
    PQupheap (pq, pq->PQcnt);
#ifdef PQCHECK
    PQcheck(pq);
#endif
    return 0;
}

static void
PQdownheap (PQ* ppq, int k)
{
    PQTYPE*  pq = ppq->pq;
    PQTYPE x = pq[k];
    PQVTYPE v = N_VAL(ppq,x);
    int	  lim = ppq->PQcnt/2;
    PQTYPE n;
    int	   j;

    while (k <= lim) {
	j = k+k;
	n = pq[j];
	if (j < ppq->PQcnt) {
	    if (N_VAL(ppq,n) < N_VAL(ppq,pq[j+1])) {
		j++;
		n = pq[j];
	    }
	}
	if (v >= N_VAL(ppq,n)) break;
	pq[k] = n;
	N_IDX(ppq,n) = k;
	k = j;
    }
    pq[k] = x;
    N_IDX(ppq,x) = k;
}

static PQTYPE
PQremove (PQ* pq)
{
    PQTYPE n;

    if (pq->PQcnt) {
	n = pq->pq[1];
	pq->pq[1] = pq->pq[pq->PQcnt];
	pq->PQcnt--;
	if (pq->PQcnt) PQdownheap (pq, 1);
#ifdef PQCHECK
	PQcheck(pq);
#endif
	return n;
    }
    else return pq->pq[0];
}

static void
PQupdate (PQ* pq, PQTYPE n, PQVTYPE d)
{
    N_VAL(pq,n) = d;
    PQupheap (pq, N_IDX(pq,n));
#ifdef PQCHECK
    PQcheck(pq);
#endif
}

#ifdef DEBUG

static void
PQprint (PQ* pq)
{
    int	i;
    PQTYPE  n;

    fprintf (stderr, "Q: ");
    for (i = 1; i <= pq->PQcnt; i++) {
	n = pq->pq[i];
	fprintf (stderr, "(%d:%f) ", N_IDX(pq,n), N_VAL(pq,n));
    }
    fprintf (stderr, "\n");
}
#endif
#endif

