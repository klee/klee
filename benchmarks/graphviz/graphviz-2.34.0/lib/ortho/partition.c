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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <partition.h>
#include <trap.h>
#include <memory.h>
#include <math.h>
#include <stdlib.h>

#define NPOINTS 4   /* only rectangles */
#define TRSIZE(ss) (5*(ss)+1)

#define TR_FROM_UP 1        /* for traverse-direction */
#define TR_FROM_DN 2

#define SP_SIMPLE_LRUP 1    /* for splitting trapezoids */
#define SP_SIMPLE_LRDN 2
#define SP_2UP_2DN     3
#define SP_2UP_LEFT    4
#define SP_2UP_RIGHT   5
#define SP_2DN_LEFT    6
#define SP_2DN_RIGHT   7
#define SP_NOSPLIT    -1

#define DOT(v0, v1) ((v0).x * (v1).x + (v0).y * (v1).y)
#define CROSS_SINE(v0, v1) ((v0).x * (v1).y - (v1).x * (v0).y)
#define LENGTH(v0) (sqrt((v0).x * (v0).x + (v0).y * (v0).y))

#ifndef HAVE_SRAND48
#define srand48 srand
#endif
#ifdef WIN32
extern double drand48(void);
#endif

typedef struct {
  int vnum;
  int next;         /* Circularly linked list  */
  int prev;         /* describing the monotone */
  int marked;           /* polygon */
} monchain_t;

typedef struct {
  pointf pt;
  int vnext[4];         /* next vertices for the 4 chains */
  int vpos[4];          /* position of v in the 4 chains */
  int nextfree;
} vertexchain_t;

static int chain_idx, mon_idx;
	/* Table to hold all the monotone */
	/* polygons . Each monotone polygon */
	/* is a circularly linked list */
static monchain_t* mchain;
	/* chain init. information. This */
	/* is used to decide which */
	/* monotone polygon to split if */
	/* there are several other */
	/* polygons touching at the same */
	/* vertex  */
static vertexchain_t* vert;
	/* contains position of any vertex in */
	/* the monotone chain for the polygon */
static int* mon;

/* return a new mon structure from the table */
#define newmon() (++mon_idx)
/* return a new chain element from the table */
#define new_chain_element() (++chain_idx)

static void
convert (boxf bb, int flip, int ccw, pointf* pts)
{
    pts[0] = bb.LL;
    pts[2] = bb.UR;
    if (ccw) {
	pts[1].x = bb.UR.x;
	pts[1].y = bb.LL.y;
	pts[3].x = bb.LL.x;
	pts[3].y = bb.UR.y;
    }
    else {
	pts[1].x = bb.LL.x;
	pts[1].y = bb.UR.y;
	pts[3].x = bb.UR.x;
	pts[3].y = bb.LL.y;
    }
    if (flip) {
	int i;
	for (i = 0; i < NPOINTS; i++) {
	    double tmp = pts[i].y;
	    pts[i].y = pts[i].x;
	    pts[i].x = -tmp;
	}
    }
}

static int
store (segment_t* seg, int first, pointf* pts)
{
    int i, last = first + NPOINTS - 1;
    int j = 0;

    for (i = first; i <= last; i++, j++) {
	if (i == first) {
	    seg[i].next = first+1;
	    seg[i].prev = last;
	}
	else if (i == last) {
	    seg[i].next = first;
	    seg[i].prev = last-1;
	}
	else {
	    seg[i].next = i+1;
	    seg[i].prev = i-1;
	}
	seg[i].is_inserted = FALSE;
	seg[seg[i].prev].v1 = seg[i].v0 = pts[j];
    }
    return (last+1);
}

static void
genSegments (cell* cells, int ncells, boxf bb, segment_t* seg, int flip)
{
    int j = 0, i = 1;
    pointf pts[4];

    convert (bb, flip, 1, pts);
    i = store (seg, i, pts);
    for (j = 0; j < ncells; j++) {
	convert (cells[j].bb, flip, 0, pts);
	i = store (seg, i, pts);
    }
}

