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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

 /* until we can include cgraph.h or something else */
typedef enum { AGWARN, AGERR, AGMAX, AGPREV } agerrlevel_t;
extern int agerr(agerrlevel_t level, char *fmt, ...);

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "delaunay.h"
#include "memory.h"
#include "logic.h"

#if HAVE_GTS
#include <gts.h>

static gboolean triangle_is_hole(GtsTriangle * t)
{
    GtsEdge *e1, *e2, *e3;
    GtsVertex *v1, *v2, *v3;
    gboolean ret;

    gts_triangle_vertices_edges(t, NULL, &v1, &v2, &v3, &e1, &e2, &e3);

    if ((GTS_IS_CONSTRAINT(e1) && GTS_SEGMENT(e1)->v1 != v1) ||
	(GTS_IS_CONSTRAINT(e2) && GTS_SEGMENT(e2)->v1 != v2) ||
	(GTS_IS_CONSTRAINT(e3) && GTS_SEGMENT(e3)->v1 != v3))
	ret = TRUE;
    else ret = FALSE;
    return ret;
}

static guint delaunay_remove_holes(GtsSurface * surface)
{
    return gts_surface_foreach_face_remove(surface,
				    (GtsFunc) triangle_is_hole, NULL);
}

/* Derived classes for vertices and faces so we can assign integer ids
 * to make it easier to identify them. In particular, this allows the
 * segments and faces to refer to vertices using the order in which
 * they were passed in. 
 */
typedef struct {
    GtsVertex v;
    int idx;
} GVertex;

typedef struct {
    GtsVertexClass parent_class;
} GVertexClass;

static GVertexClass *g_vertex_class(void)
{
    static GVertexClass *klass = NULL;

    if (klass == NULL) {
	GtsObjectClassInfo vertex_info = {
	    "GVertex",
	    sizeof(GVertex),
	    sizeof(GVertexClass),
	    (GtsObjectClassInitFunc) NULL,
	    (GtsObjectInitFunc) NULL,
	    (GtsArgSetFunc) NULL,
	    (GtsArgGetFunc) NULL
	};
	klass = gts_object_class_new(GTS_OBJECT_CLASS(gts_vertex_class()),
				     &vertex_info);
    }

    return klass;
}

typedef struct {
    GtsFace v;
    int idx;
} GFace;

typedef struct {
    GtsFaceClass parent_class;
} GFaceClass;

static GFaceClass *g_face_class(void)
{
    static GFaceClass *klass = NULL;

    if (klass == NULL) {
	GtsObjectClassInfo face_info = {
	    "GFace",
	    sizeof(GFace),
	    sizeof(GFaceClass),
	    (GtsObjectClassInitFunc) NULL,
	    (GtsObjectInitFunc) NULL,
	    (GtsArgSetFunc) NULL,
	    (GtsArgGetFunc) NULL
	};
	klass = gts_object_class_new(GTS_OBJECT_CLASS(gts_face_class()),
				     &face_info);
    }

    return klass;
}

/* destroy:
 * Destroy each edge using v, then destroy v
 */
static void
destroy (GtsVertex* v)
{
    GSList * i;

    i = v->segments;
    while (i) {
	GSList * next = i->next;
	gts_object_destroy (i->data);
	i = next;
    }
    g_assert (v->segments == NULL);
    gts_object_destroy(GTS_OBJECT(v));
}

/* tri:
 * Main entry point to using GTS for triangulation.
 * Input is npt points with x and y coordinates stored either separately
 * in x[] and y[] (sepArr != 0) or consecutively in x[] (sepArr == 0).
 * Optionally, the input can include nsegs line segments, whose endpoint
 * indices are supplied in segs[2*i] and segs[2*i+1] yielding a constrained
 * triangulation.
 *
 * The return value is the corresponding gts surface, which can be queries for
 * the triangles and line segments composing the triangulation.
 */
