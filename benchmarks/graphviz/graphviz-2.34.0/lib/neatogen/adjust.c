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

/* adjust.c
 * Routines for repositioning nodes after initial layout in
 * order to reduce/remove node overlaps.
 */

#include "neato.h"
#include "agxbuf.h"
#include "utils.h"
#include "ctype.h"
#include "voronoi.h"
#include "info.h"
#include "edges.h"
#include "site.h"
#include "heap.h"
#include "hedges.h"
#include "digcola.h"
#if ((defined(HAVE_GTS) || defined(HAVE_TRIANGLE)) && defined(SFDP))
#include "overlap.h"
#endif
#ifdef IPSEPCOLA
#include "csolve_VPSC.h"
#include "quad_prog_vpsc.h"
#endif

#define SEPFACT         0.8  /* default esep/sep */

static double margin = 0.05;	/* Create initial bounding box by adding
				 * margin * dimension around box enclosing
				 * nodes.
				 */
static double incr = 0.05;	/* Increase bounding box by adding
				 * incr * dimension around box.
				 */
static int iterations = -1;	/* Number of iterations */
static int useIter = 0;		/* Use specified number of iterations */

static int doAll = 0;		/* Move all nodes, regardless of overlap */
static Site **sites;		/* Array of pointers to sites; used in qsort */
static Site **endSite;		/* Sentinel on sites array */
static Point nw, ne, sw, se;	/* Corners of clipping window */

static Site **nextSite;

static void setBoundBox(Point * ll, Point * ur)
{
    pxmin = ll->x;
    pxmax = ur->x;
    pymin = ll->y;
    pymax = ur->y;
    nw.x = sw.x = pxmin;
    ne.x = se.x = pxmax;
    nw.y = ne.y = pymax;
    sw.y = se.y = pymin;
}

 /* freeNodes:
  * Free node resources.
  */
static void freeNodes(void)
{
    int i;
    Info_t *ip = nodeInfo;

    for (i = 0; i < nsites; i++) {
	breakPoly(&ip->poly);
	ip++;
    }
    polyFree();
    infoinit();			/* Free vertices */
    free(nodeInfo);
}

/* chkBoundBox:
 *   Compute extremes of graph, then set up bounding box.
 *   If user supplied a bounding box, use that;
 *   else if "window" is a graph attribute, use that; 
 *   otherwise, define bounding box as a percentage expansion of
 *   graph extremes.
 *   In the first two cases, check that graph fits in bounding box.
 */
static void chkBoundBox(Agraph_t * graph)
{
    char *marg;
    Point ll, ur;
    int i;
    double x, y;
    double xmin, xmax, ymin, ymax;
    double xmn, xmx, ymn, ymx;
    double ydelta, xdelta;
    Info_t *ip;
    Poly *pp;
    /* int          cnt; */

    ip = nodeInfo;
    pp = &ip->poly;
    x = ip->site.coord.x;
    y = ip->site.coord.y;
    xmin = pp->origin.x + x;
    ymin = pp->origin.y + y;
    xmax = pp->corner.x + x;
    ymax = pp->corner.y + y;
    for (i = 1; i < nsites; i++) {
	ip++;
	pp = &ip->poly;
	x = ip->site.coord.x;
	y = ip->site.coord.y;
	xmn = pp->origin.x + x;
	ymn = pp->origin.y + y;
	xmx = pp->corner.x + x;
	ymx = pp->corner.y + y;
	if (xmn < xmin)
	    xmin = xmn;
	if (ymn < ymin)
	    ymin = ymn;
	if (xmx > xmax)
	    xmax = xmx;
	if (ymx > ymax)
	    ymax = ymx;
    }

    marg = agget(graph, "voro_margin");
    if (marg && (*marg != '\0')) {
	margin = atof(marg);
    }
    ydelta = margin * (ymax - ymin);
    xdelta = margin * (xmax - xmin);
    ll.x = xmin - xdelta;
    ll.y = ymin - ydelta;
    ur.x = xmax + xdelta;
    ur.y = ymax + ydelta;

    setBoundBox(&ll, &ur);
}

 /* makeInfo:
  * For each node in the graph, create a Info data structure 
  */
