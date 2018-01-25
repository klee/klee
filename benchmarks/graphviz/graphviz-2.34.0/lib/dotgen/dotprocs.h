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

#ifndef DOTPROCS_H
#define DOTPROCS_H

#ifdef _BEGIN_EXTERNS_
_BEGIN_EXTERNS_			/* public data */
#endif
/* tabs at 8, or you're a sorry loser */
#ifdef __cplusplus
extern "C" {
#endif

#include "aspect.h"

    extern void acyclic(Agraph_t *);
    extern void allocate_ranks(Agraph_t *);
    extern void build_ranks(Agraph_t *, int);
    extern void build_skeleton(Agraph_t *, Agraph_t *);
    extern void class1(Agraph_t *);
    extern void class2(Agraph_t *);
    extern void decompose(Agraph_t *, int);
    extern void delete_fast_edge(Agedge_t *);
    extern void delete_fast_node(Agraph_t *, Agnode_t *);
    extern void delete_flat_edge(Agedge_t *);
    extern void dot_cleanup(graph_t * g);
    extern void dot_layout(Agraph_t * g);
    extern void dot_init_node_edge(graph_t * g);
    extern void dot_scan_ranks(graph_t * g);
    extern void expand_cluster(Agraph_t *);
    extern Agedge_t *fast_edge(Agedge_t *);
    extern void fast_node(Agraph_t *, Agnode_t *);
    extern void fast_nodeapp(Agnode_t *, Agnode_t *);
    extern Agedge_t *find_fast_edge(Agnode_t *, Agnode_t *);
    extern Agedge_t *find_flat_edge(Agnode_t *, Agnode_t *);
    extern void flat_edge(Agraph_t *, Agedge_t *);
    extern int flat_edges(Agraph_t *);
    extern void install_cluster(Agraph_t *, Agnode_t *, int, nodequeue *);
    extern void install_in_rank(Agraph_t *, Agnode_t *);
    extern int is_cluster(Agraph_t *);
    extern void dot_compoundEdges(Agraph_t *);
    extern Agedge_t *make_aux_edge(Agnode_t *, Agnode_t *, double, int);
    extern void mark_clusters(Agraph_t *);
    extern void mark_lowclusters(Agraph_t *);
    extern int mergeable(edge_t * e, edge_t * f);
    extern void merge_chain(Agraph_t *, Agedge_t *, Agedge_t *, int);
    extern void merge_oneway(Agedge_t *, Agedge_t *);
    extern int ncross(Agraph_t *);
    extern Agedge_t *new_virtual_edge(Agnode_t *, Agnode_t *, Agedge_t *);
    extern int nonconstraint_edge(Agedge_t *);
    extern void other_edge(Agedge_t *);
    extern void rank1(graph_t * g);
    extern int portcmp(port p0, port p1);
    extern int ports_eq(edge_t *, edge_t *);
    extern void rec_reset_vlists(Agraph_t *);
    extern void rec_save_vlists(Agraph_t *);
    extern void reverse_edge(Agedge_t *);
    extern void safe_other_edge(Agedge_t *);
    extern void save_vlist(Agraph_t *);
    extern void unmerge_oneway(Agedge_t *);
    extern Agedge_t *virtual_edge(Agnode_t *, Agnode_t *, Agedge_t *);
    extern Agnode_t *virtual_node(Agraph_t *);
    extern void virtual_weight(Agedge_t *);
    extern void zapinlist(elist *, Agedge_t *);

#if defined(_BLD_dot) && defined(_DLL)
#   define extern __EXPORT__
#endif
    extern void dot_concentrate(Agraph_t *);
    extern void dot_mincross(Agraph_t *, int);
    extern void dot_position(Agraph_t *, aspect_t*);
    extern void dot_rank(Agraph_t *, aspect_t*);
    extern void dot_sameports(Agraph_t *);
    extern void dot_splines(Agraph_t *);
#undef extern

#ifdef _END_EXTERNS_
     _END_EXTERNS_
#endif
#ifdef __cplusplus
}
#endif
#endif