static GtsSurface*
tri(double *x, double *y, int npt, int *segs, int nsegs, int sepArr)
{
    int i;
    GtsSurface *surface;
    GVertex **vertices = N_GNEW(npt, GVertex *);
    GtsEdge **edges = N_GNEW(nsegs, GtsEdge*);
    GSList *list = NULL;
    GtsVertex *v1, *v2, *v3;
    GtsTriangle *t;
    GtsVertexClass *vcl = (GtsVertexClass *) g_vertex_class();
    GtsEdgeClass *ecl = GTS_EDGE_CLASS (gts_constraint_class ());

    if (sepArr) {
	for (i = 0; i < npt; i++) {
	    GVertex *p = (GVertex *) gts_vertex_new(vcl, x[i], y[i], 0);
	    p->idx = i;
	    vertices[i] = p;
	}
    }
    else {
	for (i = 0; i < npt; i++) {
	    GVertex *p = (GVertex *) gts_vertex_new(vcl, x[2*i], x[2*i+1], 0);
	    p->idx = i;
	    vertices[i] = p;
	}
    }

    /* N.B. Edges need to be created here, presumably before the
     * the vertices are added to the face. In particular, they cannot
     * be added created and added vi gts_delaunay_add_constraint() below.
     */
    for (i = 0; i < nsegs; i++) {
	edges[i] = gts_edge_new(ecl,
		 (GtsVertex *) (vertices[ segs[ 2 * i]]),
		 (GtsVertex *) (vertices[ segs[ 2 * i + 1]]));
    }

    for (i = 0; i < npt; i++)
	list = g_slist_prepend(list, vertices[i]);
    t = gts_triangle_enclosing(gts_triangle_class(), list, 100.);
    g_slist_free(list);

    gts_triangle_vertices(t, &v1, &v2, &v3);

    surface = gts_surface_new(gts_surface_class(),
				  (GtsFaceClass *) g_face_class(),
				  gts_edge_class(),
				  gts_vertex_class());
    gts_surface_add_face(surface, gts_face_new(gts_face_class(),
					       t->e1, t->e2, t->e3));

    for (i = 0; i < npt; i++) {
	GtsVertex *v1 = (GtsVertex *) vertices[i];
	GtsVertex *v = gts_delaunay_add_vertex(surface, v1, NULL);

	/* if v != NULL, it is a previously added pt with the same
	 * coordinates as v1, in which case we replace v1 with v
	 */
	if (v) {
	    /* agerr (AGWARN, "Duplicate point %d %d\n", i, ((GVertex*)v)->idx); */
	    gts_vertex_replace (v1, v);
	}
    }

    for (i = 0; i < nsegs; i++) {
	gts_delaunay_add_constraint(surface,GTS_CONSTRAINT(edges[i]));
    }

    /* destroy enclosing triangle */
    gts_allow_floating_vertices = TRUE;
    gts_allow_floating_edges = TRUE;
/*
    gts_object_destroy(GTS_OBJECT(v1));
    gts_object_destroy(GTS_OBJECT(v2));
    gts_object_destroy(GTS_OBJECT(v3));
*/
    destroy(v1);
    destroy(v2);
    destroy(v3);
    gts_allow_floating_edges = FALSE;
    gts_allow_floating_vertices = FALSE;

    if (nsegs)
	delaunay_remove_holes(surface);

    free (edges);
    free(vertices);
    return surface;
}

typedef struct {
    int n;
    v_data *delaunay;
} estats;
    
static void cnt_edge (GtsSegment * e, estats* sp)
{
    sp->n++;
    if (sp->delaunay) {
	sp->delaunay[((GVertex*)(e->v1))->idx].nedges++;
	sp->delaunay[((GVertex*)(e->v2))->idx].nedges++;
    }
}

static void
edgeStats (GtsSurface* s, estats* sp)
{
    gts_surface_foreach_edge (s, (GtsFunc) cnt_edge, sp);
}

static void add_edge (GtsSegment * e, v_data* delaunay)
{
    int source = ((GVertex*)(e->v1))->idx;
    int dest = ((GVertex*)(e->v2))->idx;

    delaunay[source].edges[delaunay[source].nedges++] = dest;
    delaunay[dest].edges[delaunay[dest].nedges++] = source;
}