/* Generate a random permutation of the segments 1..n */
static void 
generateRandomOrdering(int n, int* permute)
{
    int i, j, tmp;
    for (i = 0; i <= n; i++) permute[i] = i;

    for (i = 1; i <= n; i++) {
	j = i + drand48() * (n + 1 - i);
	if (j != i) {
	    tmp = permute[i];
	    permute [i] = permute[j];
            permute [j] = tmp;
	}
    }
}

/* Function returns TRUE if the trapezoid lies inside the polygon */
static int 
inside_polygon (trap_t *t, segment_t* seg)
{
  int rseg = t->rseg;

  if (t->state == ST_INVALID)
    return 0;

  if ((t->lseg <= 0) || (t->rseg <= 0))
    return 0;
  
  if (((t->u0 <= 0) && (t->u1 <= 0)) || 
      ((t->d0 <= 0) && (t->d1 <= 0))) /* triangle */
    return (_greater_than(&seg[rseg].v1, &seg[rseg].v0));
  
  return 0;
}

static double
get_angle (pointf *vp0, pointf *vpnext, pointf *vp1)
{
  pointf v0, v1;
  
  v0.x = vpnext->x - vp0->x;
  v0.y = vpnext->y - vp0->y;

  v1.x = vp1->x - vp0->x;
  v1.y = vp1->y - vp0->y;

  if (CROSS_SINE(v0, v1) >= 0)	/* sine is positive */
    return DOT(v0, v1)/LENGTH(v0)/LENGTH(v1);
  else
    return (-1.0 * DOT(v0, v1)/LENGTH(v0)/LENGTH(v1) - 2);
}

/* (v0, v1) is the new diagonal to be added to the polygon. Find which */
/* chain to use and return the positions of v0 and v1 in p and q */ 
static int
get_vertex_positions (int v0, int v1, int *ip, int *iq)
{
  vertexchain_t *vp0, *vp1;
  register int i;
  double angle, temp;
  int tp, tq;

  vp0 = &vert[v0];
  vp1 = &vert[v1];
  
  /* p is identified as follows. Scan from (v0, v1) rightwards till */
  /* you hit the first segment starting from v0. That chain is the */
  /* chain of our interest */
  
  angle = -4.0;
  for (i = 0; i < 4; i++)
    {
      if (vp0->vnext[i] <= 0)
	continue;
      if ((temp = get_angle(&vp0->pt, &(vert[vp0->vnext[i]].pt), 
			    &vp1->pt)) > angle)
	{
	  angle = temp;
	  tp = i;
	}
    }

  *ip = tp;

  /* Do similar actions for q */

  angle = -4.0;
  for (i = 0; i < 4; i++)
    {
      if (vp1->vnext[i] <= 0)
	continue;      
      if ((temp = get_angle(&vp1->pt, &(vert[vp1->vnext[i]].pt), 
			    &vp0->pt)) > angle)
	{
	  angle = temp;
	  tq = i;
	}
    }

  *iq = tq;

  return 0;
}

/* v0 and v1 are specified in anti-clockwise order with respect to 
 * the current monotone polygon mcur. Split the current polygon into 
 * two polygons using the diagonal (v0, v1) 
 */
static int 
make_new_monotone_poly (int mcur, int v0, int v1)
{
  int p, q, ip, iq;
  int mnew = newmon();
  int i, j, nf0, nf1;
  vertexchain_t *vp0, *vp1;
  
  vp0 = &vert[v0];
  vp1 = &vert[v1];

  get_vertex_positions(v0, v1, &ip, &iq);

  p = vp0->vpos[ip];
  q = vp1->vpos[iq];

  /* At this stage, we have got the positions of v0 and v1 in the */
  /* desired chain. Now modify the linked lists */

  i = new_chain_element();	/* for the new list */
  j = new_chain_element();

  mchain[i].vnum = v0;
  mchain[j].vnum = v1;

  mchain[i].next = mchain[p].next;
  mchain[mchain[p].next].prev = i;
  mchain[i].prev = j;
  mchain[j].next = i;
  mchain[j].prev = mchain[q].prev;
  mchain[mchain[q].prev].next = j;

  mchain[p].next = q;
  mchain[q].prev = p;

  nf0 = vp0->nextfree;
  nf1 = vp1->nextfree;

  vp0->vnext[ip] = v1;

  vp0->vpos[nf0] = i;
  vp0->vnext[nf0] = mchain[mchain[i].next].vnum;
  vp1->vpos[nf1] = j;
  vp1->vnext[nf1] = v0;

  vp0->nextfree++;
  vp1->nextfree++;

#ifdef DEBUG
  fprintf(stderr, "make_poly: mcur = %d, (v0, v1) = (%d, %d)\n", 
	  mcur, v0, v1);
  fprintf(stderr, "next posns = (p, q) = (%d, %d)\n", p, q);
#endif

  mon[mcur] = p;
  mon[mnew] = i;
  return mnew;
}

