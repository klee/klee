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

#include <string.h>
#include <assert.h>
#include <stdio.h>
#define __USE_ISOC99
#include <math.h>
#include <geom.h>
#include <logic.h>
#include <memory.h>
#include <trap.h>

#ifndef HAVE_LOG2
#define log2(x)  (log(x)/log(2))
#endif

/* Node types */

#define T_X     1
#define T_Y     2
#define T_SINK  3

#define FIRSTPT 1       /* checking whether pt. is inserted */
#define LASTPT  2

#define S_LEFT 1        /* for merge-direction */
#define S_RIGHT 2

#define INF 1<<30

#define CROSS(v0, v1, v2) (((v1).x - (v0).x)*((v2).y - (v0).y) - \
               ((v1).y - (v0).y)*((v2).x - (v0).x))

typedef struct {
  int nodetype;         /* Y-node or S-node */
  int segnum;
  pointf yval;
  int trnum;
  int parent;           /* doubly linked DAG */
  int left, right;      /* children */
} qnode_t;

/* static int chain_idx, op_idx, mon_idx; */
static int q_idx;
static int tr_idx;
static int QSIZE;
static int TRSIZE;

/* Return a new node to be added into the query tree */
static int newnode()
{
    if (q_idx < QSIZE)
	return q_idx++;
    else {
	fprintf(stderr, "newnode: Query-table overflow\n");
	assert(0);
	return -1;
    }
}

/* Return a free trapezoid */
static int newtrap(trap_t* tr)
{
    if (tr_idx < TRSIZE) {
	tr[tr_idx].lseg = -1;
	tr[tr_idx].rseg = -1;
	tr[tr_idx].state = ST_VALID;
	return tr_idx++;
    }
    else {
	fprintf(stderr, "newtrap: Trapezoid-table overflow %d\n", tr_idx);
	assert(0);
	return -1;
    }
}

/* Return the maximum of the two points into the yval structure */
static int _max (pointf *yval, pointf *v0, pointf *v1)
{
  if (v0->y > v1->y + C_EPS)
    *yval = *v0;
  else if (FP_EQUAL(v0->y, v1->y))
    {
      if (v0->x > v1->x + C_EPS)
	*yval = *v0;
      else
	*yval = *v1;
    }
  else
    *yval = *v1;
  
  return 0;
}

/* Return the minimum of the two points into the yval structure */
static int _min (pointf *yval, pointf *v0, pointf *v1)
{
  if (v0->y < v1->y - C_EPS)
    *yval = *v0;
  else if (FP_EQUAL(v0->y, v1->y))
    {
      if (v0->x < v1->x)
	*yval = *v0;
      else
	*yval = *v1;
    }
  else
    *yval = *v1;
  
  return 0;
}

static int _greater_than_equal_to (pointf *v0, pointf *v1)
{
  if (v0->y > v1->y + C_EPS)
    return TRUE;
  else if (v0->y < v1->y - C_EPS)
    return FALSE;
  else
    return (v0->x >= v1->x);
}

static int _less_than (pointf *v0, pointf *v1)
{
  if (v0->y < v1->y - C_EPS)
    return TRUE;
  else if (v0->y > v1->y + C_EPS)
    return FALSE;
  else
    return (v0->x < v1->x);
}

/* Initilialise the query structure (Q) and the trapezoid table (T) 
 * when the first segment is added to start the trapezoidation. The
 * query-tree starts out with 4 trapezoids, one S-node and 2 Y-nodes
 *    
 *                4
 *   -----------------------------------
 *  		  \
 *  	1	   \        2
 *  		    \
 *   -----------------------------------
 *                3
 */

