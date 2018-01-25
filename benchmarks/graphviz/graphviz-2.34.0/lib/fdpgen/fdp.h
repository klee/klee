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

#ifndef FDP_H
#define FDP_H

#include <render.h>

#ifdef FDP_PRIVATE

#define NDIM     2

typedef struct bport_s {
    edge_t *e;
    node_t *n;
    double alpha;
} bport_t;

/* gdata is attached to the root graph, each cluster graph, 
 * and to each derived graph.
 * Graphs also use "builtin" fields:
 *   n_cluster, clust - to record clusters  
 */
typedef struct {
    bport_t *ports;		/* boundary ports. 0-terminated */
    int nports;			/* no. of ports */
    boxf bb;			/* bounding box of graph */
    int flags;
    int level;			/* depth in graph hierarchy */
    graph_t *parent;		/* smallest containing cluster */
#ifdef DEBUG
    graph_t *orig;		/* original of derived graph */
#endif
} gdata;

#define GDATA(g)    ((gdata*)(GD_alg(g)))
#define BB(g)       (GDATA(g)->bb)
#define PORTS(g)    (GDATA(g)->ports)
#define NPORTS(g)   (GDATA(g)->nports)
#define LEVEL(g)    (GDATA(g)->level)
#define GPARENT(g)  (GDATA(g)->parent)
#ifdef DEBUG
#define GORIG(g)    (GDATA(g)->orig)
#endif

#if 0
/* ndata is attached to nodes in real graphs.
 * Real nodes also use "builtin" fields:
 *   pos   - position information
 *   width,height     - node dimensions
 *   xsize,ysize      - node dimensions in points
 */
typedef struct {
    node_t *dn;			/* points to corresponding derived node,
				 * which may represent the node or its
				 * containing cluster. */
    graph_t *parent;		/* smallest containing cluster */
} ndata;

#define NDATA(n) ((ndata*)(ND_alg(n)))
#define DNODE(n) (NDATA(n)->dn)
#define PARENT(n) (NDATA(n)->parent)
#endif

/* 
 * Real nodes use "builtin" fields:
 *   ND_pos   - position information
 *   ND_width,ND_height     - node dimensions
 *   ND_pinned
 *   ND_lw,ND_rw,ND_ht      - node dimensions in points
 *   ND_id
 *   ND_shape, ND_shape_info
 *
 * In addition, we use two of the dot fields for parent and derived node. 
 * Previously, we attached these via ND_alg, but ND_alg may be needed for
 * spline routing, and splines=compound also requires the parent field. 
 */
#define DNODE(n) (ND_next(n))
#define PARENT(n) (ND_clust(n))

/* dndata is attached to nodes in derived graphs.
 * Derived nodes also use "builtin" fields:
 *   clust - for cluster nodes, points to cluster in real graph.
 *   pos   - position information
 *   width,height     - node dimensions
 */
typedef struct {
    int deg;			/* degree of node */
    int wdeg;			/* weighted degree of node */
    node_t *dn;			/* If derived node is not a cluster, */
    /* dn points real node. */
    double disp[NDIM];		/* incremental displacement */
} dndata;

#define DNDATA(n) ((dndata*)(ND_alg(n)))
#define DISP(n) (DNDATA(n)->disp)
#define ANODE(n) (DNDATA(n)->dn)
#define DEG(n) (DNDATA(n)->deg)
#define WDEG(n) (DNDATA(n)->wdeg)
#define IS_PORT(n) (!ANODE(n) && !ND_clust(n))

#endif				/*  FDP_PRIVATE */

#ifdef __cplusplus
extern "C" {
#endif

struct fdpParms_s {
        int useGrid;            /* use grid for speed up */
        int useNew;             /* encode x-K into attractive force */
        int numIters;           /* actual iterations in layout */
        int unscaled;           /* % of iterations used in pass 1 */
        double C;               /* Repulsion factor in xLayout */
        double Tfact;           /* scale temp from default expression */
        double K;               /* spring constant; ideal distance */
        double T0;              /* initial temperature */
};
typedef struct fdpParms_s fdpParms_t;

    extern void fdp_layout(Agraph_t * g);
    extern void fdp_nodesize(node_t *, boolean);
    extern void fdp_init_graph(Agraph_t * g);
    extern void fdp_init_node_edge(Agraph_t * g);
    extern void fdp_cleanup(Agraph_t * g);

#ifdef __cplusplus
}
#endif
#endif