/* recursively visit all the trapezoids */
static int
traverse_polygon (int* visited, boxf* decomp, int size, segment_t* seg, trap_t* tr,
    int mcur, int trnum, int from, int flip, int dir)
{
  trap_t *t = &tr[trnum];
  int mnew;
  int v0, v1;
  int retval;
  int do_switch = FALSE;

  if ((trnum <= 0) || visited[trnum])
    return size;

  visited[trnum] = TRUE;
  
  if ((t->hi.y > t->lo.y) &&
      (seg[t->lseg].v0.x == seg[t->lseg].v1.x) &&
      (seg[t->rseg].v0.x == seg[t->rseg].v1.x)) {
      if (flip) {
          decomp[size].LL.x = t->lo.y;
          decomp[size].LL.y = -seg[t->rseg].v0.x;
          decomp[size].UR.x = t->hi.y;
          decomp[size].UR.y = -seg[t->lseg].v0.x;
      } else {
          decomp[size].LL.x = seg[t->lseg].v0.x;
          decomp[size].LL.y = t->lo.y;
          decomp[size].UR.x = seg[t->rseg].v0.x;
          decomp[size].UR.y = t->hi.y;
      }
      size++;
  }
  
  /* We have much more information available here. */
  /* rseg: goes upwards   */
  /* lseg: goes downwards */

  /* Initially assume that dir = TR_FROM_DN (from the left) */
  /* Switch v0 and v1 if necessary afterwards */


  /* special cases for triangles with cusps at the opposite ends. */
  /* take care of this first */
  if ((t->u0 <= 0) && (t->u1 <= 0))
    {
      if ((t->d0 > 0) && (t->d1 > 0)) /* downward opening triangle */
	{
	  v0 = tr[t->d1].lseg;
	  v1 = t->lseg;
	  if (from == t->d1)
	    {
	      do_switch = TRUE;
	      mnew = make_new_monotone_poly(mcur, v1, v0);
	      size = traverse_polygon (visited, decomp, size, seg, tr, mcur, t->d1, trnum, flip, TR_FROM_UP);
	      size = traverse_polygon (visited, decomp, size, seg, tr, mnew, t->d0, trnum, flip, TR_FROM_UP);	    
	    }
	  else
	    {
	      mnew = make_new_monotone_poly(mcur, v0, v1);
	      size = traverse_polygon (visited, decomp, size, seg, tr, mcur, t->d0, trnum, flip, TR_FROM_UP);
	      size = traverse_polygon (visited, decomp, size, seg, tr, mnew, t->d1, trnum, flip, TR_FROM_UP);
	    }
	}
      else
	{
	  retval = SP_NOSPLIT;	/* Just traverse all neighbours */
	  size = traverse_polygon (visited, decomp, size, seg, tr, mcur, t->u0, trnum, flip, TR_FROM_DN);
	  size = traverse_polygon (visited, decomp, size, seg, tr, mcur, t->u1, trnum, flip, TR_FROM_DN);
	  size = traverse_polygon (visited, decomp, size, seg, tr, mcur, t->d0, trnum, flip, TR_FROM_UP);
	  size = traverse_polygon (visited, decomp, size, seg, tr, mcur, t->d1, trnum, flip, TR_FROM_UP);
	}
    }
  
  else if ((t->d0 <= 0) && (t->d1 <= 0))
    {
      if ((t->u0 > 0) && (t->u1 > 0)) /* upward opening triangle */
	{
	  v0 = t->rseg;
	  v1 = tr[t->u0].rseg;
	  if (from == t->u1)
	    {
	      do_switch = TRUE;
	      mnew = make_new_monotone_poly(mcur, v1, v0);
	      size = traverse_polygon (visited, decomp, size, seg, tr, mcur, t->u1, trnum, flip, TR_FROM_DN);
	      size = traverse_polygon (visited, decomp, size, seg, tr, mnew, t->u0, trnum, flip, TR_FROM_DN);	    
	    }
	  else
	    {
	      mnew = make_new_monotone_poly(mcur, v0, v1);
	      size = traverse_polygon (visited, decomp, size, seg, tr, mcur, t->u0, trnum, flip, TR_FROM_DN);
	      size = traverse_polygon (visited, decomp, size, seg, tr, mnew, t->u1, trnum, flip, TR_FROM_DN);
	    }
	}
      else
	{
	  retval = SP_NOSPLIT;	/* Just traverse all neighbours */
	  size = traverse_polygon (visited, decomp, size, seg, tr, mcur, t->u0, trnum, flip, TR_FROM_DN);
	  size = traverse_polygon (visited, decomp, size, seg, tr, mcur, t->u1, trnum, flip, TR_FROM_DN);
	  size = traverse_polygon (visited, decomp, size, seg, tr, mcur, t->d0, trnum, flip, TR_FROM_UP);
	  size = traverse_polygon (visited, decomp, size, seg, tr, mcur, t->d1, trnum, flip, TR_FROM_UP);
	}
    }
  
  else if ((t->u0 > 0) && (t->u1 > 0)) 
    {
      if ((t->d0 > 0) && (t->d1 > 0)) /* downward + upward cusps */
	{
	  v0 = tr[t->d1].lseg;
	  v1 = tr[t->u0].rseg;
	  retval = SP_2UP_2DN;
	  if (((dir == TR_FROM_DN) && (t->d1 == from)) ||
	      ((dir == TR_FROM_UP) && (t->u1 == from)))
	    {
	      do_switch = TRUE;
	      mnew = make_new_monotone_poly(mcur, v1, v0);
	      size = traverse_polygon (visited, decomp, size, seg, tr, mcur, t->u1, trnum, flip, TR_FROM_DN);
	      size = traverse_polygon (visited, decomp, size, seg, tr, mcur, t->d1, trnum, flip, TR_FROM_UP);
	      size = traverse_polygon (visited, decomp, size, seg, tr, mnew, t->u0, trnum, flip, TR_FROM_DN);
	      size = traverse_polygon (visited, decomp, size, seg, tr, mnew, t->d0, trnum, flip, TR_FROM_UP);
	    }
	  else
	    {
	      mnew = make_new_monotone_poly(mcur, v0, v1);
	      size = traverse_polygon (visited, decomp, size, seg, tr, mcur, t->u0, trnum, flip, TR_FROM_DN);
	      size = traverse_polygon (visited, decomp, size, seg, tr, mcur, t->d0, trnum, flip, TR_FROM_UP);
	      size = traverse_polygon (visited, decomp, size, seg, tr, mnew, t->u1, trnum, flip, TR_FROM_DN);
	      size = traverse_polygon (visited, decomp, size, seg, tr, mnew, t->d1, trnum, flip, TR_FROM_UP);	      
	    }
	}
      else			/* only downward cusp */
	{
	  if (_equal_to(&t->lo, &seg[t->lseg].v1))
	    {
	      v0 = tr[t->u0].rseg;
	      v1 = seg[t->lseg].next;

	      retval = SP_2UP_LEFT;
	      if ((dir == TR_FROM_UP) && (t->u0 == from))
		{
		  do_switch = TRUE;
		  mnew = make_new_monotone_poly(mcur, v1, v0);
		  size = traverse_polygon (visited, decomp, size, seg, tr, mcur, t->u0, trnum, flip, TR_FROM_DN);
		  size = traverse_polygon (visited, decomp, size, seg, tr, mnew, t->d0, trnum, flip, TR_FROM_UP);
		  size = traverse_polygon (visited, decomp, size, seg, tr, mnew, t->u1, trnum, flip, TR_FROM_DN);
		  size = traverse_polygon (visited, decomp, size, seg, tr, mnew, t->d1, trnum, flip, TR_FROM_UP);
		}
	      else
		{
		  mnew = make_new_monotone_poly(mcur, v0, v1);
		  size = traverse_polygon (visited, decomp, size, seg, tr, mcur, t->u1, trnum, flip, TR_FROM_DN);
		  size = traverse_polygon (visited, decomp, size, seg, tr, mcur, t->d0, trnum, flip, TR_FROM_UP);
		  size = traverse_polygon (visited, decomp, size, seg, tr, mcur, t->d1, trnum, flip, TR_FROM_UP);
		  size = traverse_polygon (visited, decomp, size, seg, tr, mnew, t->u0, trnum, flip, TR_FROM_DN);
		}
	    }
	  else
	    {
	      v0 = t->rseg;
	      v1 = tr[t->u0].rseg;	
	      retval = SP_2UP_RIGHT;
	      if ((dir == TR_FROM_UP) && (t->u1 == from))
		{
		  do_switch = TRUE;
		  mnew = make_new_monotone_poly(mcur, v1, v0);
		  size = traverse_polygon (visited, decomp, size, seg, tr, mcur, t->u1, trnum, flip, TR_FROM_DN);
		  size = traverse_polygon (visited, decomp, size, seg, tr, mnew, t->d1, trnum, flip, TR_FROM_UP);
		  size = traverse_polygon (visited, decomp, size, seg, tr, mnew, t->d0, trnum, flip, TR_FROM_UP);
		  size = traverse_polygon (visited, decomp, size, seg, tr, mnew, t->u0, trnum, flip, TR_FROM_DN);
		}
	      else
		{
		  mnew = make_new_monotone_poly(mcur, v0, v1);
		  size = traverse_polygon (visited, decomp, size, seg, tr, mcur, t->u0, trnum, flip, TR_FROM_DN);
		  size = traverse_polygon (visited, decomp, size, seg, tr, mcur, t->d0, trnum, flip, TR_FROM_UP);
		  size = traverse_polygon (visited, decomp, size, seg, tr, mcur, t->d1, trnum, flip, TR_FROM_UP);
		  size = traverse_polygon (visited, decomp, size, seg, tr, mnew, t->u1, trnum, flip, TR_FROM_DN);
		}
	    }
	}
    }
  else if ((t->u0 > 0) || (t->u1 > 0)) /* no downward cusp */
    {
      if ((t->d0 > 0) && (t->d1 > 0)) /* only upward cusp */
	{
	  if (_equal_to(&t->hi, &seg[t->lseg].v0))
	    {
	      v0 = tr[t->d1].lseg;
	      v1 = t->lseg;
	      retval = SP_2DN_LEFT;
	      if (!((dir == TR_FROM_DN) && (t->d0 == from)))
		{
		  do_switch = TRUE;
		  mnew = make_new_monotone_poly(mcur, v1, v0);
		  size = traverse_polygon (visited, decomp, size, seg, tr, mcur, t->u1, trnum, flip, TR_FROM_DN);
		  size = traverse_polygon (visited, decomp, size, seg, tr, mcur, t->d1, trnum, flip, TR_FROM_UP);
		  size = traverse_polygon (visited, decomp, size, seg, tr, mcur, t->u0, trnum, flip, TR_FROM_DN);
		  size = traverse_polygon (visited, decomp, size, seg, tr, mnew, t->d0, trnum, flip, TR_FROM_UP);
		}
	      else
		{
		  mnew = make_new_monotone_poly(mcur, v0, v1);
		  size = traverse_polygon (visited, decomp, size, seg, tr, mcur, t->d0, trnum, flip, TR_FROM_UP);
		  size = traverse_polygon (visited, decomp, size, seg, tr, mnew, t->u0, trnum, flip, TR_FROM_DN);
		  size = traverse_polygon (visited, decomp, size, seg, tr, mnew, t->u1, trnum, flip, TR_FROM_DN);
		  size = traverse_polygon (visited, decomp, size, seg, tr, mnew, t->d1, trnum, flip, TR_FROM_UP);	      
		}
	    }
	  else
	    {
	      v0 = tr[t->d1].lseg;
	      v1 = seg[t->rseg].next;

	      retval = SP_2DN_RIGHT;	    
	      if ((dir == TR_FROM_DN) && (t->d1 == from))
		{
		  do_switch = TRUE;
		  mnew = make_new_monotone_poly(mcur, v1, v0);
		  size = traverse_polygon (visited, decomp, size, seg, tr, mcur, t->d1, trnum, flip, TR_FROM_UP);
		  size = traverse_polygon (visited, decomp, size, seg, tr, mnew, t->u1, trnum, flip, TR_FROM_DN);
		  size = traverse_polygon (visited, decomp, size, seg, tr, mnew, t->u0, trnum, flip, TR_FROM_DN);
		  size = traverse_polygon (visited, decomp, size, seg, tr, mnew, t->d0, trnum, flip, TR_FROM_UP);
		}
	      else
		{
		  mnew = make_new_monotone_poly(mcur, v0, v1);
		  size = traverse_polygon (visited, decomp, size, seg, tr, mcur, t->u0, trnum, flip, TR_FROM_DN);
		  size = traverse_polygon (visited, decomp, size, seg, tr, mcur, t->d0, trnum, flip, TR_FROM_UP);
		  size = traverse_polygon (visited, decomp, size, seg, tr, mcur, t->u1, trnum, flip, TR_FROM_DN);
		  size = traverse_polygon (visited, decomp, size, seg, tr, mnew, t->d1, trnum, flip, TR_FROM_UP);
		}
	    }
	}
      else			/* no cusp */
	{
	  if (_equal_to(&t->hi, &seg[t->lseg].v0) &&
	      _equal_to(&t->lo, &seg[t->rseg].v0))
	    {
	      v0 = t->rseg;
	      v1 = t->lseg;
	      retval = SP_SIMPLE_LRDN;
	      if (dir == TR_FROM_UP)
		{
		  do_switch = TRUE;
		  mnew = make_new_monotone_poly(mcur, v1, v0);
		  size = traverse_polygon (visited, decomp, size, seg, tr, mcur, t->u0, trnum, flip, TR_FROM_DN);
		  size = traverse_polygon (visited, decomp, size, seg, tr, mcur, t->u1, trnum, flip, TR_FROM_DN);
		  size = traverse_polygon (visited, decomp, size, seg, tr, mnew, t->d1, trnum, flip, TR_FROM_UP);
		  size = traverse_polygon (visited, decomp, size, seg, tr, mnew, t->d0, trnum, flip, TR_FROM_UP);
		}
	      else
		{
		  mnew = make_new_monotone_poly(mcur, v0, v1);
		  size = traverse_polygon (visited, decomp, size, seg, tr, mcur, t->d1, trnum, flip, TR_FROM_UP);
		  size = traverse_polygon (visited, decomp, size, seg, tr, mcur, t->d0, trnum, flip, TR_FROM_UP);
		  size = traverse_polygon (visited, decomp, size, seg, tr, mnew, t->u0, trnum, flip, TR_FROM_DN);
		  size = traverse_polygon (visited, decomp, size, seg, tr, mnew, t->u1, trnum, flip, TR_FROM_DN);
		}
	    }
	  else if (_equal_to(&t->hi, &seg[t->rseg].v1) &&
		   _equal_to(&t->lo, &seg[t->lseg].v1))
	    {
	      v0 = seg[t->rseg].next;
	      v1 = seg[t->lseg].next;

	      retval = SP_SIMPLE_LRUP;
	      if (dir == TR_FROM_UP)
		{
		  do_switch = TRUE;
		  mnew = make_new_monotone_poly(mcur, v1, v0);
		  size = traverse_polygon (visited, decomp, size, seg, tr, mcur, t->u0, trnum, flip, TR_FROM_DN);
		  size = traverse_polygon (visited, decomp, size, seg, tr, mcur, t->u1, trnum, flip, TR_FROM_DN);
		  size = traverse_polygon (visited, decomp, size, seg, tr, mnew, t->d1, trnum, flip, TR_FROM_UP);
		  size = traverse_polygon (visited, decomp, size, seg, tr, mnew, t->d0, trnum, flip, TR_FROM_UP);
		}
	      else
		{
		  mnew = make_new_monotone_poly(mcur, v0, v1);
		  size = traverse_polygon (visited, decomp, size, seg, tr, mcur, t->d1, trnum, flip, TR_FROM_UP);
		  size = traverse_polygon (visited, decomp, size, seg, tr, mcur, t->d0, trnum, flip, TR_FROM_UP);
		  size = traverse_polygon (visited, decomp, size, seg, tr, mnew, t->u0, trnum, flip, TR_FROM_DN);
		  size = traverse_polygon (visited, decomp, size, seg, tr, mnew, t->u1, trnum, flip, TR_FROM_DN);
		}
	    }
	  else			/* no split possible */
	    {
	      retval = SP_NOSPLIT;
	      size = traverse_polygon (visited, decomp, size, seg, tr, mcur, t->u0, trnum, flip, TR_FROM_DN);
	      size = traverse_polygon (visited, decomp, size, seg, tr, mcur, t->d0, trnum, flip, TR_FROM_UP);
	      size = traverse_polygon (visited, decomp, size, seg, tr, mcur, t->u1, trnum, flip, TR_FROM_DN);
	      size = traverse_polygon (visited, decomp, size, seg, tr, mcur, t->d1, trnum, flip, TR_FROM_UP);	      	      
	    }
	}
    }

  return size;
}