static int 
init_query_structure(int segnum, segment_t* seg, trap_t* tr, qnode_t* qs)
{
  int i1, i2, i3, i4, i5, i6, i7, root;
  int t1, t2, t3, t4;
  segment_t *s = &seg[segnum];

  i1 = newnode();
  qs[i1].nodetype = T_Y;
  _max(&qs[i1].yval, &s->v0, &s->v1); /* root */
  root = i1;

  qs[i1].right = i2 = newnode();
  qs[i2].nodetype = T_SINK;
  qs[i2].parent = i1;

  qs[i1].left = i3 = newnode();
  qs[i3].nodetype = T_Y;
  _min(&qs[i3].yval, &s->v0, &s->v1); /* root */
  qs[i3].parent = i1;
  
  qs[i3].left = i4 = newnode();
  qs[i4].nodetype = T_SINK;
  qs[i4].parent = i3;
  
  qs[i3].right = i5 = newnode();
  qs[i5].nodetype = T_X;
  qs[i5].segnum = segnum;
  qs[i5].parent = i3;
  
  qs[i5].left = i6 = newnode();
  qs[i6].nodetype = T_SINK;
  qs[i6].parent = i5;

  qs[i5].right = i7 = newnode();
  qs[i7].nodetype = T_SINK;
  qs[i7].parent = i5;

  t1 = newtrap(tr);		/* middle left */
  t2 = newtrap(tr);		/* middle right */
  t3 = newtrap(tr);		/* bottom-most */
  t4 = newtrap(tr);		/* topmost */

  tr[t1].hi = tr[t2].hi = tr[t4].lo = qs[i1].yval;
  tr[t1].lo = tr[t2].lo = tr[t3].hi = qs[i3].yval;
  tr[t4].hi.y = (double) (INF);
  tr[t4].hi.x = (double) (INF);
  tr[t3].lo.y = (double) -1* (INF);
  tr[t3].lo.x = (double) -1* (INF);
  tr[t1].rseg = tr[t2].lseg = segnum;
  tr[t1].u0 = tr[t2].u0 = t4;
  tr[t1].d0 = tr[t2].d0 = t3;
  tr[t4].d0 = tr[t3].u0 = t1;
  tr[t4].d1 = tr[t3].u1 = t2;
  
  tr[t1].sink = i6;
  tr[t2].sink = i7;
  tr[t3].sink = i4;
  tr[t4].sink = i2;

  tr[t1].state = tr[t2].state = ST_VALID;
  tr[t3].state = tr[t4].state = ST_VALID;

  qs[i2].trnum = t4;
  qs[i4].trnum = t3;
  qs[i6].trnum = t1;
  qs[i7].trnum = t2;

  s->is_inserted = TRUE;
  return root;
}

/* Retun TRUE if the vertex v is to the left of line segment no.
 * segnum. Takes care of the degenerate cases when both the vertices
 * have the same y--cood, etc.
 */
static int
is_left_of (int segnum, segment_t* seg, pointf *v)
{
  segment_t *s = &seg[segnum];
  double area;
  
  if (_greater_than(&s->v1, &s->v0)) /* seg. going upwards */
    {
      if (FP_EQUAL(s->v1.y, v->y))
	{
	  if (v->x < s->v1.x)
	    area = 1.0;
	  else
	    area = -1.0;
	}
      else if (FP_EQUAL(s->v0.y, v->y))
	{
	  if (v->x < s->v0.x)
	    area = 1.0;
	  else
	    area = -1.0;
	}
      else
	area = CROSS(s->v0, s->v1, (*v));
    }
  else				/* v0 > v1 */
    {
      if (FP_EQUAL(s->v1.y, v->y))
	{
	  if (v->x < s->v1.x)
	    area = 1.0;
	  else
	    area = -1.0;
	}
      else if (FP_EQUAL(s->v0.y, v->y))
	{
	  if (v->x < s->v0.x)
	    area = 1.0;
	  else
	    area = -1.0;
	}
      else
	area = CROSS(s->v1, s->v0, (*v));
    }
  
  if (area > 0.0)
    return TRUE;
  else 
    return FALSE;
}

/* Returns true if the corresponding endpoint of the given segment is */
/* already inserted into the segment tree. Use the simple test of */
/* whether the segment which shares this endpoint is already inserted */
static int inserted (int segnum, segment_t* seg, int whichpt)
{
  if (whichpt == FIRSTPT)
    return seg[seg[segnum].prev].is_inserted;
  else
    return seg[seg[segnum].next].is_inserted;
}

/* This is query routine which determines which trapezoid does the 
 * point v lie in. The return value is the trapezoid number. 
 */
