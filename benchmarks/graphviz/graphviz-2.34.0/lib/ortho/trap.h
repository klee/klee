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

#ifndef TRAP_H
#define TRAP_H

/* Segment attributes */

typedef struct {
  pointf v0, v1;       /* two endpoints */ 
  int is_inserted;      /* inserted in trapezoidation yet ? */
  int root0, root1;     /* root nodes in Q */
  int next;         /* Next logical segment */
  int prev;         /* Previous segment */
} segment_t;


/* Trapezoid attributes */

typedef struct {
  int lseg, rseg;       /* two adjoining segments */
  pointf hi, lo;       /* max/min y-values */ 
  int u0, u1;
  int d0, d1;
  int sink;         /* pointer to corresponding in Q */
  int usave, uside;     /* I forgot what this means */
  int state;
} trap_t; 

#define ST_VALID 1      /* for trapezium state */
#define ST_INVALID 2

#define C_EPS 1.0e-7        /* tolerance value: Used for making */
                /* all decisions about collinearity or */
                /* left/right of segment. Decrease */
                /* this value if the input points are */
                /* spaced very close together */
#define FP_EQUAL(s, t) (fabs(s - t) <= C_EPS)

#define _equal_to(v0,v1) \
  (FP_EQUAL((v0)->y, (v1)->y) && FP_EQUAL((v0)->x, (v1)->x))

#define _greater_than(v0, v1) \
  (((v0)->y > (v1)->y + C_EPS) ? TRUE : (((v0)->y < (v1)->y - C_EPS) ? FALSE : ((v0)->x > (v1)->x)))

extern int construct_trapezoids(int, segment_t*, int*, int, trap_t*);

#endif