v_data *delaunay_triangulation(double *x, double *y, int n)
{
    v_data *delaunay;
    GtsSurface* s = tri(x, y, n, NULL, 0, 1);
    int i, nedges;
    int* edges;
    estats stats;

    if (!s) return NULL;

    delaunay = N_GNEW(n, v_data);

    for (i = 0; i < n; i++) {
	delaunay[i].ewgts = NULL;
	delaunay[i].nedges = 1;
    }

    stats.n = 0;
    stats.delaunay = delaunay;
    edgeStats (s, &stats);
    nedges = stats.n;
    edges = N_GNEW(2 * nedges + n, int);

    for (i = 0; i < n; i++) {
	delaunay[i].edges = edges;
	edges += delaunay[i].nedges;
	delaunay[i].edges[0] = i;
	delaunay[i].nedges = 1;
    }
    gts_surface_foreach_edge (s, (GtsFunc) add_edge, delaunay);

    gts_object_destroy (GTS_OBJECT (s));

    return delaunay;
}

typedef struct {
    int n;
    int* edges;
} estate;

static void addEdge (GtsSegment * e, estate* es)
{
    int source = ((GVertex*)(e->v1))->idx;
    int dest = ((GVertex*)(e->v2))->idx;

    es->edges[2*(es->n)] = source;
    es->edges[2*(es->n)+1] = dest;
    es->n += 1;
}

/* If qsort_r ever becomes standardized, this should be used
 * instead of having a global variable.
 */
static double* _vals;
typedef int (*qsort_cmpf) (const void *, const void *);

static int 
vcmp (int* a, int* b)
{
    double va = _vals[*a];
    double vb = _vals[*b];

    if (va < vb) return -1; 
    else if (va > vb) return 1; 
    else return 0;
}

/* delaunay_tri:
 * Given n points whose coordinates are in the x[] and y[]
 * arrays, compute a Delaunay triangulation of the points.
 * The number of edges in the triangulation is returned in pnedges.
 * The return value itself is an array e of 2*(*pnedges) integers,
 * with edge i having points whose indices are e[2*i] and e[2*i+1].
 *
 * If the points are collinear, GTS fails with 0 edges.
 * In this case, we sort the points by x coordinates (or y coordinates
 * if the points form a vertical line). We then return a "triangulation"
 * consisting of the n-1 pairs of adjacent points.
 */
int *delaunay_tri(double *x, double *y, int n, int* pnedges)
{
    GtsSurface* s = tri(x, y, n, NULL, 0, 1);
    int nedges;
    int* edges;
    estats stats;
    estate state;

    if (!s) return NULL;

    stats.n = 0;
    stats.delaunay = NULL;
    edgeStats (s, &stats);
    *pnedges = nedges = stats.n;

    if (nedges) {
	edges = N_GNEW(2 * nedges, int);
	state.n = 0;
	state.edges = edges;
	gts_surface_foreach_edge (s, (GtsFunc) addEdge, &state);
    }
    else {
	int* vs = N_GNEW(n, int);
	int* ip;
	int i, hd, tl;

	*pnedges = nedges = n-1;
	ip = edges = N_GNEW(2 * nedges, int);

	for (i = 0; i < n; i++)
	    vs[i] = i;

	if (x[0] == x[1])  /* vertical line */ 
	    _vals = y;
	else              
	    _vals = x;
	qsort (vs, n, sizeof(int), (qsort_cmpf)vcmp);

	tl = vs[0];
	for (i = 1; i < n; i++) {
	    hd = vs[i];
	    *ip++ = tl;
	    *ip++ = hd;
	    tl = hd;
	}

	free (vs);
    }

    gts_object_destroy (GTS_OBJECT (s));

    return edges;
}

static void cntFace (GFace* fp, int* ip)
{
    fp->idx = *ip;
    *ip += 1;
}

typedef struct {
    GtsSurface* s;
    int* faces;
    int* neigh;
} fstate;

typedef struct {
    int nneigh;
    int* neigh;
} ninfo;

static void addNeighbor (GFace* f, ninfo* es)
{
    es->neigh[es->nneigh] = f->idx;
    es->nneigh++;
}