static int 
locate_endpoint (pointf *v, pointf *vo, int r, segment_t* seg, qnode_t* qs)
{
  qnode_t *rptr = &qs[r];
  
  switch (rptr->nodetype) {
    case T_SINK:
      return rptr->trnum;
      
    case T_Y:
      if (_greater_than(v, &rptr->yval)) /* above */
	return locate_endpoint(v, vo, rptr->right, seg, qs);
      else if (_equal_to(v, &rptr->yval)) /* the point is already */
	{			          /* inserted. */
	  if (_greater_than(vo, &rptr->yval)) /* above */
	    return locate_endpoint(v, vo, rptr->right, seg, qs);
	  else 
	    return locate_endpoint(v, vo, rptr->left, seg, qs); /* below */	    
	}
      else
	return locate_endpoint(v, vo, rptr->left, seg, qs); /* below */

    case T_X:
      if (_equal_to(v, &seg[rptr->segnum].v0) || 
	       _equal_to(v, &seg[rptr->segnum].v1))
	{
	  if (FP_EQUAL(v->y, vo->y)) /* horizontal segment */
	    {
	      if (vo->x < v->x)
		return locate_endpoint(v, vo, rptr->left, seg, qs); /* left */
	      else
		return locate_endpoint(v, vo, rptr->right, seg, qs); /* right */
	    }

	  else if (is_left_of(rptr->segnum, seg, vo))
	    return locate_endpoint(v, vo, rptr->left, seg, qs); /* left */
	  else
	    return locate_endpoint(v, vo, rptr->right, seg, qs); /* right */
	}
      else if (is_left_of(rptr->segnum, seg, v))
	return locate_endpoint(v, vo, rptr->left, seg, qs); /* left */
      else
	return locate_endpoint(v, vo, rptr->right, seg, qs); /* right */	

    default:
      fprintf(stderr, "unexpected case in locate_endpoint\n");
      assert (0);
      break;
    }
    return 1; /* stop warning */
}

/* Thread in the segment into the existing trapezoidation. The 
 * limiting trapezoids are given by tfirst and tlast (which are the
 * trapezoids containing the two endpoints of the segment. Merges all
 * possible trapezoids which flank this segment and have been recently
 * divided because of its insertion
 */
static void
merge_trapezoids (int segnum, int tfirst, int tlast, int side, trap_t* tr,
    qnode_t* qs)
{
  int t, tnext, cond;
  int ptnext;

  /* First merge polys on the LHS */
  t = tfirst;
  while ((t > 0) && _greater_than_equal_to(&tr[t].lo, &tr[tlast].lo))
    {
      if (side == S_LEFT)
	cond = ((((tnext = tr[t].d0) > 0) && (tr[tnext].rseg == segnum)) ||
		(((tnext = tr[t].d1) > 0) && (tr[tnext].rseg == segnum)));
      else
	cond = ((((tnext = tr[t].d0) > 0) && (tr[tnext].lseg == segnum)) ||
		(((tnext = tr[t].d1) > 0) && (tr[tnext].lseg == segnum)));
      
      if (cond)
	{
	  if ((tr[t].lseg == tr[tnext].lseg) &&
	      (tr[t].rseg == tr[tnext].rseg)) /* good neighbours */
	    {			              /* merge them */
	      /* Use the upper node as the new node i.e. t */
	      
	      ptnext = qs[tr[tnext].sink].parent;
	      
	      if (qs[ptnext].left == tr[tnext].sink)
		qs[ptnext].left = tr[t].sink;
	      else
		qs[ptnext].right = tr[t].sink;	/* redirect parent */
	      
	      
	      /* Change the upper neighbours of the lower trapezoids */
	      
	      if ((tr[t].d0 = tr[tnext].d0) > 0) {
		if (tr[tr[t].d0].u0 == tnext)
		  tr[tr[t].d0].u0 = t;
		else if (tr[tr[t].d0].u1 == tnext)
		  tr[tr[t].d0].u1 = t;
	      }
	      
	      if ((tr[t].d1 = tr[tnext].d1) > 0) {
		if (tr[tr[t].d1].u0 == tnext)
		  tr[tr[t].d1].u0 = t;
		else if (tr[tr[t].d1].u1 == tnext)
		  tr[tr[t].d1].u1 = t;
	      }
	      
	      tr[t].lo = tr[tnext].lo;
	      tr[tnext].state = ST_INVALID; /* invalidate the lower */
				            /* trapezium */
	    }
	  else		    /* not good neighbours */
	    t = tnext;
	}
      else		    /* do not satisfy the outer if */
	t = tnext;
      
    } /* end-while */
       
}

/* Add in the new segment into the trapezoidation and update Q and T
 * structures. First locate the two endpoints of the segment in the
 * Q-structure. Then start from the topmost trapezoid and go down to
 * the  lower trapezoid dividing all the trapezoids in between .
 */
