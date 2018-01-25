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

#ifndef STRUCTURES_H
#define STRUCTURES_H

#include "types.h"
#ifdef WITH_CGRAPH
#include "cgraph.h"
#else
#include "graph.h"
#endif
#include "rawgraph.h"

typedef struct {
    double p1, p2;
} paird;

typedef struct {
    int a,b;
} pair;

typedef struct {
	pair t1, t2;
} pair2;

typedef enum {B_NODE, B_UP, B_LEFT, B_DOWN, B_RIGHT} bend;

/* Example : segment connecting maze point (3,2) 
 * and (3,8) has isVert = 1, common coordinate = 3, p1 = 2, p2 = 8
 */
typedef struct segment {
  boolean isVert;
  boolean flipped;
  double comm_coord;  /* the common coordinate */
  paird p;      /* end points */
  bend l1, l2; 
  int ind_no;      /* index number of this segment in its channel */
  int track_no;    /* track number assigned in the channel */
  struct segment* prev;
  struct segment* next;
} segment;

typedef struct {
  int n;
  segment* segs;
} route;

typedef struct {
  Dtlink_t link;
  paird p;   /* extrema of channel */
  int cnt;   /* number of segments */
  segment** seg_list; /* array of segment pointers */
  rawgraph* G;
  struct cell* cp;
} channel;

#if 0
typedef struct {
  int i1, i2, j;
  int cnt;
  int* seg_list;  /* list of indices of the segment list */

  rawgraph* G;
} hor_channel;

typedef struct {
	hor_channel* hs;
	int cnt;
} vhor_channel;

typedef struct {
  int i, j1, j2;
  int cnt;
  int* seg_list;  /* list of indices of the segment list */

  rawgraph* G;
} vert_channel;

typedef struct {
	vert_channel* vs;
	int cnt;
} vvert_channel;
#endif

#define N_DAD(n) (n)->n_dad

#endif
