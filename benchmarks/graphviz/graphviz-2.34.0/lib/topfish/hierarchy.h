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

#ifndef _HIERARCHY_H_
#define _HIERARCHY_H_

#include "sparsegraph.h"

typedef struct {
    int nedges;       // degree, including self-loop
    int *edges;       // neighbors; edges[0] = self
    int size;         // no. of original nodes contained
    int active_level; // Node displayed iff active_level == node's level
    int globalIndex;  // Each node has a unique ID over all levels

    // position of node in universal coordinate system
    float x_coord;
    float y_coord;

    // position of node in physical (device) coordinate system
    float physical_x_coord;
    float physical_y_coord;	
	//previous coords and active level (for animation)
    float old_physical_x_coord;
    float old_physical_y_coord;	
	int old_active_level;


} ex_vtx_data;

typedef struct {
    int nlevels;
    v_data ** graphs;
    ex_vtx_data ** geom_graphs;
    int * nvtxs;
    int * nedges;
    /* Node i on level k is mapped to coarse node v2cv[k][i] on level k+1
     */
    int ** v2cv;
    /* Coarse node i on level k contains nodes cv2v[k][2*i] and 
     * cv2v[k][2*i+1] on level k-1. If it contains only 1 node, then
     * cv2v[k][2*i+1] will be -1
     */
    int ** cv2v;
    int maxNodeIndex;
} Hierarchy;

typedef struct {
    int num_fine_nodes; /* 50 */
    double coarsening_rate; /* 2.5 */
} levelparms_t;

typedef struct {
    // if dist2_limit true, don't contract nodes of distance larger than 2
    // if false then also distance 3 is possible
    int dist2_limit; /* TRUE */
    int min_nvtxs;   /* 20 */
} hierparms_t;

void release(Hierarchy*);

Hierarchy* create_hierarchy(v_data * graph, int nvtxs, int nedges, 
    ex_vtx_data* geom_graph, int ngeom_edges, hierparms_t*);
	
void set_active_levels(Hierarchy*, int*, int, levelparms_t*);
double find_closest_active_node(Hierarchy*, double x, double y, int*);

int extract_active_logical_coords(Hierarchy * hierarchy, int node, int level, 
    double *x_coords, double *y_coords, int counter);
int set_active_physical_coords(Hierarchy *, int node, int level,
    double *x_coords, double *y_coords, int counter);

int count_active_nodes(Hierarchy *);
void init_active_level(Hierarchy* hierarchy, int level);

// creating a geometric graph:
int init_ex_graph(v_data * graph1, v_data * graph2, int n,
   double *x_coords, double *y_coords, ex_vtx_data ** gp);

// layout distortion:
void rescale_layout(double *x_coords, double *y_coords,
    int n, int interval, double width, double height,
    double margin, double distortion);
void rescale_layout_polar(double * x_coords, double * y_coords, 
    double * x_foci, double * y_foci, int num_foci, int n, int interval, 
    double width, double height, double margin, double distortion);

void find_physical_coords(Hierarchy*, int, int, double *x, double *y);
void find_old_physical_coords(Hierarchy * hierarchy, int level, int node, double *x,double *y);


int find_active_ancestor(Hierarchy*, int, int);
void find_active_ancestor_info(Hierarchy * hierarchy, int level, int node, int *levell,int *nodee);

int find_old_active_ancestor(Hierarchy * hierarchy, int level, int node);
int locateByIndex(Hierarchy*, int, int*);
int findGlobalIndexesOfActiveNeighbors(Hierarchy*, int, int**);

#endif