static void addFace (GFace* f, fstate* es)
{
    int i, myid = f->idx;
    int* ip = es->faces + 3*myid;
    int* neigh = es->neigh + 3*myid;
    ninfo ni;
    GtsVertex *v1, *v2, *v3;

    gts_triangle_vertices (&f->v.triangle, &v1, &v2, &v3);
    *ip++ = ((GVertex*)(v1))->idx;
    *ip++ = ((GVertex*)(v2))->idx;
    *ip++ = ((GVertex*)(v3))->idx;

    ni.nneigh = 0;
    ni.neigh = neigh;
    gts_face_foreach_neighbor ((GtsFace*)f, 0, (GtsFunc) addNeighbor, &ni);
    for (i = ni.nneigh; i < 3; i++)
	neigh[i] = -1;
}

static void addTri (GFace* f, fstate* es)
{
    int myid = f->idx;
    int* ip = es->faces + 3*myid;
    GtsVertex *v1, *v2, *v3;

    gts_triangle_vertices (&f->v.triangle, &v1, &v2, &v3);
    *ip++ = ((GVertex*)(v1))->idx;
    *ip++ = ((GVertex*)(v2))->idx;
    *ip++ = ((GVertex*)(v3))->idx;
}

/* mkSurface:
 * Given n points whose coordinates are in x[] and y[], and nsegs line
 * segments whose end point indices are given in segs, return a surface
 * corresponding the constrained Delaunay triangulation.
 * The surface records the line segments, the triangles, and the neighboring
 * triangles.
 */
surface_t* 
mkSurface (double *x, double *y, int n, int* segs, int nsegs)
{
    GtsSurface* s = tri(x, y, n, segs, nsegs, 1);
    estats stats;
    estate state;
    fstate statf;
    surface_t* sf;
    int nfaces = 0;
    int* faces; 
    int* neigh; 

    if (!s) return NULL;

    sf = GNEW(surface_t);
    stats.n = 0;
    stats.delaunay = NULL;
    edgeStats (s, &stats);
    nsegs = stats.n;
    segs = N_GNEW(2 * nsegs, int);

    state.n = 0;
    state.edges = segs;
    gts_surface_foreach_edge (s, (GtsFunc) addEdge, &state);

    gts_surface_foreach_face (s, (GtsFunc) cntFace, &nfaces);

    faces = N_GNEW(3 * nfaces, int);
    neigh = N_GNEW(3 * nfaces, int);

    statf.faces = faces;
    statf.neigh = neigh;
    gts_surface_foreach_face (s, (GtsFunc) addFace, &statf);

    sf->nedges = nsegs;
    sf->edges = segs;
    sf->nfaces = nfaces;
    sf->faces = faces;
    sf->neigh = neigh;

    gts_object_destroy (GTS_OBJECT (s));

    return sf;
}

/* get_triangles:
 * Given n points whose coordinates are stored as (x[2*i],x[2*i+1]),
 * compute a Delaunay triangulation of the points.
 * The number of triangles in the triangulation is returned in tris.
 * The return value t is an array of 3*(*tris) integers,
 * with triangle i having points whose indices are t[3*i], t[3*i+1] and t[3*i+2].
 */
int* 
get_triangles (double *x, int n, int* tris)
{
    GtsSurface* s;
    int nfaces = 0;
    fstate statf;

    if (n <= 2) return NULL;

    s = tri(x, NULL, n, NULL, 0, 0);
    if (!s) return NULL;

    gts_surface_foreach_face (s, (GtsFunc) cntFace, &nfaces);
    statf.faces = N_GNEW(3 * nfaces, int);
    gts_surface_foreach_face (s, (GtsFunc) addTri, &statf);

    gts_object_destroy (GTS_OBJECT (s));

    *tris = nfaces;
    return statf.faces;
}

void 
freeSurface (surface_t* s)
{
    free (s->edges);
    free (s->faces);
    free (s->neigh);
}
#elif HAVE_TRIANGLE
#define TRILIBRARY
#include "triangle.c"
#include "assert.h"
#include "general.h"

