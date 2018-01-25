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

#ifndef NEATOPROCS_H
#define NEATOPROCS_H

#ifdef __cplusplus
extern "C" {
#endif
#include "adjust.h"
#ifdef GVDLL
#define extern __declspec(dllexport)
#else
#define extern
#endif

    extern int allow_edits(int);
    extern void avoid_cycling(graph_t *, Agnode_t *, double *);
    extern int checkStart(graph_t * G, int nG, int);
    extern Agnode_t *choose_node(graph_t *, int);
    extern int circuit_model(graph_t *, int);
    extern void D2E(Agraph_t *, int, int, double *);
    extern void diffeq_model(graph_t *, int);
    extern double distvec(double *, double *, double *);
    extern void do_graph_label(Agraph_t *);
    extern void final_energy(graph_t *, int);
    extern double fpow32(double);
    extern Ppolyline_t getPath(edge_t *, vconfig_t *, int, Ppoly_t **,
			       int);
    extern void heapdown(Agnode_t *);
    extern void heapup(Agnode_t *);
    extern void initial_positions(graph_t *, int);
    extern int init_port(Agnode_t *, Agedge_t *, char *, boolean);
    extern void jitter3d(Agnode_t *, int);
    extern void jitter_d(Agnode_t *, int, int);
    extern Ppoly_t *makeObstacle(node_t * n, expand_t* );
    extern void makeSelfArcs(path * P, edge_t * e, int stepx);
    extern void makeSpline(graph_t*, edge_t *, Ppoly_t **, int, boolean);
    extern void make_spring(graph_t *, Agnode_t *, Agnode_t *, double);
    extern void move_node(graph_t *, int, Agnode_t *);
    extern int init_nop(graph_t * g, int);
    extern void neato_cleanup(graph_t * g);
    extern node_t *neato_dequeue(void);
    extern void neato_enqueue(node_t *);
    extern void neato_init_node(node_t * n);
    extern void neato_layout(Agraph_t * g);
    extern int Plegal_arrangement(Ppoly_t ** polys, int n_polys);
    extern void randompos(Agnode_t *, int);
    extern void s1(graph_t *, node_t *);
    extern int scan_graph(graph_t *);
    extern int scan_graph_mode(graph_t * G, int mode);
    extern void free_scan_graph(graph_t *);
    extern int setSeed (graph_t*, int dflt, long* seedp);
    extern void shortest_path(graph_t *, int);
    extern void solve(double *, double *, double *, int);
    extern void solve_model(graph_t *, int);
    extern int solveCircuit(int nG, double **Gm, double **Gm_inv);
    extern void spline_edges(Agraph_t *);
    extern void spline_edges0(Agraph_t *);
    extern int spline_edges1(graph_t * g, int);
    extern int splineEdges(graph_t *,
			   int (*edgefn) (graph_t *, expand_t*, int), int);
    extern void neato_set_aspect(graph_t * g);
    extern void toggle(int);
    extern int user_pos(Agsym_t *, Agsym_t *, Agnode_t *, int);
    extern double **new_array(int i, int j, double val);
    extern void free_array(double **rv);
    extern int matinv(double **A, double **Ainv, int n);

#undef extern
#ifdef __cplusplus
}
#endif
#endif
