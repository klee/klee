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

#ifndef DOTIO_H
#define DOTIO_H

#include <cgraph.h>
#include "SparseMatrix.h"

enum {COLOR_SCHEME_NONE, COLOR_SCHEME_PASTEL = 1, COLOR_SCHEME_BLUE_YELLOW, COLOR_SCHEME_WHITE_RED, COLOR_SCHEME_GREY_RED, COLOR_SCHEME_PRIMARY, COLOR_SCHEME_SEQUENTIAL_SINGLEHUE_RED, COLOR_SCHEME_ADAM, COLOR_SCHEME_ADAM_BLEND, COLOR_SCHEME_SEQUENTIAL_SINGLEHUE_RED_LIGHTER, COLOR_SCHEME_GREY};
extern void initDotIO (Agraph_t *g);
extern Agraph_t* SparseMatrix_read_dot(FILE*);

extern void setDotNodeID (Agnode_t* n, int v);
extern int getDotNodeID (Agnode_t* n);

/* extern void attach_embedding(Agraph_t *g, int dim, double sc, real *x); */
extern SparseMatrix SparseMatrix_import_dot(Agraph_t* g, int dim, real **label_sizes, real **x, int *n_edge_label_nodes,
					    int **edge_label_nodes, int format, SparseMatrix *D);
extern Agraph_t* makeDotGraph (SparseMatrix, char *title, int dim, real *x, int with_color, int with_label, int use_matrix_value);
Agraph_t *convert_edge_labels_to_nodes(Agraph_t* g);
Agraph_t* assign_random_edge_color(Agraph_t* g);
void dump_coordinates(char *name, int n, int dim, real *x);
void edgelist_export(FILE* f, SparseMatrix A, int dim, double *x);
char * hue2rgb(real hue, char *color);

SparseMatrix Import_coord_clusters_from_dot(Agraph_t* g, int maxcluster, int dim, int *nn, real **label_sizes, real **x, int **clusters, float **rgb_r,  float **rgb_g,  float **rgb_b,  float **fsz, char ***labels, int default_color_scheme, int clustering_scheme, int useClusters);

void Dot_SetClusterColor(Agraph_t* g, float *rgb_r,  float *rgb_g,  float *rgb_b, int *clustering);
void attached_clustering(Agraph_t* g, int maxcluster, int clustering_scheme);

#endif