static int makeInfo(Agraph_t * graph)
{
    Agnode_t *node;
    int i;
    Info_t *ip;
    expand_t pmargin;
    int (*polyf)(Poly *, Agnode_t *, float, float);

    nsites = agnnodes(graph);
    geominit();

    nodeInfo = N_GNEW(nsites, Info_t);

    node = agfstnode(graph);
    ip = nodeInfo;

    pmargin = sepFactor (graph);

    if (pmargin.doAdd) {
	polyf = makeAddPoly;
	/* we need inches for makeAddPoly */
	pmargin.x = PS2INCH(pmargin.x);
	pmargin.y = PS2INCH(pmargin.y);
    }
	
    else polyf = makePoly;
    for (i = 0; i < nsites; i++) {
	ip->site.coord.x = ND_pos(node)[0];
	ip->site.coord.y = ND_pos(node)[1];

	if (polyf(&ip->poly, node, pmargin.x, pmargin.y)) {
	    free (nodeInfo);
	    nodeInfo = NULL;
	    return 1;
        }

	ip->site.sitenbr = i;
	ip->site.refcnt = 1;
	ip->node = node;
	ip->verts = NULL;
	node = agnxtnode(graph, node);
	ip++;
    }
    return 0;
}

/* sort sites on y, then x, coord */
static int scomp(const void *S1, const void *S2)
{
    Site *s1, *s2;

    s1 = *(Site **) S1;
    s2 = *(Site **) S2;
    if (s1->coord.y < s2->coord.y)
	return (-1);
    if (s1->coord.y > s2->coord.y)
	return (1);
    if (s1->coord.x < s2->coord.x)
	return (-1);
    if (s1->coord.x > s2->coord.x)
	return (1);
    return (0);
}

 /* sortSites:
  * Fill array of pointer to sites and sort the sites using scomp
  */
static void sortSites(void)
{
    int i;
    Site **sp;
    Info_t *ip;

    if (sites == 0) {
	sites = N_GNEW(nsites, Site *);
	endSite = sites + nsites;
    }

    sp = sites;
    ip = nodeInfo;
    infoinit();
    for (i = 0; i < nsites; i++) {
	*sp++ = &(ip->site);
	ip->verts = NULL;
	ip->site.refcnt = 1;
	ip++;
    }

    qsort(sites, nsites, sizeof(Site *), scomp);

    /* Reset site index for nextOne */
    nextSite = sites;

}

static void geomUpdate(int doSort)
{
    int i;

    if (doSort)
	sortSites();

    /* compute ranges */
    xmin = sites[0]->coord.x;
    xmax = sites[0]->coord.x;
    for (i = 1; i < nsites; i++) {
	if (sites[i]->coord.x < xmin)
	    xmin = sites[i]->coord.x;
	if (sites[i]->coord.x > xmax)
	    xmax = sites[i]->coord.x;
    }
    ymin = sites[0]->coord.y;
    ymax = sites[nsites - 1]->coord.y;

    deltay = ymax - ymin;
    deltax = xmax - xmin;
}

static Site *nextOne(void)
{
    Site *s;

    if (nextSite < endSite) {
	s = *nextSite++;
	return (s);
    } else
	return ((Site *) NULL);
}

/* rmEquality:
 * Check for nodes with identical positions and tweak
 * the positions.
 */
static void rmEquality(void)
{
    int i, cnt;
    Site **ip;
    Site **jp;
    Site **kp;
    double xdel;

    sortSites();
    ip = sites;

    while (ip < endSite) {
	jp = ip + 1;
	if ((jp >= endSite) ||
	    ((*jp)->coord.x != (*ip)->coord.x) ||
	    ((*jp)->coord.y != (*ip)->coord.y)) {
	    ip = jp;
	    continue;
	}

	/* Find first node kp with position different from ip */
	cnt = 2;
	kp = jp + 1;
	while ((kp < endSite) &&
	       ((*kp)->coord.x == (*ip)->coord.x) &&
	       ((*kp)->coord.y == (*ip)->coord.y)) {
	    cnt++;
	    jp = kp;
	    kp = jp + 1;
	}

	/* If next node exists and is on the same line */
	if ((kp < endSite) && ((*kp)->coord.y == (*ip)->coord.y)) {
	    xdel = ((*kp)->coord.x - (*ip)->coord.x) / cnt;
	    i = 1;
	    for (jp = ip + 1; jp < kp; jp++) {
		(*jp)->coord.x += i * xdel;
		i++;
	    }
	} else {		/* nothing is to the right */
	    Info_t *info;
	    for (jp = ip + 1; jp < kp; ip++, jp++) {
		info = nodeInfo + (*ip)->sitenbr;
		xdel = info->poly.corner.x - info->poly.origin.x;
		info = nodeInfo + (*jp)->sitenbr;
		xdel += info->poly.corner.x - info->poly.origin.x;
		(*jp)->coord.x = (*ip)->coord.x + xdel / 2;
	    }
	}
	ip = kp;
    }
}

/* countOverlap:
 * Count number of node-node overlaps at iteration iter.
 */