int*
get_triangles (double *x, int n, int* tris)
{
    struct triangulateio in, mid, vorout;
    int i;

    if (n <= 2) return NULL;

    in.numberofpoints = n;
    in.numberofpointattributes = 0;
    in.pointlist = (REAL *) N_GNEW(in.numberofpoints * 2, REAL);

    for (i = 0; i < n; i++){
	in.pointlist[i*2] = x[i*2];
	in.pointlist[i*2 + 1] = x[i*2 + 1];
    }
    in.pointattributelist = NULL;
    in.pointmarkerlist = NULL;
    in.numberofsegments = 0;
    in.numberofholes = 0;
    in.numberofregions = 0;
    in.regionlist = NULL;
    mid.pointlist = (REAL *) NULL;            /* Not needed if -N switch used. */
    mid.pointattributelist = (REAL *) NULL;
    mid.pointmarkerlist = (int *) NULL; /* Not needed if -N or -B switch used. */
    mid.trianglelist = (int *) NULL;          /* Not needed if -E switch used. */
    mid.triangleattributelist = (REAL *) NULL;
    mid.neighborlist = (int *) NULL;         /* Needed only if -n switch used. */
    mid.segmentlist = (int *) NULL;
    mid.segmentmarkerlist = (int *) NULL;
    mid.edgelist = (int *) NULL;             /* Needed only if -e switch used. */
    mid.edgemarkerlist = (int *) NULL;   /* Needed if -e used and -B not used. */
    vorout.pointlist = (REAL *) NULL;        /* Needed only if -v switch used. */
    vorout.pointattributelist = (REAL *) NULL;
    vorout.edgelist = (int *) NULL;          /* Needed only if -v switch used. */
    vorout.normlist = (REAL *) NULL;         /* Needed only if -v switch used. */

    /* Triangulate the points.  Switches are chosen to read and write a  */
    /*   PSLG (p), preserve the convex hull (c), number everything from  */
    /*   zero (z), assign a regional attribute to each element (A), and  */
    /*   produce an edge list (e), a Voronoi diagram (v), and a triangle */
    /*   neighbor list (n).                                              */

    triangulate("Qenv", &in, &mid, &vorout);
    assert (mid.numberofcorners == 3);

    *tris = mid.numberoftriangles;
    
    FREE(in.pointlist);
    FREE(in.pointattributelist);
    FREE(in.pointmarkerlist);
    FREE(in.regionlist);
    FREE(mid.pointlist);
    FREE(mid.pointattributelist);
    FREE(mid.pointmarkerlist);
    FREE(mid.triangleattributelist);
    FREE(mid.neighborlist);
    FREE(mid.segmentlist);
    FREE(mid.segmentmarkerlist);
    FREE(mid.edgelist);
    FREE(mid.edgemarkerlist);
    FREE(vorout.pointlist);
    FREE(vorout.pointattributelist);
    FREE(vorout.edgelist);
    FREE(vorout.normlist);

    return mid.trianglelist;
}

// maybe it should be replaced by RNG - relative neigborhood graph, or by GG - gabriel graph
int* 
delaunay_tri (double *x, double *y, int n, int* nedges)
{
    struct triangulateio in, out;
    int i;

    in.pointlist = N_GNEW(2 * n, REAL);
    for (i = 0; i < n; i++) {
	in.pointlist[2 * i] = x[i];
	in.pointlist[2 * i + 1] = y[i];
    }

    in.pointattributelist = NULL;
    in.pointmarkerlist = NULL;
    in.numberofpoints = n;
    in.numberofpointattributes = 0;
    in.trianglearealist = NULL;
    in.triangleattributelist = NULL;
    in.numberoftriangleattributes = 0;
    in.neighborlist = NULL;
    in.segmentlist = NULL;
    in.segmentmarkerlist = NULL;
    in.holelist = NULL;
    in.numberofholes = 0;
    in.regionlist = NULL;
    in.edgelist = NULL;
    in.edgemarkerlist = NULL;
    in.normlist = NULL;

    out.pointattributelist = NULL;
    out.pointmarkerlist = NULL;
    out.numberofpoints = n;
    out.numberofpointattributes = 0;
    out.trianglearealist = NULL;
    out.triangleattributelist = NULL;
    out.numberoftriangleattributes = 0;
    out.neighborlist = NULL;
    out.segmentlist = NULL;
    out.segmentmarkerlist = NULL;
    out.holelist = NULL;
    out.numberofholes = 0;
    out.regionlist = NULL;
    out.edgelist = NULL;
    out.edgemarkerlist = NULL;
    out.normlist = NULL;

    triangulate("zQNEeB", &in, &out, NULL);

    *nedges = out.numberofedges;
    free (in.pointlist);
    free (in.pointattributelist);
    free (in.pointmarkerlist);
    free (in.trianglearealist);
    free (in.triangleattributelist);
    free (in.neighborlist);
    free (in.segmentlist);
    free (in.segmentmarkerlist);
    free (in.holelist);
    free (in.regionlist);
    free (in.edgemarkerlist);
    free (in.normlist);
    free (out.pointattributelist);
    free (out.pointmarkerlist);
    free (out.trianglearealist);
    free (out.triangleattributelist);
    free (out.neighborlist);
    free (out.segmentlist);
    free (out.segmentmarkerlist);
    free (out.holelist);
    free (out.regionlist);
    free (out.edgemarkerlist);
    free (out.normlist);
    return out.edgelist;
}