static int 
add_segment (int segnum, segment_t* seg, trap_t* tr, qnode_t* qs)
{
  segment_t s;
  int tu, tl, sk, tfirst, tlast;
  int tfirstr, tlastr, tfirstl, tlastl;
  int i1, i2, t, tn;
  pointf tpt;
  int tritop = 0, tribot = 0, is_swapped;
  int tmptriseg;

  s = seg[segnum];
  if (_greater_than(&s.v1, &s.v0)) /* Get higher vertex in v0 */
    {
      int tmp;
      tpt = s.v0;
      s.v0 = s.v1;
      s.v1 = tpt;
      tmp = s.root0;
      s.root0 = s.root1;
      s.root1 = tmp;
      is_swapped = TRUE;
    }
  else is_swapped = FALSE;

  if ((is_swapped) ? !inserted(segnum, seg, LASTPT) :
       !inserted(segnum, seg, FIRSTPT))     /* insert v0 in the tree */
    {
      int tmp_d;

      tu = locate_endpoint(&s.v0, &s.v1, s.root0, seg, qs);
      tl = newtrap(tr);		/* tl is the new lower trapezoid */
      tr[tl].state = ST_VALID;
      tr[tl] = tr[tu];
      tr[tu].lo.y = tr[tl].hi.y = s.v0.y;
      tr[tu].lo.x = tr[tl].hi.x = s.v0.x;
      tr[tu].d0 = tl;      
      tr[tu].d1 = 0;
      tr[tl].u0 = tu;
      tr[tl].u1 = 0;

      if (((tmp_d = tr[tl].d0) > 0) && (tr[tmp_d].u0 == tu))
	tr[tmp_d].u0 = tl;
      if (((tmp_d = tr[tl].d0) > 0) && (tr[tmp_d].u1 == tu))
	tr[tmp_d].u1 = tl;

      if (((tmp_d = tr[tl].d1) > 0) && (tr[tmp_d].u0 == tu))
	tr[tmp_d].u0 = tl;
      if (((tmp_d = tr[tl].d1) > 0) && (tr[tmp_d].u1 == tu))
	tr[tmp_d].u1 = tl;

      /* Now update the query structure and obtain the sinks for the */
      /* two trapezoids */ 
      
      i1 = newnode();		/* Upper trapezoid sink */
      i2 = newnode();		/* Lower trapezoid sink */
      sk = tr[tu].sink;
      
      qs[sk].nodetype = T_Y;
      qs[sk].yval = s.v0;
      qs[sk].segnum = segnum;	/* not really reqd ... maybe later */
      qs[sk].left = i2;
      qs[sk].right = i1;

      qs[i1].nodetype = T_SINK;
      qs[i1].trnum = tu;
      qs[i1].parent = sk;

      qs[i2].nodetype = T_SINK;
      qs[i2].trnum = tl;
      qs[i2].parent = sk;

      tr[tu].sink = i1;
      tr[tl].sink = i2;
      tfirst = tl;
    }
  else				/* v0 already present */
    {       /* Get the topmost intersecting trapezoid */
      tfirst = locate_endpoint(&s.v0, &s.v1, s.root0, seg, qs);
      tritop = 1;
    }


  if ((is_swapped) ? !inserted(segnum, seg, FIRSTPT) :
       !inserted(segnum, seg, LASTPT))     /* insert v1 in the tree */
    {
      int tmp_d;

      tu = locate_endpoint(&s.v1, &s.v0, s.root1, seg, qs);

      tl = newtrap(tr);		/* tl is the new lower trapezoid */
      tr[tl].state = ST_VALID;
      tr[tl] = tr[tu];
      tr[tu].lo.y = tr[tl].hi.y = s.v1.y;
      tr[tu].lo.x = tr[tl].hi.x = s.v1.x;
      tr[tu].d0 = tl;      
      tr[tu].d1 = 0;
      tr[tl].u0 = tu;
      tr[tl].u1 = 0;

      if (((tmp_d = tr[tl].d0) > 0) && (tr[tmp_d].u0 == tu))
	tr[tmp_d].u0 = tl;
      if (((tmp_d = tr[tl].d0) > 0) && (tr[tmp_d].u1 == tu))
	tr[tmp_d].u1 = tl;

      if (((tmp_d = tr[tl].d1) > 0) && (tr[tmp_d].u0 == tu))
	tr[tmp_d].u0 = tl;
      if (((tmp_d = tr[tl].d1) > 0) && (tr[tmp_d].u1 == tu))
	tr[tmp_d].u1 = tl;
      
      /* Now update the query structure and obtain the sinks for the */
      /* two trapezoids */ 
      
      i1 = newnode();		/* Upper trapezoid sink */
      i2 = newnode();		/* Lower trapezoid sink */
      sk = tr[tu].sink;
      
      qs[sk].nodetype = T_Y;
      qs[sk].yval = s.v1;
      qs[sk].segnum = segnum;	/* not really reqd ... maybe later */
      qs[sk].left = i2;
      qs[sk].right = i1;

      qs[i1].nodetype = T_SINK;
      qs[i1].trnum = tu;
      qs[i1].parent = sk;

      qs[i2].nodetype = T_SINK;
      qs[i2].trnum = tl;
      qs[i2].parent = sk;

      tr[tu].sink = i1;
      tr[tl].sink = i2;
      tlast = tu;
    }
  else				/* v1 already present */
    {       /* Get the lowermost intersecting trapezoid */
      tlast = locate_endpoint(&s.v1, &s.v0, s.root1, seg, qs);
      tribot = 1;
    }
  
  /* Thread the segment into the query tree creating a new X-node */
  /* First, split all the trapezoids which are intersected by s into */
  /* two */

  t = tfirst;			/* topmost trapezoid */
  
  while ((t > 0) && 
	 _greater_than_equal_to(&tr[t].lo, &tr[tlast].lo))
				/* traverse from top to bot */
    {
      int t_sav, tn_sav;
      sk = tr[t].sink;
      i1 = newnode();		/* left trapezoid sink */
      i2 = newnode();		/* right trapezoid sink */
      
      qs[sk].nodetype = T_X;
      qs[sk].segnum = segnum;
      qs[sk].left = i1;
      qs[sk].right = i2;

      qs[i1].nodetype = T_SINK;	/* left trapezoid (use existing one) */
      qs[i1].trnum = t;
      qs[i1].parent = sk;

      qs[i2].nodetype = T_SINK;	/* right trapezoid (allocate new) */
      qs[i2].trnum = tn = newtrap(tr);
      tr[tn].state = ST_VALID;
      qs[i2].parent = sk;

      if (t == tfirst)
	tfirstr = tn;
      if (_equal_to(&tr[t].lo, &tr[tlast].lo))
	tlastr = tn;

      tr[tn] = tr[t];
      tr[t].sink = i1;
      tr[tn].sink = i2;
      t_sav = t;
      tn_sav = tn;

      /* error */

      if ((tr[t].d0 <= 0) && (tr[t].d1 <= 0)) /* case cannot arise */
	{
	  fprintf(stderr, "add_segment: error\n");
	  break;
	}
      
      /* only one trapezoid below. partition t into two and make the */
      /* two resulting trapezoids t and tn as the upper neighbours of */
      /* the sole lower trapezoid */
      
      else if ((tr[t].d0 > 0) && (tr[t].d1 <= 0))
	{			/* Only one trapezoid below */
	  if ((tr[t].u0 > 0) && (tr[t].u1 > 0))
	    {			/* continuation of a chain from abv. */
	      if (tr[t].usave > 0) /* three upper neighbours */
		{
		  if (tr[t].uside == S_LEFT)
		    {
		      tr[tn].u0 = tr[t].u1;
		      tr[t].u1 = -1;
		      tr[tn].u1 = tr[t].usave;
		      
		      tr[tr[t].u0].d0 = t;
		      tr[tr[tn].u0].d0 = tn;
		      tr[tr[tn].u1].d0 = tn;
		    }
		  else		/* intersects in the right */
		    {
		      tr[tn].u1 = -1;
		      tr[tn].u0 = tr[t].u1;
		      tr[t].u1 = tr[t].u0;
		      tr[t].u0 = tr[t].usave;

		      tr[tr[t].u0].d0 = t;
		      tr[tr[t].u1].d0 = t;
		      tr[tr[tn].u0].d0 = tn;		      
		    }
		  
		  tr[t].usave = tr[tn].usave = 0;
		}
	      else		/* No usave.... simple case */
		{
		  tr[tn].u0 = tr[t].u1;
		  tr[t].u1 = tr[tn].u1 = -1;
		  tr[tr[tn].u0].d0 = tn;
		}
	    }
	  else 
	    {			/* fresh seg. or upward cusp */
	      int tmp_u = tr[t].u0;
	      int td0, td1;
	      if (((td0 = tr[tmp_u].d0) > 0) && 
		  ((td1 = tr[tmp_u].d1) > 0))
		{		/* upward cusp */
		  if ((tr[td0].rseg > 0) &&
		      !is_left_of(tr[td0].rseg, seg, &s.v1))
		    {
		      tr[t].u0 = tr[t].u1 = tr[tn].u1 = -1;
		      tr[tr[tn].u0].d1 = tn;
		    }
		  else		/* cusp going leftwards */
		    { 
		      tr[tn].u0 = tr[tn].u1 = tr[t].u1 = -1;
		      tr[tr[t].u0].d0 = t;
		    }
		}
	      else		/* fresh segment */
		{
		  tr[tr[t].u0].d0 = t;
		  tr[tr[t].u0].d1 = tn;
		}	      
	    }
	  
	  if (FP_EQUAL(tr[t].lo.y, tr[tlast].lo.y) && 
	      FP_EQUAL(tr[t].lo.x, tr[tlast].lo.x) && tribot)
	    {		/* bottom forms a triangle */

	      if (is_swapped)	
		tmptriseg = seg[segnum].prev;
	      else
		tmptriseg = seg[segnum].next;
	      
	      if ((tmptriseg > 0) && is_left_of(tmptriseg, seg, &s.v0))
		{
				/* L-R downward cusp */
		  tr[tr[t].d0].u0 = t;
		  tr[tn].d0 = tr[tn].d1 = -1;
		}
	      else
		{
				/* R-L downward cusp */
		  tr[tr[tn].d0].u1 = tn;
		  tr[t].d0 = tr[t].d1 = -1;
		}
	    }
	  else
	    {
	      if ((tr[tr[t].d0].u0 > 0) && (tr[tr[t].d0].u1 > 0))
		{
		  if (tr[tr[t].d0].u0 == t) /* passes thru LHS */
		    {
		      tr[tr[t].d0].usave = tr[tr[t].d0].u1;
		      tr[tr[t].d0].uside = S_LEFT;
		    }
		  else
		    {
		      tr[tr[t].d0].usave = tr[tr[t].d0].u0;
		      tr[tr[t].d0].uside = S_RIGHT;
		    }		    
		}
	      tr[tr[t].d0].u0 = t;
	      tr[tr[t].d0].u1 = tn;
	    }
	  
	  t = tr[t].d0;
	}


      else if ((tr[t].d0 <= 0) && (tr[t].d1 > 0))
	{			/* Only one trapezoid below */
	  if ((tr[t].u0 > 0) && (tr[t].u1 > 0))
	    {			/* continuation of a chain from abv. */
	      if (tr[t].usave > 0) /* three upper neighbours */
		{
		  if (tr[t].uside == S_LEFT)
		    {
		      tr[tn].u0 = tr[t].u1;
		      tr[t].u1 = -1;
		      tr[tn].u1 = tr[t].usave;
		      
		      tr[tr[t].u0].d0 = t;
		      tr[tr[tn].u0].d0 = tn;
		      tr[tr[tn].u1].d0 = tn;
		    }
		  else		/* intersects in the right */
		    {
		      tr[tn].u1 = -1;
		      tr[tn].u0 = tr[t].u1;
		      tr[t].u1 = tr[t].u0;
		      tr[t].u0 = tr[t].usave;

		      tr[tr[t].u0].d0 = t;
		      tr[tr[t].u1].d0 = t;
		      tr[tr[tn].u0].d0 = tn;		      
		    }
		  
		  tr[t].usave = tr[tn].usave = 0;
		}
	      else		/* No usave.... simple case */
		{
		  tr[tn].u0 = tr[t].u1;
		  tr[t].u1 = tr[tn].u1 = -1;
		  tr[tr[tn].u0].d0 = tn;
		}
	    }
	  else 
	    {			/* fresh seg. or upward cusp */
	      int tmp_u = tr[t].u0;
	      int td0, td1;
	      if (((td0 = tr[tmp_u].d0) > 0) && 
		  ((td1 = tr[tmp_u].d1) > 0))
		{		/* upward cusp */
		  if ((tr[td0].rseg > 0) &&
		      !is_left_of(tr[td0].rseg, seg, &s.v1))
		    {
		      tr[t].u0 = tr[t].u1 = tr[tn].u1 = -1;
		      tr[tr[tn].u0].d1 = tn;
		    }
		  else 
		    {
		      tr[tn].u0 = tr[tn].u1 = tr[t].u1 = -1;
		      tr[tr[t].u0].d0 = t;
		    }
		}
	      else		/* fresh segment */
		{
		  tr[tr[t].u0].d0 = t;
		  tr[tr[t].u0].d1 = tn;
		}
	    }
	  
	  if (FP_EQUAL(tr[t].lo.y, tr[tlast].lo.y) && 
	      FP_EQUAL(tr[t].lo.x, tr[tlast].lo.x) && tribot)
	    {		/* bottom forms a triangle */
	      /* int tmpseg; */

	      if (is_swapped)	
		tmptriseg = seg[segnum].prev;
	      else
		tmptriseg = seg[segnum].next;

	      /* if ((tmpseg > 0) && is_left_of(tmpseg, seg, &s.v0)) */
	      if ((tmptriseg > 0) && is_left_of(tmptriseg, seg, &s.v0))
		{
		  /* L-R downward cusp */
		  tr[tr[t].d1].u0 = t;
		  tr[tn].d0 = tr[tn].d1 = -1;
		}
	      else
		{
		  /* R-L downward cusp */
		  tr[tr[tn].d1].u1 = tn;
		  tr[t].d0 = tr[t].d1 = -1;
		}
	    }		
	  else
	    {
	      if ((tr[tr[t].d1].u0 > 0) && (tr[tr[t].d1].u1 > 0))
		{
		  if (tr[tr[t].d1].u0 == t) /* passes thru LHS */
		    {
		      tr[tr[t].d1].usave = tr[tr[t].d1].u1;
		      tr[tr[t].d1].uside = S_LEFT;
		    }
		  else
		    {
		      tr[tr[t].d1].usave = tr[tr[t].d1].u0;
		      tr[tr[t].d1].uside = S_RIGHT;
		    }		    
		}
	      tr[tr[t].d1].u0 = t;
	      tr[tr[t].d1].u1 = tn;
	    }
	  
	  t = tr[t].d1;
	}

      /* two trapezoids below. Find out which one is intersected by */
      /* this segment and proceed down that one */
      
      else
	{
	  /* int tmpseg = tr[tr[t].d0].rseg; */
	  double y0, yt;
	  pointf tmppt;
	  int tnext, i_d0, i_d1;

	  i_d0 = i_d1 = FALSE;
	  if (FP_EQUAL(tr[t].lo.y, s.v0.y))
	    {
	      if (tr[t].lo.x > s.v0.x)
		i_d0 = TRUE;
	      else
		i_d1 = TRUE;
	    }
	  else
	    {
	      tmppt.y = y0 = tr[t].lo.y;
	      yt = (y0 - s.v0.y)/(s.v1.y - s.v0.y);
	      tmppt.x = s.v0.x + yt * (s.v1.x - s.v0.x);
	      
	      if (_less_than(&tmppt, &tr[t].lo))
		i_d0 = TRUE;
	      else
		i_d1 = TRUE;
	    }
	  
	  /* check continuity from the top so that the lower-neighbour */
	  /* values are properly filled for the upper trapezoid */

	  if ((tr[t].u0 > 0) && (tr[t].u1 > 0))
	    {			/* continuation of a chain from abv. */
	      if (tr[t].usave > 0) /* three upper neighbours */
		{
		  if (tr[t].uside == S_LEFT)
		    {
		      tr[tn].u0 = tr[t].u1;
		      tr[t].u1 = -1;
		      tr[tn].u1 = tr[t].usave;
		      
		      tr[tr[t].u0].d0 = t;
		      tr[tr[tn].u0].d0 = tn;
		      tr[tr[tn].u1].d0 = tn;
		    }
		  else		/* intersects in the right */
		    {
		      tr[tn].u1 = -1;
		      tr[tn].u0 = tr[t].u1;
		      tr[t].u1 = tr[t].u0;
		      tr[t].u0 = tr[t].usave;

		      tr[tr[t].u0].d0 = t;
		      tr[tr[t].u1].d0 = t;
		      tr[tr[tn].u0].d0 = tn;		      
		    }
		  
		  tr[t].usave = tr[tn].usave = 0;
		}
	      else		/* No usave.... simple case */
		{
		  tr[tn].u0 = tr[t].u1;
		  tr[tn].u1 = -1;
		  tr[t].u1 = -1;
		  tr[tr[tn].u0].d0 = tn;
		}
	    }
	  else 
	    {			/* fresh seg. or upward cusp */
	      int tmp_u = tr[t].u0;
	      int td0, td1;
	      if (((td0 = tr[tmp_u].d0) > 0) && 
		  ((td1 = tr[tmp_u].d1) > 0))
		{		/* upward cusp */
		  if ((tr[td0].rseg > 0) &&
		      !is_left_of(tr[td0].rseg, seg, &s.v1))
		    {
		      tr[t].u0 = tr[t].u1 = tr[tn].u1 = -1;
		      tr[tr[tn].u0].d1 = tn;
		    }
		  else 
		    {
		      tr[tn].u0 = tr[tn].u1 = tr[t].u1 = -1;
		      tr[tr[t].u0].d0 = t;
		    }
		}
	      else		/* fresh segment */
		{
		  tr[tr[t].u0].d0 = t;
		  tr[tr[t].u0].d1 = tn;
		}
	    }
	  
	  if (FP_EQUAL(tr[t].lo.y, tr[tlast].lo.y) && 
	      FP_EQUAL(tr[t].lo.x, tr[tlast].lo.x) && tribot)
	    {
	      /* this case arises only at the lowest trapezoid.. i.e.
		 tlast, if the lower endpoint of the segment is
		 already inserted in the structure */
	      
	      tr[tr[t].d0].u0 = t;
	      tr[tr[t].d0].u1 = -1;
	      tr[tr[t].d1].u0 = tn;
	      tr[tr[t].d1].u1 = -1;

	      tr[tn].d0 = tr[t].d1;
	      tr[t].d1 = tr[tn].d1 = -1;
	      
	      tnext = tr[t].d1;	      
	    }
	  else if (i_d0)
				/* intersecting d0 */
	    {
	      tr[tr[t].d0].u0 = t;
	      tr[tr[t].d0].u1 = tn;
	      tr[tr[t].d1].u0 = tn;
	      tr[tr[t].d1].u1 = -1;
	      
	      /* new code to determine the bottom neighbours of the */
	      /* newly partitioned trapezoid */
	      
	      tr[t].d1 = -1;

	      tnext = tr[t].d0;
	    }
	  else			/* intersecting d1 */
	    {
	      tr[tr[t].d0].u0 = t;
	      tr[tr[t].d0].u1 = -1;
	      tr[tr[t].d1].u0 = t;
	      tr[tr[t].d1].u1 = tn;

	      /* new code to determine the bottom neighbours of the */
	      /* newly partitioned trapezoid */
	      
	      tr[tn].d0 = tr[t].d1;
	      tr[tn].d1 = -1;
	      
	      tnext = tr[t].d1;
	    }	    
	  
	  t = tnext;
	}
      
      tr[t_sav].rseg = tr[tn_sav].lseg  = segnum;
    } /* end-while */
  
  /* Now combine those trapezoids which share common segments. We can */
  /* use the pointers to the parent to connect these together. This */
  /* works only because all these new trapezoids have been formed */
  /* due to splitting by the segment, and hence have only one parent */

  tfirstl = tfirst; 
  tlastl = tlast;
  merge_trapezoids(segnum, tfirstl, tlastl, S_LEFT, tr, qs);
  merge_trapezoids(segnum, tfirstr, tlastr, S_RIGHT, tr, qs);

  seg[segnum].is_inserted = TRUE;
  return 0;
}