static int countOverlap(int iter)
{
    int count = 0;
    int i, j;
    Info_t *ip = nodeInfo;
    Info_t *jp;

    for (i = 0; i < nsites; i++)
	nodeInfo[i].overlaps = 0;

    for (i = 0; i < nsites - 1; i++) {
	jp = ip + 1;
	for (j = i + 1; j < nsites; j++) {
	    if (polyOverlap
		(ip->site.coord, &ip->poly, jp->site.coord, &jp->poly)) {
		count++;
		ip->overlaps = 1;
		jp->overlaps = 1;
	    }
	    jp++;
	}
	ip++;
    }

    if (Verbose > 1)
	fprintf(stderr, "overlap [%d] : %d\n", iter, count);
    return count;
}

static void increaseBoundBox(void)
{
    double ydelta, xdelta;
    Point ll, ur;

    ur.x = pxmax;
    ur.y = pymax;
    ll.x = pxmin;
    ll.y = pymin;

    ydelta = incr * (ur.y - ll.y);
    xdelta = incr * (ur.x - ll.x);

    ur.x += xdelta;
    ur.y += ydelta;
    ll.x -= xdelta;
    ll.y -= ydelta;

    setBoundBox(&ll, &ur);
}

 /* areaOf:
  * Area of triangle whose vertices are a,b,c
  */
static double areaOf(Point a, Point b, Point c)
{
    double area;

    area =
	(double) (fabs
		  (a.x * (b.y - c.y) + b.x * (c.y - a.y) +
		   c.x * (a.y - b.y)) / 2);
    return area;
}

 /* centroidOf:
  * Compute centroid of triangle with vertices a, b, c.
  * Return coordinates in x and y.
  */
static void centroidOf(Point a, Point b, Point c, double *x, double *y)
{
    *x = (a.x + b.x + c.x) / 3;
    *y = (a.y + b.y + c.y) / 3;
}

 /* newpos;
  * The new position is the centroid of the
  * voronoi polygon. This is the weighted sum of the
  * centroids of a triangulation, normalized to the
  * total area.
  */
static void newpos(Info_t * ip)
{
    PtItem *anchor = ip->verts;
    PtItem *p, *q;
    double totalArea = 0.0;
    double cx = 0.0;
    double cy = 0.0;
    double x;
    double y;
    double area;

    p = anchor->next;
    q = p->next;
    while (q != NULL) {
	area = areaOf(anchor->p, p->p, q->p);
	centroidOf(anchor->p, p->p, q->p, &x, &y);
	cx = cx + area * x;
	cy = cy + area * y;
	totalArea = totalArea + area;
	p = q;
	q = q->next;
    }

    ip->site.coord.x = cx / totalArea;
    ip->site.coord.y = cy / totalArea;
}

 /* addCorners:
  * Add corners of clipping window to appropriate sites.
  * A site gets a corner if it is the closest site to that corner.
  */
static void addCorners(void)
{
    Info_t *ip = nodeInfo;
    Info_t *sws = ip;
    Info_t *nws = ip;
    Info_t *ses = ip;
    Info_t *nes = ip;
    double swd = dist_2(&ip->site.coord, &sw);
    double nwd = dist_2(&ip->site.coord, &nw);
    double sed = dist_2(&ip->site.coord, &se);
    double ned = dist_2(&ip->site.coord, &ne);
    double d;
    int i;

    ip++;
    for (i = 1; i < nsites; i++) {
	d = dist_2(&ip->site.coord, &sw);
	if (d < swd) {
	    swd = d;
	    sws = ip;
	}
	d = dist_2(&ip->site.coord, &se);
	if (d < sed) {
	    sed = d;
	    ses = ip;
	}
	d = dist_2(&ip->site.coord, &nw);
	if (d < nwd) {
	    nwd = d;
	    nws = ip;
	}
	d = dist_2(&ip->site.coord, &ne);
	if (d < ned) {
	    ned = d;
	    nes = ip;
	}
	ip++;
    }

    addVertex(&sws->site, sw.x, sw.y);
    addVertex(&ses->site, se.x, se.y);
    addVertex(&nws->site, nw.x, nw.y);
    addVertex(&nes->site, ne.x, ne.y);
}

 /* newPos:
  * Calculate the new position of a site as the centroid
  * of its voronoi polygon, if it overlaps other nodes.
  * The polygons are finite by being clipped to the clipping
  * window.
  * We first add the corner of the clipping windows to the
  * vertex lists of the appropriate sites.
  */
static void newPos(void)
{
    int i;
    Info_t *ip = nodeInfo;

    addCorners();
    for (i = 0; i < nsites; i++) {
	if (doAll || ip->overlaps)
	    newpos(ip);
	ip++;
    }
}