static int
monotonate_trapezoids(int nsegs, segment_t*seg, trap_t* tr, 
    int flip, boxf* decomp)
{
    int i, size;
    int tr_start;
    int tr_size = TRSIZE(nsegs);
    int* visited = N_NEW(tr_size,int);

    mchain = N_NEW(tr_size, monchain_t);
    vert = N_NEW(nsegs+1,vertexchain_t); 
    mon = N_NEW(nsegs, int);    

  /* First locate a trapezoid which lies inside the polygon */
  /* and which is triangular */
    for (i = 0; i < TRSIZE(nsegs); i++)
	if (inside_polygon(&tr[i], seg)) break;
    tr_start = i;
  
  /* Initialise the mon data-structure and start spanning all the */
  /* trapezoids within the polygon */

    for (i = 1; i <= nsegs; i++) {
	mchain[i].prev = seg[i].prev;
	mchain[i].next = seg[i].next;
	mchain[i].vnum = i;
	vert[i].pt = seg[i].v0;
	vert[i].vnext[0] = seg[i].next; /* next vertex */
	vert[i].vpos[0] = i;	/* locn. of next vertex */
	vert[i].nextfree = 1;
    }

    chain_idx = nsegs;
    mon_idx = 0;
    mon[0] = 1;			/* position of any vertex in the first */
				/* chain  */
  
  /* traverse the polygon */
    if (tr[tr_start].u0 > 0)
	size = traverse_polygon (visited, decomp, 0, seg, tr, 0, tr_start, tr[tr_start].u0, flip, TR_FROM_UP);
    else if (tr[tr_start].d0 > 0)
	size = traverse_polygon (visited, decomp, 0, seg, tr, 0, tr_start, tr[tr_start].d0, flip, TR_FROM_DN);
  
    free (visited);
    free (mchain);
    free (vert);
    free (mon);

  /* return the number of rects created */
  return size;
}

