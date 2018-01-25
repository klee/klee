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

/* Priority Queue Code for shortest path in graph */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <memory.h>
#include <assert.h>

#include "fPQ.h"

static snode**  pq;
static int     PQcnt;
static snode    guard;
static int     PQsize;

void
PQgen(int sz)
{
  if (!pq) {
    pq = N_NEW(sz+1,snode*);
    pq[0] = &guard;
    PQsize = sz;
  }
  PQcnt = 0;
}

void
PQfree(void)
{
  free (pq);
  pq = NULL;
  PQcnt = 0;
}

void
PQinit(void)
{
  PQcnt = 0;
}

void
PQcheck (void)
{
  int i;
 
  for (i = 1; i <= PQcnt; i++) {
    if (N_IDX(pq[i]) != i) {
      assert (0);
    }
  }
}

void
PQupheap(int k)
{
  snode* x = pq[k];
  int     v = x->n_val;
  int     next = k/2;
  snode*  n;
  
  while (N_VAL(n = pq[next]) < v) {
    pq[k] = n;
    N_IDX(n) = k;
    k = next;
    next /= 2;
  }
  pq[k] = x;
  N_IDX(x) = k;
}

int
PQ_insert(snode* np)
{
  if (PQcnt == PQsize) {
    agerr (AGERR, "Heap overflow\n");
    return (1);
  }
  PQcnt++;
  pq[PQcnt] = np;
  PQupheap (PQcnt);
  PQcheck();
  return 0;
}

void
PQdownheap (int k)
{
  snode*    x = pq[k];
  int      v = N_VAL(x);
  int      lim = PQcnt/2;
  snode*    n;
  int      j;

  while (k <= lim) {
    j = k+k;
    n = pq[j];
    if (j < PQcnt) {
      if (N_VAL(n) < N_VAL(pq[j+1])) {
        j++;
        n = pq[j];
      }
    }
    if (v >= N_VAL(n)) break;
    pq[k] = n;
    N_IDX(n) = k;
    k = j;
  }
  pq[k] = x;
  N_IDX(x) = k;
}

snode*
PQremove (void)
{
  snode* n;

  if (PQcnt) {
    n = pq[1];
    pq[1] = pq[PQcnt];
    PQcnt--;
    if (PQcnt) PQdownheap (1);
    PQcheck();
    return n;
  }
  else return 0;
}

void
PQupdate (snode* n, int d)
{
  N_VAL(n) = d;
  PQupheap (n->n_idx);
  PQcheck();
}

void
PQprint (void)
{
  int    i;
  snode*  n;

  fprintf (stderr, "Q: ");
  for (i = 1; i <= PQcnt; i++) {
    n = pq[i];
    fprintf (stderr, "%d(%d:%d) ",  
      n->index, N_IDX(n), N_VAL(n));
  }
  fprintf (stderr, "\n");
}
