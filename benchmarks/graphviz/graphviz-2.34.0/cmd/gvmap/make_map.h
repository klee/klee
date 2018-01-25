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

#ifndef MAKE_MAP_H
#define MAKE_MAP_H

#include <SparseMatrix.h>
#include <cgraph.h>

int make_map_from_rectangle_groups(int exclude_random, int include_OK_points, int n, int dim, real *x, real *sizes, 
				   int *grouping, SparseMatrix graph, real bounding_box_margin[], int *nrandom,int *nart, int nedgep, 
				   real shore_depth_tol, real edge_bridge_tol, real **xcombined, int *nverts, real **x_poly, 
				   int *npolys, SparseMatrix *poly_lines, SparseMatrix *polys, int **polys_groups, SparseMatrix *poly_point_map, 
				   SparseMatrix *country_graph, int highlight_cluster, int *flag);

int make_map_from_point_groups(int exclude_random, int include_OK_points, int n, int dim, real *x, int *grouping, SparseMatrix graph, real bounding_box_margin[], int *nrandom,
			       real shore_depth_tol, real edge_bridge_tol, real **xcombined, int *nverts, real **x_poly, 
			       int *npolys, SparseMatrix *poly_lines, SparseMatrix *polys, int **polys_groups, SparseMatrix *poly_point_map, 
			       SparseMatrix *country_graph, int *flag);


void improve_contiguity(int n, int dim, int *grouping, SparseMatrix poly_point_map, real *x, SparseMatrix graph, real *label_sizes);


#if 0
void plot_polys(int use_line, SparseMatrix polys, real *x_poly, int *polys_groups, float *r, float *g, float *b);
void plot_points(int n, int dim, real *x);
void plot_edges(int n, int dim, real *x, SparseMatrix A);
void plot_labels(int n, int dim, real *x, char **labels);
void plot_ps_map(int n, int dim, real *x, SparseMatrix polys, SparseMatrix poly_lines, real line_width, real *x_poly, int *polys_groups, char **labels, real *width,
		 float *fsz, float *r, float *g, float *b, char *plot_label, real *bg_color, SparseMatrix A);
#endif

void plot_dot_map(Agraph_t* gr, int n, int dim, real *x, SparseMatrix polys, SparseMatrix poly_lines, real line_width, char *line_color, real *x_poly, int *polys_groups, char **labels, real *width, float *fsz, float *r, float *g, float *b, char* opacity, char *plot_label, real *bg_color, SparseMatrix A, FILE*);

#if 0
void plot_processing_map(Agraph_t* gr, int n, int dim, real *x, SparseMatrix polys, SparseMatrix poly_lines, real line_width, int nverts, real *x_poly, int *polys_groups, char **labels, real *width, float *fsz, float *r, float *g, float *b, char *plot_label, real *bg_color, SparseMatrix A);
#endif

void map_optimal_coloring(int seed, SparseMatrix A, float *rgb_r,  float *rgb_g, float *rgb_b);

enum {POLY_LINE_REAL_EDGE, POLY_LINE_NOT_REAL_EDGE};
enum {OUT_PS = 1, OUT_M = 0, OUT_M_COUNTRY_GRAPH = 2, OUT_DOT = 3, OUT_PROCESSING = 4};
#define neighbor(t, i, edim, elist) elist[(edim)*(t)+i]
#define edge_head(e) edge_table[2*(e)]
#define edge_tail(e) edge_table[2*(e)+1]
#define cycle_prev(e) cycle[2*(e)]
#define cycle_next(e) cycle[2*(e)+1]

#endif