/* cleanup:
 * Cleanup voronoi memory.
 * Note that PQcleanup and ELcleanup rely on the number
 * of sites, so should at least be reset everytime we use
 * vAdjust.
 * This could be optimized, over multiple components or
 * even multiple graphs, but probably not worth it.
 */
static void cleanup(void)
{
    PQcleanup();
    ELcleanup();
    siteinit();			/* free memory */
    edgeinit();			/* free memory */
}

static int vAdjust(void)
{
    int iterCnt = 0;
    int overlapCnt = 0;
    int badLevel = 0;
    int increaseCnt = 0;
    int cnt;

    if (!useIter || (iterations > 0))
	overlapCnt = countOverlap(iterCnt);

    if ((overlapCnt == 0) || (iterations == 0))
	return 0;

    rmEquality();
    geomUpdate(0);
    voronoi(0, nextOne);
    while (1) {
	newPos();
	iterCnt++;

	if (useIter && (iterCnt == iterations))
	    break;
	cnt = countOverlap(iterCnt);
	if (cnt == 0)
	    break;
	if (cnt >= overlapCnt)
	    badLevel++;
	else
	    badLevel = 0;
	overlapCnt = cnt;

	switch (badLevel) {
	case 0:
	    doAll = 1;
	    break;
/*
      case 1:
        doAll = 1;
        break;
*/
	default:
	    doAll = 1;
	    increaseCnt++;
	    increaseBoundBox();
	    break;
	}

	geomUpdate(1);
	voronoi(0, nextOne);
    }

    if (Verbose) {
	fprintf(stderr, "Number of iterations = %d\n", iterCnt);
	fprintf(stderr, "Number of increases = %d\n", increaseCnt);
    }

    cleanup();
    return 1;
}

static double rePos(Point c)
{
    int i;
    Info_t *ip = nodeInfo;
    double f = 1.0 + incr;

    for (i = 0; i < nsites; i++) {
	/* ip->site.coord.x = f*(ip->site.coord.x - c.x) + c.x; */
	/* ip->site.coord.y = f*(ip->site.coord.y - c.y) + c.y; */
	ip->site.coord.x = f * ip->site.coord.x;
	ip->site.coord.y = f * ip->site.coord.y;
	ip++;
    }
    return f;
}

static int sAdjust(void)
{
    int iterCnt = 0;
    int overlapCnt = 0;
    int cnt;
    Point center;
    /* double sc; */

    if (!useIter || (iterations > 0))
	overlapCnt = countOverlap(iterCnt);

    if ((overlapCnt == 0) || (iterations == 0))
	return 0;

    rmEquality();
    center.x = (pxmin + pxmax) / 2.0;
    center.y = (pymin + pymax) / 2.0;
    while (1) {
	/* sc = */ rePos(center);
	iterCnt++;

	if (useIter && (iterCnt == iterations))
	    break;
	cnt = countOverlap(iterCnt);
	if (cnt == 0)
	    break;
    }

    if (Verbose) {
	fprintf(stderr, "Number of iterations = %d\n", iterCnt);
    }

    return 1;
}

 /* updateGraph:
  * Enter new node positions into the graph
  */
static void updateGraph(Agraph_t * graph)
{
    /* Agnode_t*    node; */
    int i;
    Info_t *ip;
    /* char         pos[100]; */

    ip = nodeInfo;
    for (i = 0; i < nsites; i++) {
	ND_pos(ip->node)[0] = ip->site.coord.x;
	ND_pos(ip->node)[1] = ip->site.coord.y;
	ip++;
    }
}

#define ELS "|edgelabel|"
#define ELSN (sizeof(ELS)-1)
  /* Return true if node name starts with ELS */
#define IS_LNODE(n) (!strncmp(agnameof(n),ELS,ELSN))

/* getSizes:
 * Set up array of half sizes in inches.
 */
double *getSizes(Agraph_t * g, pointf pad, int* n_elabels, int** elabels)
{
    Agnode_t *n;
    real *sizes = N_GNEW(2 * agnnodes(g), real);
    int i, nedge_nodes;
    int* elabs;

    for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	if (elabels && IS_LNODE(n)) nedge_nodes++;

	i = ND_id(n);
	sizes[i * 2] = ND_width(n) * .5 + pad.x;
	sizes[i * 2 + 1] = ND_height(n) * .5 + pad.y;
    }

    if (elabels && nedge_nodes) {
	elabs = N_GNEW(nedge_nodes, int);
	nedge_nodes = 0;
	for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	    if (IS_LNODE(n))
		elabs[nedge_nodes++] = ND_id(n);
	}
	*elabels = elabs;
	*n_elabels = nedge_nodes;
    }

    return sizes;
}

