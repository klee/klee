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

#include <stdio.h>
#include <stdlib.h>
#include "simple.h"

/* this is all out of pathgeom.h and will eventually get included */

typedef struct Pxy_t {
    double x, y;
} Pxy_t;

typedef struct Pxy_t Ppoint_t;
typedef struct Pxy_t Pvector_t;

typedef struct Ppoly_t {
    Ppoint_t *ps;
    int pn;
} Ppoly_t;

typedef Ppoly_t Ppolyline_t;

typedef struct Pedge_t {
    Ppoint_t a, b;
} Pedge_t;


#ifdef NOTDEF
int main(int argc, char **argv)
{
    Ppoly_t polys[2], *polys_ptr[2];

    polys[0].ps = (Ppoint_t *) malloc(3 * sizeof(Ppoint_t));
    polys[1].ps = (Ppoint_t *) malloc(3 * sizeof(Ppoint_t));
    polys[0].pn = 3;
    polys[1].pn = 3;

    polys[0].ps[0].x = 0.0;
    polys[0].ps[0].y = 0.0;
    polys[0].ps[1].x = 0.0;
    polys[0].ps[1].y = 100.0;
    polys[0].ps[2].x = 100.0;
    polys[0].ps[2].y = 0.0;

    polys[1].ps[0].x = 70.0;
    polys[1].ps[0].y = 70.0;
    polys[1].ps[1].x = 70.0;
    polys[1].ps[1].y = 100.0;
    polys[1].ps[2].x = 100.0;
    polys[1].ps[2].y = 70.0;

    polys_ptr[0] = &polys[0];
    polys_ptr[1] = &polys[1];

    if (Plegal_arrangement(polys_ptr, 2))
	printf(" it is legal\n");
    else
	printf(" it is not legal\n");
}
#endif

#ifdef NOTDEF
struct vertex *after(struct vertex *v)
{
    if (v == v->poly->finish)
	return v->poly->start;
    else
	return v + 1;
}

struct vertex *before(struct vertex *v)
{
    if (v == v->poly->start)
	return v->poly->finish;
    else
	return v - 1;
}
#endif

void find_ints(struct vertex vertex_list[], struct polygon polygon_list[],
	       struct data *input, struct intersection ilist[]);

int Plegal_arrangement(Ppoly_t ** polys, int n_polys)
{

    int i, j, vno, nverts, rv;

    struct vertex *vertex_list;
    struct polygon *polygon_list;
    struct data input;
    struct intersection ilist[10000];

    polygon_list = (struct polygon *)
	malloc(n_polys * sizeof(struct polygon));

    for (i = nverts = 0; i < n_polys; i++)
	nverts += polys[i]->pn;

    vertex_list = (struct vertex *)
	malloc(nverts * sizeof(struct vertex));

    for (i = vno = 0; i < n_polys; i++) {
	polygon_list[i].start = &vertex_list[vno];
	for (j = 0; j < polys[i]->pn; j++) {
	    vertex_list[vno].pos.x = polys[i]->ps[j].x;
	    vertex_list[vno].pos.y = polys[i]->ps[j].y;
	    vertex_list[vno].poly = &polygon_list[i];
	    vno++;
	}
	polygon_list[i].finish = &vertex_list[vno - 1];
    }

    input.nvertices = nverts;
    input.npolygons = n_polys;

    find_ints(vertex_list, polygon_list, &input, ilist);


#define EQ_PT(v,w) (((v).x == (w).x) && ((v).y == (w).y))
    rv = 1;
    {
	int i;
	struct position vft, vsd, avft, avsd;
	for (i = 0; i < input.ninters; i++) {
	    vft = ilist[i].firstv->pos;
	    avft = after(ilist[i].firstv)->pos;
	    vsd = ilist[i].secondv->pos;
	    avsd = after(ilist[i].secondv)->pos;
	    if (((vft.x != avft.x) && (vsd.x != avsd.x)) ||
		((vft.x == avft.x) &&
		 !EQ_PT(vft, ilist[i]) &&
		 !EQ_PT(avft, ilist[i])) ||
		((vsd.x == avsd.x) &&
		 !EQ_PT(vsd, ilist[i]) && !EQ_PT(avsd, ilist[i]))) {
		rv = 0;
		fprintf(stderr, "\nintersection %d at %.3f %.3f\n",
			i, ilist[i].x, ilist[i].y);
		fprintf(stderr, "seg#1 : (%.3f, %.3f) (%.3f, %.3f)\n",
			(double) (ilist[i].firstv->pos.x)
			, (double) (ilist[i].firstv->pos.y)
			, (double) (after(ilist[i].firstv)->pos.x)
			, (double) (after(ilist[i].firstv)->pos.y));
		fprintf(stderr, "seg#2 : (%.3f, %.3f) (%.3f, %.3f)\n",
			(double) (ilist[i].secondv->pos.x)
			, (double) (ilist[i].secondv->pos.y)
			, (double) (after(ilist[i].secondv)->pos.x)
			, (double) (after(ilist[i].secondv)->pos.y));
	    }
	}
    }
    free(polygon_list);
    free(vertex_list);
    return rv;
}