v_data *delaunay_triangulation(double *x, double *y, int n)
{
    v_data *delaunay;
    int nedges;
    int *edges;
    int source, dest;
    int* edgelist = delaunay_tri (x, y, n, &nedges);
    int i;

    delaunay = N_GNEW(n, v_data);
    edges = N_GNEW(2 * nedges + n, int);

    for (i = 0; i < n; i++) {
	delaunay[i].ewgts = NULL;
	delaunay[i].nedges = 1;
    }

    for (i = 0; i < 2 * nedges; i++)
	delaunay[edgelist[i]].nedges++;

    for (i = 0; i < n; i++) {
	delaunay[i].edges = edges;
	edges += delaunay[i].nedges;
	delaunay[i].edges[0] = i;
	delaunay[i].nedges = 1;
    }
    for (i = 0; i < nedges; i++) {
	source = edgelist[2 * i];
	dest = edgelist[2 * i + 1];
	delaunay[source].edges[delaunay[source].nedges++] = dest;
	delaunay[dest].edges[delaunay[dest].nedges++] = source;
    }

    free(edgelist);
    return delaunay;
}

surface_t* 
mkSurface (double *x, double *y, int n, int* segs, int nsegs)
{
    agerr (AGERR, "mkSurface not yet implemented using Triangle library\n");
    assert (0);
    return 0;
}
void 
freeSurface (surface_t* s)
{
    agerr (AGERR, "freeSurface not yet implemented using Triangle library\n");
    assert (0);
}
#else
static char* err = "Graphviz built without any triangulation library\n";
int* get_triangles (double *x, int n, int* tris)
{
    agerr(AGERR, "get_triangles: %s\n", err);
    return 0;
}
v_data *delaunay_triangulation(double *x, double *y, int n)
{
    agerr(AGERR, "delaunay_triangulation: %s\n", err);
    return 0;
}
int *delaunay_tri(double *x, double *y, int n, int* nedges)
{
    agerr(AGERR, "delaunay_tri: %s\n", err);
    return 0;
}
surface_t* 
mkSurface (double *x, double *y, int n, int* segs, int nsegs)
{
    agerr(AGERR, "mkSurface: %s\n", err);
    return 0;
}
void 
freeSurface (surface_t* s)
{
    agerr (AGERR, "freeSurface: %s\n", err);
}
#endif

static void remove_edge(v_data * graph, int source, int dest)
{
    int i;
    for (i = 1; i < graph[source].nedges; i++) {
	if (graph[source].edges[i] == dest) {
	    graph[source].edges[i] =
		graph[source].edges[--graph[source].nedges];
	    break;
	}
    }
}