/* makeMatrix:
 * Assumes g is connected and simple, i.e., we can have a->b and b->a
 * but not a->b and a->b
 */
SparseMatrix makeMatrix(Agraph_t* g, int dim, SparseMatrix *D)
{
    SparseMatrix A = 0;
    Agnode_t *n;
    Agedge_t *e;
    Agsym_t *sym;
    int nnodes;
    int nedges;
    int i, row;
    int *I;
    int *J;
    real *val;
    real v;
    int type = MATRIX_TYPE_REAL;
    Agsym_t* symD = NULL;
    real* valD = NULL;

    if (!g)
	return NULL;
    nnodes = agnnodes(g);
    nedges = agnedges(g);

    /* Assign node ids */
    i = 0;
    for (n = agfstnode(g); n; n = agnxtnode(g, n))
	ND_id(n) = i++;

    I = N_GNEW(nedges, int);
    J = N_GNEW(nedges, int);
    val = N_GNEW(nedges, real);

    sym = agfindedgeattr(g, "weight");
    if (D) {
	symD = agfindedgeattr(g, "len");
	valD = N_NEW(nedges, real);
    }

    i = 0;
    for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	row = ND_id(n);
	for (e = agfstout(g, n); e; e = agnxtout(g, e)) {
	    I[i] = row;
	    J[i] = ND_id(aghead(e));
#ifndef WITH_CGRAPH
	    if (!sym || (sscanf(agxget(e, sym->index), "%lf", &v) != 1))
#else
	    if (!sym || (sscanf(agxget(e, sym), "%lf", &v) != 1))
#endif
		v = 1;
	    val[i] = v;
	/* edge length */
	    if (symD) {
#ifndef WITH_CGRAPH
		if (sscanf (agxget (e, symD->index), "%lf", &v) != 1) v = 1;
#else
		if (sscanf (agxget (e, symD), "%lf", &v) != 1) v = 1;
#endif
		valD[i] = v;
	    }
	    i++;
	}
    }

    A = SparseMatrix_from_coordinate_arrays(nedges, nnodes, nnodes, I, J,
					    val, type, sizeof(real));

    if (D) *D = SparseMatrix_from_coordinate_arrays(nedges, nnodes, nnodes, I, J, valD, type, sizeof(real));

    free(I);
    free(J);
    free(val);
    if (valD) free (valD);

    return A;
}

#if ((defined(HAVE_GTS) || defined(HAVE_TRIANGLE)) && defined(SFDP))
static int
fdpAdjust (graph_t* g, adjust_data* am)
{
    SparseMatrix A0 = makeMatrix(g, Ndim, NULL);
    SparseMatrix A = A0;
    real *sizes;
    real *pos = N_NEW(Ndim * agnnodes(g), real);
    Agnode_t *n;
    int flag, i;
    expand_t sep = sepFactor(g);
    pointf pad;

    if (sep.doAdd) {
	pad.x = PS2INCH(sep.x);
	pad.y = PS2INCH(sep.y);
    } else {
	pad.x = PS2INCH(DFLT_MARGIN);
	pad.y = PS2INCH(DFLT_MARGIN);
    }
    sizes = getSizes(g, pad, NULL, NULL);

    for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	real* npos = pos + (Ndim * ND_id(n));
	for (i = 0; i < Ndim; i++) {
	    npos[i] = ND_pos(n)[i];
	}
    }

    if (!SparseMatrix_is_symmetric(A, FALSE)
	|| A->type != MATRIX_TYPE_REAL) {
	A = SparseMatrix_get_real_adjacency_matrix_symmetrized(A);
    } else {
	A = SparseMatrix_remove_diagonal(A);
    }

    remove_overlap(Ndim, A, pos, sizes, am->value, am->scaling, 
	ELSCHEME_NONE, 0, NULL, NULL, &flag);

    for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	real *npos = pos + (Ndim * ND_id(n));
	for (i = 0; i < Ndim; i++) {
	    ND_pos(n)[i] = npos[i];
	}
    }

    free(sizes);
    free(pos);
    if (A != A0)
	SparseMatrix_delete(A);
    SparseMatrix_delete (A0);

    return flag;
}
#endif