static int 
rectIntersect (boxf *d, const boxf *r0, const boxf *r1)
{
    double t;

    t = (r0->LL.x > r1->LL.x) ? r0->LL.x : r1->LL.x;
    d->UR.x = (r0->UR.x < r1->UR.x) ? r0->UR.x : r1->UR.x;
    d->LL.x = t;
    
    t = (r0->LL.y > r1->LL.y) ? r0->LL.y : r1->LL.y;
    d->UR.y = (r0->UR.y < r1->UR.y) ? r0->UR.y : r1->UR.y;
    d->LL.y = t;

    if ((d->LL.x >= d->UR.x) ||
        (d->LL.y >= d->UR.y))
    return 0;

    return 1;
}

#ifdef DEBUG
static void
dumpTrap (trap_t* tr, int n)
{
    int i;
    for (i = 1; i <= n; i++) {
      tr++;
      fprintf (stderr, "%d : %d %d (%f,%f) (%f,%f) %d %d %d %d\n", i,
         tr->lseg, tr->rseg, tr->hi.x, tr->hi.y, tr->lo.x, tr->lo.y,
         tr->u0, tr->u1,  tr->d0, tr->d1);
      fprintf (stderr, "    %d %d %d %d\n", tr->sink, tr->usave,
         tr->uside, tr->state);
    }
    fprintf (stderr, "====\n");
}