/* Update the roots stored for each of the endpoints of the segment.
 * This is done to speed up the location-query for the endpoint when
 * the segment is inserted into the trapezoidation subsequently
 */
static void
find_new_roots(int segnum, segment_t* seg, trap_t* tr, qnode_t* qs)
{
  segment_t *s = &seg[segnum];
  
  if (s->is_inserted) return;

  s->root0 = locate_endpoint(&s->v0, &s->v1, s->root0, seg, qs);
  s->root0 = tr[s->root0].sink;

  s->root1 = locate_endpoint(&s->v1, &s->v0, s->root1, seg, qs);
  s->root1 = tr[s->root1].sink;  
}

/* Get log*n for given n */
static int math_logstar_n(int n)
{
  register int i;
  double v;

  for (i = 0, v = (double) n; v >= 1; i++)
      v = log2(v);

  return (i - 1);
}

static int math_N(int n, int h)
{
  register int i;
  double v;

  for (i = 0, v = (int) n; i < h; i++)
      v = log2(v);

  return (int) ceil((double) 1.0*n/v);
}

/* Main routine to perform trapezoidation */
int
construct_trapezoids(int nseg, segment_t* seg, int* permute, int ntraps,
  trap_t* tr)
{
    int i;
    int root, h;
    int segi = 1;
    qnode_t* qs;
  
    QSIZE = 2*ntraps;
    TRSIZE = ntraps;
    qs = N_NEW (2*ntraps, qnode_t);
    q_idx = tr_idx = 1;
    memset((void *)tr, 0, ntraps*sizeof(trap_t));

  /* Add the first segment and get the query structure and trapezoid */
  /* list initialised */

    root = init_query_structure(permute[segi++], seg, tr, qs);

    for (i = 1; i <= nseg; i++)
	seg[i].root0 = seg[i].root1 = root;
  
    for (h = 1; h <= math_logstar_n(nseg); h++) {
	for (i = math_N(nseg, h -1) + 1; i <= math_N(nseg, h); i++)
	    add_segment(permute[segi++], seg, tr, qs);
      
      /* Find a new root for each of the segment endpoints */
	for (i = 1; i <= nseg; i++)
	    find_new_roots(i, seg, tr, qs);
    }
  
    for (i = math_N(nseg, math_logstar_n(nseg)) + 1; i <= nseg; i++)
	add_segment(permute[segi++], seg, tr, qs);

    free (qs);
    return tr_idx;
}

