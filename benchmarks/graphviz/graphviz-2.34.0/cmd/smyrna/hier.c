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

#include "smyrnadefs.h"
#include "hier.h"
#include "delaunay.h"
#include "memory.h"

/* scale_coords:
 */
static void
scale_coords(double *x_coords, double *y_coords, int n,
	     double w, double h, double margin)
{
    int i;
    double minX, maxX, minY, maxY;
    double scale_ratioX;
    double scale_ratioY;
    double scale_ratio;

    w -= 2 * margin;
    h -= 2 * margin;

    minX = maxX = x_coords[0];
    minY = maxY = y_coords[0];
    for (i = 1; i < n; i++) {
	if (x_coords[i] < minX)
	    minX = x_coords[i];
	if (y_coords[i] < minY)
	    minY = y_coords[i];
	if (x_coords[i] > maxX)
	    maxX = x_coords[i];
	if (y_coords[i] > maxY)
	    maxY = y_coords[i];
    }
    for (i = 0; i < n; i++) {
	x_coords[i] -= minX;
	y_coords[i] -= minY;
    }

    // scale the layout to fill canvas:

    scale_ratioX = w / (maxX - minX);
    scale_ratioY = h / (maxY - minY);
    scale_ratio = MIN(scale_ratioX, scale_ratioY);
    for (i = 0; i < n; i++) {
	x_coords[i] *= scale_ratio;
	y_coords[i] *= scale_ratio;
    }

    for (i = 0; i < n; i++) {
	x_coords[i] += margin;
	y_coords[i] += margin;
    }
}

void positionAllItems(Hierarchy * hp, focus_t * fs, reposition_t * parms)
{
    int i;
    int interval = 20;
    int counter = 0;		/* no. of active nodes */
    double *x_coords = N_NEW(hp->nvtxs[0], double);
    double *y_coords = N_NEW(hp->nvtxs[0], double);
    int max_level = hp->nlevels - 1;	// coarsest level
    double width = parms->width;
    double height = parms->height;
    double margin = parms->margin;
    double distortion = parms->distortion;

    /* get all logical coordinates of active nodes */
    for (i = 0; i < hp->nvtxs[max_level]; i++) {
	counter =
	    extract_active_logical_coords(hp, i, max_level, x_coords,
					  y_coords, counter);
    }

    /* distort logical coordinates in order to get uniform density
     * (equivalent to concentrating on the focus area)
     */
    width *= parms->graphSize / 100.0;
    height *= parms->graphSize / 100.0;
    if (fs->num_foci == 0) {
	if (parms->rescale == Scale)
	    scale_coords(x_coords, y_coords, counter, width, height,
			 margin);
    } else
	switch (parms->rescale) {
	case Polar:
	    rescale_layout_polar(x_coords, y_coords, fs->x_foci,
				 fs->y_foci, fs->num_foci, counter,
				 interval, width, height, margin,
				 distortion);
	    break;
	case Rectilinear:
	    rescale_layout(x_coords, y_coords, counter, interval,
			   width, height, margin, distortion);
	    break;
	case Scale:
	    scale_coords(x_coords, y_coords, counter, width, height,
			 margin);
	    break;
	case NoRescale:
	    break;
	}

    /* Update the final physical coordinates of the active nodes */
    for (counter = 0, i = 0; i < hp->nvtxs[max_level]; i++) {
	counter =
	    set_active_physical_coords(hp, i, max_level, x_coords,
				       y_coords, counter);
    }

    free(x_coords);
    free(y_coords);
}

#ifdef DEBUG
static void dumpG(int nn, v_data * graph)
{
    int i, j;
    for (i = 0; i < nn; i++) {
	fprintf(stderr, "[%d]", i);
	for (j = 1; j < graph->nedges; j++)
	    fprintf(stderr, " %d", graph->edges[j]);
	fprintf(stderr, "\n");
	graph++;
    }
}

static void dumpEG(int nn, ex_vtx_data * graph)
{
    int i, j;
    for (i = 0; i < nn; i++) {
	fprintf(stderr, "[%d](%d,%d,%d)(%f,%f)", i, graph->size,
		graph->active_level, graph->globalIndex, graph->x_coord,
		graph->y_coord);
	for (j = 1; j < graph->nedges; j++)
	    fprintf(stderr, " %d", graph->edges[j]);
	fprintf(stderr, "\n");
	graph++;
    }
}

static void dumpHier(Hierarchy * hier)
{
    int i;

    for (i = 0; i < hier->nlevels; i++) {
	fprintf(stderr, "level [%d] %d %d \n", i, hier->nvtxs[i],
		hier->nedges[i]);
	fprintf(stderr, "graph\n");
	dumpG(hier->nvtxs[i], hier->graphs[0]);
	fprintf(stderr, "geom_graph\n");
	dumpEG(hier->nvtxs[i], hier->geom_graphs[0]);
    }
}

#endif

Hierarchy *makeHier(int nn, int ne, v_data * graph, double *x_coords,
		    double *y_coords, hierparms_t * parms)
{
    v_data *delaunay;
    ex_vtx_data *geom_graph;
    int ngeom_edges;
    Hierarchy *hp;
    int i;

    delaunay = UG_graph(x_coords, y_coords, nn, 0);

    ngeom_edges =
	init_ex_graph(delaunay, graph, nn, x_coords, y_coords,
		      &geom_graph);
    free(delaunay[0].edges);
    free(delaunay);

    hp = create_hierarchy(graph, nn, ne, geom_graph, ngeom_edges, parms);
    free(geom_graph[0].edges);
    free(geom_graph);

    init_active_level(hp, 0);
    geom_graph = hp->geom_graphs[0];
    for (i = 0; i < hp->nvtxs[0]; i++) {
	geom_graph[i].physical_x_coord = (float) x_coords[i];
	geom_graph[i].physical_y_coord = (float) y_coords[i];
    }

/* dumpHier (hp); */
    return hp;
}

focus_t *initFocus(int ncnt)
{
    focus_t *fs = NEW(focus_t);
    fs->num_foci = 0;
    fs->foci_nodes = N_NEW(ncnt, int);
    fs->x_foci = N_NEW(ncnt, double);
    fs->y_foci = N_NEW(ncnt, double);
    return fs;
}

#ifdef UNUSED
void freeFocus(focus_t * fs)
{
    free(fs->foci_nodes);
    free(fs->x_foci);
    free(fs->y_foci);
    free(fs);
}
#endif