static void
dumpSegs (segment_t* sg, int n)
{
    int i;
    for (i = 1; i <= n; i++) {
      sg++;
      fprintf (stderr, "%d : (%f,%f) (%f,%f) %d %d %d %d %d\n", i,
         sg->v0.x, sg->v0.y, sg->v1.x, sg->v1.y,
         sg->is_inserted, sg->root0,  sg->root1, sg->next, sg->prev);
    }
    fprintf (stderr, "====\n");
}
#endif

boxf*
partition (cell* cells, int ncells, int* nrects, boxf bb)
{
    int nsegs = 4*(ncells+1);
    segment_t* segs = N_GNEW(nsegs+1, segment_t);
    int* permute = N_NEW(nsegs+1, int);
    int hd_size, vd_size;
    int i, j, cnt = 0;
    boxf* rs;
    int ntraps = TRSIZE(nsegs);
    trap_t* trs = N_GNEW(ntraps, trap_t);
    boxf* hor_decomp = N_NEW(ntraps, boxf);
    boxf* vert_decomp = N_NEW(ntraps, boxf);
    int nt;

    /* fprintf (stderr, "cells = %d segs = %d traps = %d\n", ncells, nsegs, ntraps);  */
    genSegments (cells, ncells, bb, segs, 0);
#if 0
fprintf (stderr, "%d\n\n", ncells+1);
for (i = 1; i<= nsegs; i++) {
  if (i%4 == 1) fprintf(stderr, "4\n");
  fprintf (stderr, "%f %f\n", segs[i].v0.x, segs[i].v0.y);
  if (i%4 == 0) fprintf(stderr, "\n");
}
#endif
    srand48(173);
    generateRandomOrdering (nsegs, permute);
    nt = construct_trapezoids(nsegs, segs, permute, ntraps, trs);
    /* fprintf (stderr, "hor traps = %d\n", nt); */
    hd_size = monotonate_trapezoids (nsegs, segs, trs, 0, hor_decomp);

    genSegments (cells, ncells, bb, segs, 1);
    generateRandomOrdering (nsegs, permute);
    nt = construct_trapezoids(nsegs, segs, permute, ntraps, trs);
    /* fprintf (stderr, "ver traps = %d\n", nt); */
    vd_size = monotonate_trapezoids (nsegs, segs, trs, 1, vert_decomp);

    rs = N_NEW (hd_size*vd_size, boxf);
    for (i=0; i<vd_size; i++) 
	for (j=0; j<hd_size; j++)
	    if (rectIntersect(&rs[cnt], &vert_decomp[i], &hor_decomp[j]))
		cnt++;

    rs = RALLOC (cnt, rs, boxf);
    free (segs);
    free (permute);
    free (trs);
    free (hor_decomp);
    free (vert_decomp);
    *nrects = cnt;
    return rs;
}