#ifdef IPSEPCOLA
static int
vpscAdjust(graph_t* G)
{
    int dim = 2;
    int nnodes = agnnodes(G);
    ipsep_options opt;
    pointf* nsize = N_GNEW(nnodes, pointf);
    float** coords = N_GNEW(dim, float*);
    float* f_storage = N_GNEW(dim * nnodes, float);
    int i, j;
    Agnode_t* v;
    expand_t margin;

    for (i = 0; i < dim; i++) {
	coords[i] = f_storage + i * nnodes;
    }

    j = 0;
    for (v = agfstnode(G); v; v = agnxtnode(G, v)) {
	for (i = 0; i < dim; i++) {
	    coords[i][j] =  (float) (ND_pos(v)[i]);
	}
	nsize[j].x = ND_width(v);
	nsize[j].y = ND_height(v);
	j++;
    }

    opt.diredges = 0;
    opt.edge_gap = 0;
    opt.noverlap = 2;
    opt.clusters = NEW(cluster_data);
    margin = sepFactor (G);
 	/* Multiply by 2 since opt.gap is the gap size, not the margin */
    if (margin.doAdd) {
	opt.gap.x = 2.0*PS2INCH(margin.x);
	opt.gap.y = 2.0*PS2INCH(margin.y);
    }
    else {
	opt.gap.x = opt.gap.y = 2.0*PS2INCH(DFLT_MARGIN);
    }
    opt.nsize = nsize;

    removeoverlaps(nnodes, coords, &opt);

    j = 0;
    for (v = agfstnode(G); v; v = agnxtnode(G, v)) {
	for (i = 0; i < dim; i++) {
	    ND_pos(v)[i] = coords[i][j];
	}
	j++;
    }

    free (opt.clusters);
    free (f_storage);
    free (coords);
    free (nsize);
    return 0;
}
#endif

/* angleSet:
 * Return true if "normalize" is defined and valid; return angle in phi.
 * Read angle as degrees, convert to radians.
 * Guarantee -PI < phi <= PI.
 */
static int
angleSet (graph_t* g, double* phi)
{
    double ang;
    char* p;
    char* a = agget(g, "normalize");

    if (!a || (*a == '\0'))
	return 0;
    ang = strtod (a, &p);
    if (p == a) {  /* no number */
	if (mapbool(a))
	    ang = 0.0;
	else
	    return 0;
    }
    while (ang > 180) ang -= 360;
    while (ang <= -180) ang += 360;

    *phi = RADIANS(ang);
    return 1;
}

/* normalize:
 * If normalize is set, move first node to origin, then
 * rotate graph so that the angle of the first edge is given
 * by the degrees from normalize.
 * FIX: Generalize to allow rotation determined by graph shape.
 */
int normalize(graph_t * g)
{
    node_t *v;
    edge_t *e;
    double phi;
    double cosv, sinv;
    pointf p, orig;
    int ret;

    if (!angleSet(g, &phi))
	return 0;

    v = agfstnode(g);
    p.x = ND_pos(v)[0];
    p.y = ND_pos(v)[1];
    for (v = agfstnode(g); v; v = agnxtnode(g, v)) {
	ND_pos(v)[0] -= p.x;
	ND_pos(v)[1] -= p.y;
    }
    if (p.x || p.y) ret = 1;
    else ret = 0;

    e = NULL;
    for (v = agfstnode(g); v; v = agnxtnode(g, v))
	if ((e = agfstout(g, v)))
	    break;
    if (e == NULL)
	return ret;

	/* rotation necessary; pos => ccw */
    phi -= atan2(ND_pos(aghead(e))[1] - ND_pos(agtail(e))[1],
		   ND_pos(aghead(e))[0] - ND_pos(agtail(e))[0]);

    if (phi) {
	orig.x = ND_pos(agtail(e))[0];
	orig.y = ND_pos(agtail(e))[1];
	cosv = cos(phi);
	sinv = sin(phi);
	for (v = agfstnode(g); v; v = agnxtnode(g, v)) {
	    p.x = ND_pos(v)[0] - orig.x;
	    p.y = ND_pos(v)[1] - orig.y;
	    ND_pos(v)[0] = p.x * cosv - p.y * sinv + orig.x;
	    ND_pos(v)[1] = p.x * sinv + p.y * cosv + orig.y;
	}
	return 1;
    }
    else return ret;
}

typedef struct {
    adjust_mode mode;
    char *attrib;
    int len;
    char *print;
} lookup_t;

#define STRLEN(s) ((sizeof(s)-1)/sizeof(char))
#define ITEM(i,s,v) {i, s, STRLEN(s), v}

/* Translation table from overlap values to algorithms.
 * adjustMode[0] corresponds to overlap=true
 * adjustMode[1] corresponds to overlap=false
 */