v_data *UG_graph(double *x, double *y, int n, int accurate_computation)
{
    v_data *delaunay;
    int i;
    double dist_ij, dist_ik, dist_jk, x_i, y_i, x_j, y_j;
    int j, k, neighbor_j, neighbor_k;
    int removed;

    if (n == 2) {
	int *edges = N_GNEW(4, int);
	delaunay = N_GNEW(n, v_data);
	delaunay[0].ewgts = NULL;
	delaunay[0].edges = edges;
	delaunay[0].nedges = 2;
	delaunay[0].edges[0] = 0;
	delaunay[0].edges[1] = 1;
	delaunay[1].edges = edges + 2;
	delaunay[1].ewgts = NULL;
	delaunay[1].nedges = 2;
	delaunay[1].edges[0] = 1;
	delaunay[1].edges[1] = 0;
	return delaunay;
    } else if (n == 1) {
	int *edges = N_GNEW(1, int);
	delaunay = N_GNEW(n, v_data);
	delaunay[0].ewgts = NULL;
	delaunay[0].edges = edges;
	delaunay[0].nedges = 1;
	delaunay[0].edges[0] = 0;
	return delaunay;
    }

    delaunay = delaunay_triangulation(x, y, n);

    if (accurate_computation) {
	for (i = 0; i < n; i++) {
	    x_i = x[i];
	    y_i = y[i];
	    for (j = 1; j < delaunay[i].nedges;) {
		neighbor_j = delaunay[i].edges[j];
		if (neighbor_j < i) {
		    j++;
		    continue;
		}
		x_j = x[neighbor_j];
		y_j = y[neighbor_j];
		dist_ij =
		    (x_j - x_i) * (x_j - x_i) + (y_j - y_i) * (y_j - y_i);
		removed = FALSE;
		for (k = 0; k < n && !removed; k++) {
		    dist_ik =
			(x[k] - x_i) * (x[k] - x_i) + (y[k] -
						       y_i) * (y[k] - y_i);
		    if (dist_ik < dist_ij) {
			dist_jk =
			    (x[k] - x_j) * (x[k] - x_j) + (y[k] -
							   y_j) * (y[k] -
								   y_j);
			if (dist_jk < dist_ij) {
			    // remove the edge beteween i and neighbor j
			    delaunay[i].edges[j] =
				delaunay[i].edges[--delaunay[i].nedges];
			    remove_edge(delaunay, neighbor_j, i);
			    removed = TRUE;
			}
		    }
		}
		if (!removed) {
		    j++;
		}
	    }
	}
    } else {
	// remove all edges v-u if there is w, neighbor of u or v, that is closer to both u and v than dist(u,v)
	for (i = 0; i < n; i++) {
	    x_i = x[i];
	    y_i = y[i];
	    for (j = 1; j < delaunay[i].nedges;) {
		neighbor_j = delaunay[i].edges[j];
		x_j = x[neighbor_j];
		y_j = y[neighbor_j];
		dist_ij =
		    (x_j - x_i) * (x_j - x_i) + (y_j - y_i) * (y_j - y_i);
		// now look at i'th neighbors to see whether there is a node in the "forbidden region"
		// we will also go through neighbor_j's neighbors when we traverse the edge from its other side
		removed = FALSE;
		for (k = 1; k < delaunay[i].nedges && !removed; k++) {
		    neighbor_k = delaunay[i].edges[k];
		    dist_ik =
			(x[neighbor_k] - x_i) * (x[neighbor_k] - x_i) +
			(y[neighbor_k] - y_i) * (y[neighbor_k] - y_i);
		    if (dist_ik < dist_ij) {
			dist_jk =
			    (x[neighbor_k] - x_j) * (x[neighbor_k] - x_j) +
			    (y[neighbor_k] - y_j) * (y[neighbor_k] - y_j);
			if (dist_jk < dist_ij) {
			    // remove the edge beteween i and neighbor j
			    delaunay[i].edges[j] =
				delaunay[i].edges[--delaunay[i].nedges];
			    remove_edge(delaunay, neighbor_j, i);
			    removed = TRUE;
			}
		    }
		}
		if (!removed) {
		    j++;
		}
	    }
	}
    }
    return delaunay;
}

void freeGraph (v_data * graph)
{
    if (graph != NULL) {
	if (graph[0].edges != NULL)
	    free(graph[0].edges);
	if (graph[0].ewgts != NULL)
	    free(graph[0].ewgts);
	free(graph);
    }
}

void freeGraphData(vtx_data * graph)
{
    if (graph != NULL) {
	if (graph[0].edges != NULL)
	    free(graph[0].edges);
	if (graph[0].ewgts != NULL)
	    free(graph[0].ewgts);
#ifdef USE_STYLES
	if (graph[0].styles != NULL)
	    free(graph[0].styles);
#endif
#ifdef DIGCOLA
	if (graph[0].edists != NULL)
	    free(graph[0].edists);
#endif
	free(graph);
    }
}