static lookup_t adjustMode[] = {
    ITEM(AM_NONE, "", "none"),
#if ((defined(HAVE_GTS) || defined(HAVE_TRIANGLE)) && defined(SFDP))
    ITEM(AM_PRISM, "prism", "prism"),
#endif
    ITEM(AM_VOR, "voronoi", "Voronoi"),
    ITEM(AM_NSCALE, "scale", "scaling"),
    ITEM(AM_COMPRESS, "compress", "compress"),
    ITEM(AM_VPSC, "vpsc", "vpsc"),
    ITEM(AM_IPSEP, "ipsep", "ipsep"),
    ITEM(AM_SCALE, "oscale", "old scaling"),
    ITEM(AM_SCALEXY, "scalexy", "x and y scaling"),
    ITEM(AM_ORTHO, "ortho", "orthogonal constraints"),
    ITEM(AM_ORTHO_YX, "ortho_yx", "orthogonal constraints"),
    ITEM(AM_ORTHOXY, "orthoxy", "xy orthogonal constraints"),
    ITEM(AM_ORTHOYX, "orthoyx", "yx orthogonal constraints"),
    ITEM(AM_PORTHO, "portho", "pseudo-orthogonal constraints"),
    ITEM(AM_PORTHO_YX, "portho_yx", "pseudo-orthogonal constraints"),
    ITEM(AM_PORTHOXY, "porthoxy", "xy pseudo-orthogonal constraints"),
    ITEM(AM_PORTHOYX, "porthoyx", "yx pseudo-orthogonal constraints"),
#if !((defined(HAVE_GTS) || defined(HAVE_TRIANGLE)) && defined(SFDP))
    ITEM(AM_PRISM, "prism", 0),
#endif
    {AM_NONE, 0, 0, 0}
};

    
/* setPrismValues:
 * Initialize and set prism values
 */
static void
setPrismValues (Agraph_t* g, char* s, adjust_data* dp)
{
    int v;

    if ((sscanf (s, "%d", &v) > 0) && (v >= 0))
	dp->value = v;
    else
	dp->value = 1000;
    dp->scaling = late_double(g, agfindgraphattr(g, "overlap_scaling"), -4.0, -1.e10);
}

/* getAdjustMode:
 * Convert string value to internal value of adjustment mode.
 * If s is NULL or empty, return NONE.
 */
static adjust_data *getAdjustMode(Agraph_t* g, char *s, adjust_data* dp)
{
    lookup_t *ap = adjustMode + 1;
    if ((s == NULL) || (*s == '\0')) {
	dp->mode = adjustMode[0].mode;
	dp->print = adjustMode[0].print;
    }
    else {
	while (ap->attrib) {
	    if (!strncasecmp(s, ap->attrib, ap->len)) {
		if (ap->print == NULL) {
		    agerr (AGWARN, "Overlap value \"%s\" unsupported - ignored\n", ap->attrib);
		    ap = &adjustMode[1];
		}
		dp->mode = ap->mode;
		dp->print = ap->print;
		if (ap->mode == AM_PRISM)
		    setPrismValues (g, s + ap->len, dp);
		break;
	    }
	    ap++;
	}
	if (ap->attrib == NULL ) {
	    int v = mapBool(s,'?');
	    if (v == '?') {
		agerr (AGWARN, "Unrecognized overlap value \"%s\" - using false\n", s);
		v = FALSE;
	    }
	    if (v) {
		dp->mode = adjustMode[0].mode;
		dp->print = adjustMode[0].print;
	    }
	    else {
		dp->mode = adjustMode[1].mode;
		dp->print = adjustMode[1].print;
	    }
	    if (dp->mode == AM_PRISM)
		setPrismValues (g, "", dp);
	}
    }
    return dp;
}

adjust_data *graphAdjustMode(graph_t *G, adjust_data* dp, char* dflt)
{
    char* am = agget(G, "overlap");
    return (getAdjustMode (G, am ? am : (dflt ? dflt : ""), dp));
}

/* removeOverlapWith:
 * Use adjust_data to determine if and how to remove
 * node overlaps.
 * Return non-zero if nodes are moved.
 */
int 
removeOverlapWith (graph_t * G, adjust_data* am)
{
    int ret, nret;

    if (agnnodes(G) < 2)
	return 0;

    nret = normalize (G);

    if (am->mode == AM_NONE)
	return nret;

    if (Verbose)
	fprintf(stderr, "Adjusting %s using %s\n", agnameof(G), am->print);

    if (am->mode > AM_SCALE) {
/* start_timer(); */
	switch (am->mode) {
	case AM_NSCALE:
	    ret = scAdjust(G, 1);
	    break;
	case AM_SCALEXY:
	    ret = scAdjust(G, 0);
	    break;
	case AM_PUSH:
	    /* scanAdjust (G, 1); */
	    break;
	case AM_PUSHPULL:
	    /* scanAdjust (G, 0); */
	    break;
	case AM_PORTHO_YX:
	case AM_PORTHO:
	case AM_PORTHOXY:
	case AM_PORTHOYX:
	case AM_ORTHO_YX:
	case AM_ORTHO:
	case AM_ORTHOXY:
	case AM_ORTHOYX:
	    cAdjust(G, am->mode);
	    break;
	case AM_COMPRESS:
	    ret = scAdjust(G, -1);
	    break;
#if ((defined(HAVE_GTS) || defined(HAVE_TRIANGLE)) && defined(SFDP))
	case AM_PRISM:
	    ret = fdpAdjust(G, am);
	    break;
#endif
#ifdef IPSEPCOLA
	case AM_IPSEP:
	    return nret;   /* handled during layout */
	    break;
	case AM_VPSC:
	    ret = vpscAdjust(G);
	    break;
#endif
	default:		/* to silence warnings */
	    if ((am->mode != AM_VOR) && (am->mode != AM_SCALE))
		agerr(AGWARN, "Unhandled adjust option %s\n", am->print);
	    break;
	}
/* fprintf (stderr, "%s %.4f sec\n", am->print, elapsed_sec()); */
	return nret+ret;
    }

    /* create main array */
/* start_timer(); */
    if (makeInfo(G)) {
	freeNodes();
	free(sites);
	sites = 0;
	return nret;
    }

    /* establish and verify bounding box */
    chkBoundBox(G);

    if (am->mode == AM_SCALE)
	ret = sAdjust();
    else
	ret = vAdjust();

    if (ret)
	updateGraph(G);

    freeNodes();
    free(sites);
    sites = 0;
/* fprintf (stderr, "%s %.4f sec\n", am->print, elapsed_sec()); */

    return ret+nret;
}

/* removeOverlapAs:
 * Use flag value to determine if and how to remove
 * node overlaps.
 */
int 
removeOverlapAs(graph_t * G, char* flag)
{
    adjust_data am;

    if (agnnodes(G) < 2)
	return 0;
    getAdjustMode(G, flag, &am);
    return removeOverlapWith (G, &am);
}

/* adjustNodes:
 * Remove node overlap relying on graph's overlap attribute.
 * Return non-zero if graph has changed.
 */
int adjustNodes(graph_t * G)
{
    return (removeOverlapAs(G, agget(G, "overlap")));
}

/* parseFactor:
 * Convert "sep" attribute into expand_t.
 * Input "+x,y" becomes {x,y,true}
 * Input "x,y" becomes {1 + x/sepfact,1 + y/sepfact,false}
 * Return 1 on success, 0 on failure
 */
static int
parseFactor (char* s, expand_t* pp, float sepfact)
{
    int i;
    float x, y;

    while (isspace(*s)) s++;
    if (*s == '+') {
	s++;
	pp->doAdd = 1;
    }
    else pp->doAdd = 0;

    if ((i = sscanf(s, "%f,%f", &x, &y))) {
	if (i == 1) y = x;
	if (pp->doAdd) {
	    pp->x = x/sepfact;
	    pp->y = y/sepfact;
	}
	else {
	    pp->x = 1.0 + x/sepfact;
	    pp->y = 1.0 + y/sepfact;
	}
	return 1;
    }
    else return 0;
}

/* sepFactor:
 */
expand_t
sepFactor(graph_t* g)
{
    expand_t pmargin;
    char*  marg;

    if ((marg = agget(g, "sep")) && parseFactor(marg, &pmargin, 1.0)) {
    }
    else if ((marg = agget(g, "esep")) && parseFactor(marg, &pmargin, SEPFACT)) {
    }
    else { /* default */
	pmargin.x = pmargin.y = DFLT_MARGIN;
	pmargin.doAdd = 1;
    }
    if (Verbose)
	fprintf (stderr, "Node separation: add=%d (%f,%f)\n",
	    pmargin.doAdd, pmargin.x, pmargin.y);
    return pmargin;
}

/* esepFactor:
 * This value should be smaller than the sep value used to expand
 * nodes during adjustment. If not, when the adjustment pass produces
 * a fairly tight layout, the spline code will find that some nodes
 * still overlap.
 */
expand_t
esepFactor(graph_t* g)
{
    expand_t pmargin;
    char*  marg;

    if ((marg = agget(g, "esep")) && parseFactor(marg, &pmargin, 1.0)) {
    }
    else if ((marg = agget(g, "sep")) && parseFactor(marg, &pmargin, 1.0/SEPFACT)) {
    }
    else {
	pmargin.x = pmargin.y = SEPFACT*DFLT_MARGIN;
	pmargin.doAdd = 1;
    }
    if (Verbose)
	fprintf (stderr, "Edge separation: add=%d (%f,%f)\n",
	    pmargin.doAdd, pmargin.x, pmargin.y);
    return pmargin;
}
