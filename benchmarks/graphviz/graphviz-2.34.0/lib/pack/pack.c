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


/* Module for packing disconnected graphs together.
 * Based on "Disconnected Graph Layout and the Polyomino Packing Approach", 
 * K. Freivalds et al., GD0'01, LNCS 2265, pp. 378-391.
 */

#include <math.h>
#include <assert.h>
#include "render.h"
#include "pack.h"
#include "pointset.h"
#include <assert.h>

#define strneq(a,b,n)		(!strncmp(a,b,n))

#define C 100			/* Max. avg. polyomino size */

#define MOVEPT(p) ((p).x += dx, (p).y += dy)
/* Given cell size s, GRID(x:double,s:int) returns how many cells are required by size x */
#define GRID(x,s) ((int)ceil((x)/(s)))
/* Given grid cell size s, CVAL(v:int,s:int) returns index of cell containing point v */
#define CVAL(v,s) ((v) >= 0 ? (v)/(s) : (((v)+1)/(s))-1)
/* Given grid cell size s, CELL(p:point,s:int) sets p to cell containing point p */
#define CELL(p,s) ((p).x = CVAL((p).x,s), (p).y = CVAL((p).y,(s)))

typedef struct {
    int perim;			/* half size of bounding rectangle perimeter */
    point *cells;		/* cells in covering polyomino */
    int nc;			/* no. of cells */
    int index;			/* index in original array */
} ginfo;

typedef struct {
    double width, height;
    int index;			/* index in original array */
} ainfo;

/* computeStep:
 * Compute grid step size. This is a root of the
 * quadratic equation al^2 +bl + c, where a, b and
 * c are defined below.
 */
static int computeStep(int ng, boxf* bbs, int margin)
{
    double l1, l2;
    double a, b, c, d, r;
    double W, H;		/* width and height of graph, with margin */
    int i, root;

    a = C * ng - 1;
    c = 0;
    b = 0;
    for (i = 0; i < ng; i++) {
	boxf bb = bbs[i];
	W = bb.UR.x - bb.LL.x + 2 * margin;
	H = bb.UR.y - bb.LL.y + 2 * margin;
	b -= (W + H);
	c -= (W * H);
    }
    d = b * b - 4.0 * a * c;
    if (d < 0) {
	agerr(AGERR, "libpack: disc = %f ( < 0)\n", d);
	return -1;
    }
    r = sqrt(d);
    l1 = (-b + r) / (2 * a);
    l2 = (-b - r) / (2 * a);
    root = (int) l1;
    if (root == 0) root = 1;
    if (Verbose > 2) {
	fprintf(stderr, "Packing: compute grid size\n");
	fprintf(stderr, "a %f b %f c %f d %f r %f\n", a, b, c, d, r);
	fprintf(stderr, "root %d (%f) %d (%f)\n", root, l1, (int) l2,
		l2);
	fprintf(stderr, " r1 %f r2 %f\n", a * l1 * l1 + b * l1 + c,
		a * l2 * l2 + b * l2 + c);
    }

    return root;
}

/* cmpf;
 * Comparison function for polyominoes.
 * Size is determined by perimeter.
 */
static int cmpf(const void *X, const void *Y)
{
    ginfo *x = *(ginfo **) X;
    ginfo *y = *(ginfo **) Y;
    /* flip order to get descending array */
    return (y->perim - x->perim);
}

/* fillLine:
 * Mark cells crossed by line from cell p to cell q.
 * Bresenham's algorithm, from Graphics Gems I, pp. 99-100.
 */
/* static  */
void fillLine(pointf p, pointf q, PointSet * ps)
{
    int x1 = ROUND(p.x);
    int y1 = ROUND(p.y);
    int x2 = ROUND(q.x);
    int y2 = ROUND(q.y);
    int d, x, y, ax, ay, sx, sy, dx, dy;

    dx = x2 - x1;
    ax = ABS(dx) << 1;
    sx = SGN(dx);
    dy = y2 - y1;
    ay = ABS(dy) << 1;
    sy = SGN(dy);

/* fprintf (stderr, "fillLine %d %d - %d %d\n", x1,y1,x2,y2); */
    x = x1;
    y = y1;
    if (ax > ay) {              /* x dominant */
        d = ay - (ax >> 1);
        for (;;) {
/* fprintf (stderr, "  addPS %d %d\n", x,y); */
            addPS(ps, x, y);
            if (x == x2)
                return;
            if (d >= 0) {
                y += sy;
                d -= ax;
            }
            x += sx;
            d += ay;
        }
    } else {                    /* y dominant */
        d = ax - (ay >> 1);
        for (;;) {
/* fprintf (stderr, "  addPS %d %d\n", x,y); */
            addPS(ps, x, y);
            if (y == y2)
                return;
            if (d >= 0) {
                x += sx;
                d -= ay;
            }
            y += sy;
            d += ax;
        }
    }
}

/* fillEdge:
 * It appears that spline_edges always have the start point at the
 * beginning and the end point at the end.
 */
static void
fillEdge(Agedge_t * e, point p, PointSet * ps, int dx, int dy,
	 int ssize, int doS)
{
    int j, k;
    bezier bz;
    pointf pt, hpt;
    Agnode_t *h;

    P2PF(p, pt);

    /* If doS is false or the edge has not splines, use line segment */
    if (!doS || !ED_spl(e)) {
	h = aghead(e);
	hpt = coord(h);
	MOVEPT(hpt);
	CELL(hpt, ssize);
	fillLine(pt, hpt, ps);
	return;
    }

    for (j = 0; j < ED_spl(e)->size; j++) {
	bz = ED_spl(e)->list[j];
	if (bz.sflag) {
	    pt = bz.sp;
	    hpt = bz.list[0];
	    k = 1;
	} else {
	    pt = bz.list[0];
	    hpt = bz.list[1];
	    k = 2;
	}
	MOVEPT(pt);
	CELL(pt, ssize);
	MOVEPT(hpt);
	CELL(hpt, ssize);
	fillLine(pt, hpt, ps);

	for (; k < bz.size; k++) {
	    pt = hpt;
	    hpt = bz.list[k];
	    MOVEPT(hpt);
	    CELL(hpt, ssize);
	    fillLine(pt, hpt, ps);
	}

	if (bz.eflag) {
	    pt = hpt;
	    hpt = bz.ep;
	    MOVEPT(hpt);
	    CELL(hpt, ssize);
	    fillLine(pt, hpt, ps);
	}
    }

}

/* genBox:
 * Generate polyomino info from graph using the bounding box of
 * the graph.
 */
static void
genBox(boxf bb0, ginfo * info, int ssize, int margin, point center, char* s)
{
    PointSet *ps;
    int W, H;
    point UR, LL;
    box bb;
    int x, y;

    BF2B(bb0, bb);
    ps = newPS();

    LL.x = center.x - margin;
    LL.y = center.y - margin;
    UR.x = center.x + bb.UR.x - bb.LL.x + margin;
    UR.y = center.y + bb.UR.y - bb.LL.y + margin;
    CELL(LL, ssize);
    CELL(UR, ssize);

    for (x = LL.x; x <= UR.x; x++)
	for (y = LL.y; y <= UR.y; y++)
	    addPS(ps, x, y);

    info->cells = pointsOf(ps);
    info->nc = sizeOf(ps);
    W = GRID(bb0.UR.x - bb0.LL.x + 2 * margin, ssize);
    H = GRID(bb0.UR.y - bb0.LL.y + 2 * margin, ssize);
    info->perim = W + H;

    if (Verbose > 2) {
	int i;
	fprintf(stderr, "%s no. cells %d W %d H %d\n",
		s, info->nc, W, H);
	for (i = 0; i < info->nc; i++)
	    fprintf(stderr, "  %d %d cell\n", info->cells[i].x,
		    info->cells[i].y);
    }

    freePS(ps);
}

/* genPoly:
 * Generate polyomino info from graph.
 * We add all cells covered partially by the bounding box of the 
 * node. If doSplines is true and an edge has a spline, we use the 
 * polyline determined by the control point. Otherwise,
 * we use each cell crossed by a straight edge between the head and tail.
 * If mode = l_clust, we use the graph's GD_clust array to treat the
 * top level clusters like large nodes.
 * Returns 0 if okay.
 */
static int
genPoly(Agraph_t * root, Agraph_t * g, ginfo * info,
	int ssize, pack_info * pinfo, point center)
{
    PointSet *ps;
    int W, H;
    point LL, UR;
    point pt, s2;
    pointf ptf;
    Agraph_t *eg;		/* graph containing edges */
    Agnode_t *n;
    Agedge_t *e;
    int x, y;
    int dx, dy;
    graph_t *subg;
    int margin = pinfo->margin;
    int doSplines = pinfo->doSplines;
    box bb;

    if (root)
	eg = root;
    else
	eg = g;

    ps = newPS();
    dx = center.x - ROUND(GD_bb(g).LL.x);
    dy = center.y - ROUND(GD_bb(g).LL.y);

    if (pinfo->mode == l_clust) {
	int i;
	void **alg;

	/* backup the alg data */
	alg = N_GNEW(agnnodes(g), void *);
	for (i = 0, n = agfstnode(g); n; n = agnxtnode(g, n)) {
	    alg[i++] = ND_alg(n);
	    ND_alg(n) = 0;
	}

	/* do bbox of top clusters */
	for (i = 1; i <= GD_n_cluster(g); i++) {
	    subg = GD_clust(g)[i];
	    BF2B(GD_bb(subg), bb);
	    if ((bb.UR.x > bb.LL.x) && (bb.UR.y > bb.LL.y)) {
		MOVEPT(bb.LL);
		MOVEPT(bb.UR);
		bb.LL.x -= margin;
		bb.LL.y -= margin;
		bb.UR.x += margin;
		bb.UR.y += margin;
		CELL(bb.LL, ssize);
		CELL(bb.UR, ssize);

		for (x = bb.LL.x; x <= bb.UR.x; x++)
		    for (y = bb.LL.y; y <= bb.UR.y; y++)
			addPS(ps, x, y);

		/* note which nodes are in clusters */
		for (n = agfstnode(subg); n; n = agnxtnode(subg, n))
		    ND_clust(n) = subg;
	    }
	}

	/* now do remaining nodes and edges */
	for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	    ptf = coord(n);
	    PF2P(ptf, pt);
	    MOVEPT(pt);
	    if (!ND_clust(n)) {	/* n is not in a top-level cluster */
		s2.x = margin + ND_xsize(n) / 2;
		s2.y = margin + ND_ysize(n) / 2;
		LL = sub_point(pt, s2);
		UR = add_point(pt, s2);
		CELL(LL, ssize);
		CELL(UR, ssize);

		for (x = LL.x; x <= UR.x; x++)
		    for (y = LL.y; y <= UR.y; y++)
			addPS(ps, x, y);

		CELL(pt, ssize);
		for (e = agfstout(eg, n); e; e = agnxtout(eg, e)) {
		    fillEdge(e, pt, ps, dx, dy, ssize, doSplines);
		}
	    } else {
		CELL(pt, ssize);
		for (e = agfstout(eg, n); e; e = agnxtout(eg, e)) {
		    if (ND_clust(n) == ND_clust(aghead(e)))
			continue;
		    fillEdge(e, pt, ps, dx, dy, ssize, doSplines);
		}
	    }
	}

	/* restore the alg data */
	for (i = 0, n = agfstnode(g); n; n = agnxtnode(g, n)) {
	    ND_alg(n)= alg[i++];
	}
	free(alg);

    } else
	for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	    ptf = coord(n);
	    PF2P(ptf, pt);
	    MOVEPT(pt);
	    s2.x = margin + ND_xsize(n) / 2;
	    s2.y = margin + ND_ysize(n) / 2;
	    LL = sub_point(pt, s2);
	    UR = add_point(pt, s2);
	    CELL(LL, ssize);
	    CELL(UR, ssize);

	    for (x = LL.x; x <= UR.x; x++)
		for (y = LL.y; y <= UR.y; y++)
		    addPS(ps, x, y);

	    CELL(pt, ssize);
	    for (e = agfstout(eg, n); e; e = agnxtout(eg, e)) {
		fillEdge(e, pt, ps, dx, dy, ssize, doSplines);
	    }
	}

    info->cells = pointsOf(ps);
    info->nc = sizeOf(ps);
    W = GRID(GD_bb(g).UR.x - GD_bb(g).LL.x + 2 * margin, ssize);
    H = GRID(GD_bb(g).UR.y - GD_bb(g).LL.y + 2 * margin, ssize);
    info->perim = W + H;

    if (Verbose > 2) {
	int i;
	fprintf(stderr, "%s no. cells %d W %d H %d\n",
		agnameof(g), info->nc, W, H);
	for (i = 0; i < info->nc; i++)
	    fprintf(stderr, "  %d %d cell\n", info->cells[i].x,
		    info->cells[i].y);
    }

    freePS(ps);
    return 0;
}

/* fits:
 * Check if polyomino fits at given point.
 * If so, add cells to pointset, store point in place and return true.
 */
static int
fits(int x, int y, ginfo * info, PointSet * ps, point * place, int step, boxf* bbs)
{
    point *cells = info->cells;
    int n = info->nc;
    point cell;
    int i;
    point LL;

    for (i = 0; i < n; i++) {
	cell = *cells;
	cell.x += x;
	cell.y += y;
	if (inPS(ps, cell))
	    return 0;
	cells++;
    }

    PF2P(bbs[info->index].LL, LL);
    place->x = step * x - LL.x;
    place->y = step * y - LL.y;

    cells = info->cells;
    for (i = 0; i < n; i++) {
	cell = *cells;
	cell.x += x;
	cell.y += y;
	insertPS(ps, cell);
	cells++;
    }

    if (Verbose >= 2)
	fprintf(stderr, "cc (%d cells) at (%d,%d) (%d,%d)\n", n, x, y,
		place->x, place->y);
    return 1;
}

/* placeFixed:
 * Position fixed graph. Store final translation and
 * fill polyomino set. Note that polyomino set for the
 * graph is constructed where it will be.
 */
static void
placeFixed(ginfo * info, PointSet * ps, point * place, point center)
{
    point *cells = info->cells;
    int n = info->nc;
    int i;

    place->x = -center.x;
    place->y = -center.y;

    for (i = 0; i < n; i++) {
	insertPS(ps, *cells++);
    }

    if (Verbose >= 2)
	fprintf(stderr, "cc (%d cells) at (%d,%d)\n", n, place->x,
		place->y);
}

/* placeGraph:
 * Search for points on concentric "circles" out
 * from the origin. Check if polyomino can be placed
 * with bounding box origin at point.
 * First graph (i == 0) is centered on the origin if possible.
 */
static void
placeGraph(int i, ginfo * info, PointSet * ps, point * place, int step,
	   int margin, boxf* bbs)
{
    int x, y;
    int W, H;
    int bnd;
    boxf bb = bbs[info->index];

    if (i == 0) {
	W = GRID(bb.UR.x - bb.LL.x + 2 * margin, step);
	H = GRID(bb.UR.y - bb.LL.y + 2 * margin, step);
	if (fits(-W / 2, -H / 2, info, ps, place, step, bbs))
	    return;
    }

    if (fits(0, 0, info, ps, place, step, bbs))
	return;
    W = ceil(bb.UR.x - bb.LL.x);
    H = ceil(bb.UR.y - bb.LL.y);
    if (W >= H) {
	for (bnd = 1;; bnd++) {
	    x = 0;
	    y = -bnd;
	    for (; x < bnd; x++)
		if (fits(x, y, info, ps, place, step, bbs))
		    return;
	    for (; y < bnd; y++)
		if (fits(x, y, info, ps, place, step, bbs))
		    return;
	    for (; x > -bnd; x--)
		if (fits(x, y, info, ps, place, step, bbs))
		    return;
	    for (; y > -bnd; y--)
		if (fits(x, y, info, ps, place, step, bbs))
		    return;
	    for (; x < 0; x++)
		if (fits(x, y, info, ps, place, step, bbs))
		    return;
	}
    } else {
	for (bnd = 1;; bnd++) {
	    y = 0;
	    x = -bnd;
	    for (; y > -bnd; y--)
		if (fits(x, y, info, ps, place, step, bbs))
		    return;
	    for (; x < bnd; x++)
		if (fits(x, y, info, ps, place, step, bbs))
		    return;
	    for (; y < bnd; y++)
		if (fits(x, y, info, ps, place, step, bbs))
		    return;
	    for (; x > -bnd; x--)
		if (fits(x, y, info, ps, place, step, bbs))
		    return;
	    for (; y > 0; y--)
		if (fits(x, y, info, ps, place, step, bbs))
		    return;
	}
    }
}

#ifdef DEBUG
void dumpp(ginfo * info, char *pfx)
{
    point *cells = info->cells;
    int i, c_cnt = info->nc;

    fprintf(stderr, "%s\n", pfx);
    for (i = 0; i < c_cnt; i++) {
	fprintf(stderr, "%d %d box\n", cells[i].x, cells[i].y);
    }
}
#endif

static packval_t* userVals;

/* ucmpf;
 * Sort by user values.
 */
static int ucmpf(const void *X, const void *Y)
{
    ainfo* x = *(ainfo **) X;
    ainfo* y = *(ainfo **) Y;

    int dX = userVals[x->index];
    int dY = userVals[y->index];
    if (dX > dY) return 1;
    else if (dX < dY) return -1;
    else return 0;
}

/* acmpf;
 * Sort by height + width
 */
static int acmpf(const void *X, const void *Y)
{
    ainfo* x = *(ainfo **) X;
    ainfo* y = *(ainfo **) Y;
#if 0
    if (x->height < y->height) return 1;
    else if (x->height > y->height) return -1;
    else if (x->width < y->width) return 1;
    else if (x->width > y->width) return -1;
    else return 0;
#endif
    double dX = x->height + x->width; 
    double dY = y->height + y->width; 
    if (dX < dY) return 1;
    else if (dX > dY) return -1;
    else return 0;
}

#define INC(m,c,r) \
    if (m){ c++; if (c == nc) { c = 0; r++; } } \
    else { r++; if (r == nr) { r = 0; c++; } }

/* arrayRects:
 */
static point *
arrayRects (int ng, boxf* gs, pack_info* pinfo)
{
    int i;
    int nr = 0, nc;
    int r, c;
    ainfo *info;
    ainfo *ip;
    ainfo **sinfo;
    double* widths;
    double* heights;
    double v, wd, ht;
    point* places = N_NEW(ng, point);
    boxf bb;
    int sz, rowMajor;

    /* set up no. of rows and columns */
    sz = pinfo->sz;
    if (pinfo->flags & PK_COL_MAJOR) {
	rowMajor = 0;
	if (sz > 0) {
	    nr = sz;
	    nc = (ng + (nr-1))/nr;
	}
	else {
	    nr = ceil(sqrt(ng));
	    nc = (ng + (nr-1))/nr;
	}
    }
    else {
	rowMajor = 1;
	if (sz > 0) {
	    nc = sz;
	    nr = (ng + (nc-1))/nc;
	}
	else {
	    nc = ceil(sqrt(ng));
	    nr = (ng + (nc-1))/nc;
	}
    }
    widths = N_NEW(nc+1, double);
    heights = N_NEW(nr+1, double);

    ip = info = N_NEW(ng, ainfo);
    for (i = 0; i < ng; i++, ip++) {
	bb = gs[i];
	ip->width = bb.UR.x - bb.LL.x + pinfo->margin;
	ip->height = bb.UR.y - bb.LL.y + pinfo->margin;
	ip->index = i;
    }

    sinfo = N_NEW(ng, ainfo*);
    for (i = 0; i < ng; i++) {
	sinfo[i] = info + i;
    }

    if (pinfo->vals) {
	userVals = pinfo->vals;
	qsort(sinfo, ng, sizeof(ainfo *), ucmpf);
    }
    else {
	qsort(sinfo, ng, sizeof(ainfo *), acmpf);
    }

    /* compute column widths and row heights */
    r = c = 0;
    for (i = 0; i < ng; i++, ip++) {
	ip = sinfo[i];
	widths[c] = MAX(widths[c],ip->width);
	heights[r] = MAX(heights[r],ip->height);
	INC(rowMajor,c,r);
    }

    /* convert widths and heights to positions */
    wd = 0;
    for (i = 0; i <= nc; i++) {
	v = widths[i];
	widths[i] = wd;
	wd += v;
    }

    ht = 0;
    for (i = nr; 0 < i; i--) {
	v = heights[i-1];
	heights[i] = ht;
	ht += v;
    }
    heights[0] = ht;

    /* position rects */
    r = c = 0;
    for (i = 0; i < ng; i++, ip++) {
	int idx;
	ip = sinfo[i];
	idx = ip->index;
	bb = gs[idx];
	if (pinfo->flags & PK_LEFT_ALIGN)
	    places[idx].x = widths[c];
	else if (pinfo->flags & PK_RIGHT_ALIGN)
	    places[idx].x = widths[c+1] - (bb.UR.x - bb.LL.x);
	else
	    places[idx].x = (widths[c] + widths[c+1] - bb.UR.x - bb.LL.x)/2.0;
	if (pinfo->flags & PK_TOP_ALIGN)
	    places[idx].y = heights[r] - (bb.UR.y - bb.LL.y);
	else if (pinfo->flags & PK_BOT_ALIGN)
	    places[idx].y = heights[r+1];
	else
	    places[idx].y = (heights[r] + heights[r+1] - bb.UR.y - bb.LL.y)/2.0;
	INC(rowMajor,c,r);
    }

    free (info);
    free (sinfo);
    free (widths);
    free (heights);
    return places;
}

static point*
polyRects(int ng, boxf* gs, pack_info * pinfo)
{
    int stepSize;
    ginfo *info;
    ginfo **sinfo;
    point *places;
    Dict_t *ps;
    int i;
    point center;

    /* calculate grid size */
    stepSize = computeStep(ng, gs, pinfo->margin);
    if (Verbose)
	fprintf(stderr, "step size = %d\n", stepSize);
    if (stepSize <= 0)
	return 0;

    /* generate polyomino cover for the rectangles */
    center.x = center.y = 0;
    info = N_NEW(ng, ginfo);
    for (i = 0; i < ng; i++) {
	info[i].index = i;
	genBox(gs[i], info + i, stepSize, pinfo->margin, center, "");
    }

    /* sort */
    sinfo = N_NEW(ng, ginfo *);
    for (i = 0; i < ng; i++) {
	sinfo[i] = info + i;
    }
    qsort(sinfo, ng, sizeof(ginfo *), cmpf);

    ps = newPS();
    places = N_NEW(ng, point);
    for (i = 0; i < ng; i++)
	placeGraph(i, sinfo[i], ps, places + (sinfo[i]->index),
		       stepSize, pinfo->margin, gs);

    free(sinfo);
    for (i = 0; i < ng; i++)
	free(info[i].cells);
    free(info);
    freePS(ps);

    if (Verbose > 1)
	for (i = 0; i < ng; i++)
	    fprintf(stderr, "pos[%d] %d %d\n", i, places[i].x,
		    places[i].y);

    return places;
}

/* polyGraphs:
 *  Given a collection of graphs, reposition them in the plane
 *  to not overlap but pack "nicely".
 *   ng is the number of graphs
 *   gs is a pointer to an array of graph pointers
 *   root gives the graph containing the edges; if null, the function
 *     looks in each graph in gs for its edges
 *   pinfo->margin gives the amount of extra space left around nodes in points
 *   If pinfo->doSplines is true, use edge splines, if computed,
 *     in calculating polyomino.
 *   pinfo->mode specifies the packing granularity and technique:
 *     l_node : pack at the node/cluster level
 *     l_graph : pack at the bounding box level
 *  Returns array of points to which graphs should be translated;
 *  the array needs to be freed;
 * Returns NULL if problem occur or if ng == 0.
 * 
 * Depends on graph fields bb, node fields pos, xsize and ysize, and
 * edge field spl.
 *
 * FIX: fixed mode does not always work. The fixed ones get translated
 * back to be centered on the origin.
 * FIX: Check CELL and GRID macros for negative coordinates 
 * FIX: Check width and height computation
 */
static point*
polyGraphs(int ng, Agraph_t ** gs, Agraph_t * root, pack_info * pinfo)
{
    int stepSize;
    ginfo *info;
    ginfo **sinfo;
    point *places;
    Dict_t *ps;
    int i;
    boolean *fixed = pinfo->fixed;
    int fixed_cnt = 0;
    box bb, fixed_bb = { {0, 0}, {0, 0} };
    point center;
    boxf* bbs;

    if (ng <= 0)
	return 0;

    /* update bounding box info for each graph */
    /* If fixed, compute bbox of fixed graphs */
    for (i = 0; i < ng; i++) {
	Agraph_t *g = gs[i];
	compute_bb(g);
	if (fixed && fixed[i]) {
	    BF2B(GD_bb(g), bb);
	    if (fixed_cnt) {
		fixed_bb.LL.x = MIN(bb.LL.x, fixed_bb.LL.x);
		fixed_bb.LL.y = MIN(bb.LL.y, fixed_bb.LL.y);
		fixed_bb.UR.x = MAX(bb.UR.x, fixed_bb.UR.x);
		fixed_bb.UR.y = MAX(bb.UR.y, fixed_bb.UR.y);
	    } else
		fixed_bb = bb;
	    fixed_cnt++;
	}
	if (Verbose > 2) {
	    fprintf(stderr, "bb[%s] %.5g %.5g %.5g %.5g\n", agnameof(g),
		GD_bb(g).LL.x, GD_bb(g).LL.y,
		GD_bb(g).UR.x, GD_bb(g).UR.y);
	}
    }

    /* calculate grid size */
    bbs = N_GNEW(ng, boxf);
    for (i = 0; i < ng; i++)
	bbs[i] = GD_bb(gs[i]);
    stepSize = computeStep(ng, bbs, pinfo->margin);
    if (Verbose)
	fprintf(stderr, "step size = %d\n", stepSize);
    if (stepSize <= 0)
	return 0;

    /* generate polyomino cover for the graphs */
    if (fixed) {
	center.x = (fixed_bb.LL.x + fixed_bb.UR.x) / 2;
	center.y = (fixed_bb.LL.y + fixed_bb.UR.y) / 2;
    } else
	center.x = center.y = 0;
    info = N_NEW(ng, ginfo);
    for (i = 0; i < ng; i++) {
	Agraph_t *g = gs[i];
	info[i].index = i;
	if (pinfo->mode == l_graph)
	    genBox(GD_bb(g), info + i, stepSize, pinfo->margin, center, agnameof(g));
	else if (genPoly(root, gs[i], info + i, stepSize, pinfo, center)) {
	    return 0;
	}
    }

    /* sort */
    sinfo = N_NEW(ng, ginfo *);
    for (i = 0; i < ng; i++) {
	sinfo[i] = info + i;
    }
    qsort(sinfo, ng, sizeof(ginfo *), cmpf);

    ps = newPS();
    places = N_NEW(ng, point);
    if (fixed) {
	for (i = 0; i < ng; i++) {
	    if (fixed[i])
		placeFixed(sinfo[i], ps, places + (sinfo[i]->index),
			   center);
	}
	for (i = 0; i < ng; i++) {
	    if (!fixed[i])
		placeGraph(i, sinfo[i], ps, places + (sinfo[i]->index),
			   stepSize, pinfo->margin, bbs);
	}
    } else {
	for (i = 0; i < ng; i++)
	    placeGraph(i, sinfo[i], ps, places + (sinfo[i]->index),
		       stepSize, pinfo->margin, bbs);
    }

    free(sinfo);
    for (i = 0; i < ng; i++)
	free(info[i].cells);
    free(info);
    freePS(ps);
    free (bbs);

    if (Verbose > 1)
	for (i = 0; i < ng; i++)
	    fprintf(stderr, "pos[%d] %d %d\n", i, places[i].x,
		    places[i].y);

    return places;
}

point *putGraphs(int ng, Agraph_t ** gs, Agraph_t * root,
		 pack_info * pinfo)
{
    int i, v;
    boxf* bbs;
    Agraph_t* g;
    point* pts;
    char* s;

    if (ng <= 0) return NULL;

    if (pinfo->mode <= l_graph)
	return polyGraphs (ng, gs, root, pinfo);
    
    bbs = N_GNEW(ng, boxf);

    for (i = 0; i < ng; i++) {
	g = gs[i];
	compute_bb(g);
	bbs[i] = GD_bb(g);
    }

    if (pinfo->mode == l_array) {
	if (pinfo->flags & PK_USER_VALS) {
	    pinfo->vals = N_NEW(ng, unsigned char);
	    for (i = 0; i < ng; i++) {
		s = agget (gs[i], "sortv");
		if (s && (sscanf (s, "%d", &v) > 0) && (v >= 0))
		    pinfo->vals[i] = v;
	    }

	}
	pts = arrayRects (ng, bbs, pinfo);
	if (pinfo->flags & PK_USER_VALS)
	    free (pinfo->vals);
    }

    free (bbs);

    return pts;
}

point *
putRects(int ng, boxf* bbs, pack_info* pinfo)
{
    if (ng <= 0) return NULL;
    if ((pinfo->mode == l_node) || (pinfo->mode == l_clust)) return NULL;
    if (pinfo->mode == l_graph)
	return polyRects (ng, bbs, pinfo);
    else if (pinfo->mode == l_array)
	return arrayRects (ng, bbs, pinfo);
    else
	return NULL;
}

/* packRects:
 * Packs rectangles.
 *  ng - number of rectangles
 *  bbs - array of rectangles
 *  info - parameters used in packing
 * This decides where to layout the rectangles and repositions 
 * the bounding boxes.
 *
 * Returns 0 on success.
 */
int 
packRects(int ng, boxf* bbs, pack_info* pinfo)
{
    int i;
    point *pp;
    boxf bb;
    point p;

    if (ng < 0) return -1;
    if (ng <= 1) return 0;

    pp = putRects(ng, bbs, pinfo);
    if (!pp)
	return 1;

    for (i = 0; i < ng; i++) { 
	bb = bbs[i];
	p = pp[i];
	bb.LL.x += p.x;
	bb.UR.x += p.x;
	bb.LL.y += p.y;
	bb.UR.y += p.y;
	bbs[i] = bb;
    }
    free(pp);
    return 0;
}

/* shiftEdge:
 * Translate all of the edge components by the given offset.
 */
static void shiftEdge(Agedge_t * e, int dx, int dy)
{
    int j, k;
    bezier bz;

    if (ED_label(e))
	MOVEPT(ED_label(e)->pos);
    if (ED_head_label(e))
	MOVEPT(ED_head_label(e)->pos);
    if (ED_tail_label(e))
	MOVEPT(ED_tail_label(e)->pos);

    if (ED_spl(e) == NULL)
	return;

    for (j = 0; j < ED_spl(e)->size; j++) {
	bz = ED_spl(e)->list[j];
	for (k = 0; k < bz.size; k++)
	    MOVEPT(bz.list[k]);
	if (bz.sflag)
	    MOVEPT(ED_spl(e)->list[j].sp);
	if (bz.eflag)
	    MOVEPT(ED_spl(e)->list[j].ep);
    }
}

/* shiftGraph:
 */
static void shiftGraph(Agraph_t * g, int dx, int dy)
{
    graph_t *subg;
    boxf bb = GD_bb(g);
    int i;

    bb = GD_bb(g);
    bb.LL.x += dx;
    bb.UR.x += dx;
    bb.LL.y += dy;
    bb.UR.y += dy;
    GD_bb(g) = bb;

    if (GD_label(g))
	MOVEPT(GD_label(g)->pos);

    for (i = 1; i <= GD_n_cluster(g); i++) {
	subg = GD_clust(g)[i];
	shiftGraph(subg, dx, dy);
    }
}

/* shiftGraphs:
 * The function takes ng graphs gs and a similar
 * number of points pp and translates each graph so
 * that the lower left corner of the bounding box of graph gs[i] is at
 * point ps[i]. To do this, it assumes the bb field in
 * Agraphinfo_t accurately reflects the current graph layout.
 * The graph is repositioned by translating the pos and coord fields of 
 * each node appropriately.
 * 
 * If doSplines is non-zero, the function also translates splines coordinates
 * of each edge, if they have been calculated. In addition, edge labels are
 * repositioned. 
 * 
 * If root is non-NULL, it is taken as the root graph of
 * the graphs in gs and is used to find the edges. Otherwise, the function
 * uses the edges found in each graph gs[i].
 * 
 * It returns 0 on success.
 * 
 * This function uses the bb field in Agraphinfo_t,
 * the pos and coord fields in nodehinfo_t and
 * the spl field in Aedgeinfo_t.
 */
int
shiftGraphs(int ng, Agraph_t ** gs, point * pp, Agraph_t * root,
	    int doSplines)
{
    int i;
    int dx, dy;
    double fx, fy;
    point p;
    Agraph_t *g;
    Agraph_t *eg;
    Agnode_t *n;
    Agedge_t *e;

    if (ng <= 0)
	return abs(ng);

    for (i = 0; i < ng; i++) {
	g = gs[i];
	if (root)
	    eg = root;
	else
	    eg = g;
	p = pp[i];
	dx = p.x;
	dy = p.y;
	fx = PS2INCH(dx);
	fy = PS2INCH(dy);

	for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	    ND_pos(n)[0] += fx;
	    ND_pos(n)[1] += fy;
	    MOVEPT(ND_coord(n));
	    if (doSplines) {
		for (e = agfstout(eg, n); e; e = agnxtout(eg, e))
		    shiftEdge(e, dx, dy);
	    }
	}
	shiftGraph(g, dx, dy);
    }

    return 0;
}

/* packGraphs:
 * Packs graphs.
 *  ng - number of graphs
 *  gs - pointer to array of graphs
 *  root - graph used to find edges
 *  info - parameters used in packing
 *  info->doSplines - if true, use already computed spline control points
 * This decides where to layout the graphs and repositions the graph's
 * position info.
 *
 * Returns 0 on success.
 */
int packGraphs(int ng, Agraph_t ** gs, Agraph_t * root, pack_info * info)
{
    int ret;
    point *pp = putGraphs(ng, gs, root, info);

    if (!pp)
	return 1;
    ret = shiftGraphs(ng, gs, pp, root, info->doSplines);
    free(pp);
    return ret;
}

/* packSubgraphs:
 * Packs subgraphs of given root graph, then recalculates root's bounding box.
 * Note that it does not recompute subgraph bounding boxes.
 * Cluster bounding boxes are recomputed in shiftGraphs.
 */
int
packSubgraphs(int ng, Agraph_t ** gs, Agraph_t * root, pack_info * info)
{
    int ret;

    ret = packGraphs(ng, gs, root, info);
    if (ret == 0) {
	int i, j;
	boxf bb;
	graph_t* g;

	compute_bb(root);
	bb = GD_bb(root);
	for (i = 0; i < ng; i++) {
	    g = gs[i];
	    for (j = 1; j <= GD_n_cluster(g); j++) {
		EXPANDBB(bb,GD_bb(GD_clust(g)[j]));
	    }
	}
	GD_bb(root) = bb;
    }
    return ret;
}

/* pack_graph:
 * Pack subgraphs followed by postprocessing.
 */
int 
pack_graph(int ng, Agraph_t** gs, Agraph_t* root, boolean* fixed)
{
    int ret;
    pack_info info;

    getPackInfo(root, l_graph, CL_OFFSET, &info);
    info.doSplines = 1;
    info.fixed = fixed;
    ret = packSubgraphs(ng, gs, root, &info);
    if (ret == 0) dotneato_postprocess (root);
    return ret;
}

#define ARRAY  "array"
#define ASPECT "aspect"
#define SLEN(s) (sizeof(s)/sizeof(char) - 1)

static char*
chkFlags (char* p, pack_info* pinfo)
{
    int c, more;

    if (*p != '_') return p;
    p++;
    more = 1;
    while (more && (c = *p)) {
	switch (c) {
	case 'c' :
	    pinfo->flags |= PK_COL_MAJOR;
	    p++;
	    break;
	case 'u' :
	    pinfo->flags |= PK_USER_VALS;
	    p++;
	    break;
	case 't' :
	    pinfo->flags |= PK_TOP_ALIGN;
	    p++;
	    break;
	case 'b' :
	    pinfo->flags |= PK_BOT_ALIGN;
	    p++;
	    break;
	case 'l' :
	    pinfo->flags |= PK_LEFT_ALIGN;
	    p++;
	    break;
	case 'r' :
	    pinfo->flags |= PK_RIGHT_ALIGN;
	    p++;
	    break;
	default :
	    more = 0;
	    break;
	}
    }
    return p;
}

/* parsePackModeInfo;
 * Return pack_mode of graph using "packmode" attribute.
 * If not defined, return dflt
 */
pack_mode 
parsePackModeInfo(char* p, pack_mode dflt, pack_info* pinfo)
{
    float v;
    int i;

    assert (pinfo);
    pinfo->flags = 0;
    pinfo->mode = dflt;
    pinfo->sz = 0;
    pinfo->vals = NULL;
    if (p && *p) {
	switch (*p) {
	case 'a':
	    if (strneq(p, ARRAY, SLEN(ARRAY))) {
		pinfo->mode = l_array;
		p += SLEN(ARRAY);
		p = chkFlags (p, pinfo);
		if ((sscanf (p, "%d", &i)>0) && (i > 0))
		    pinfo->sz = i;
	    }
	    else if (strneq(p, ASPECT, SLEN(ASPECT))) {
		pinfo->mode = l_aspect;
		if ((sscanf (p + SLEN(ARRAY), "%f", &v)>0) && (v > 0))
		    pinfo->aspect = v;
		else
		    pinfo->aspect = 1;
	    }
	    break;
#ifdef NOT_IMPLEMENTED
	case 'b':
	    if (streq(p, "bisect"))
		pinfo->mode = l_bisect;
	    break;
#endif
	case 'c':
	    if (streq(p, "cluster"))
		pinfo->mode = l_clust;
	    break;
	case 'g':
	    if (streq(p, "graph"))
		pinfo->mode = l_graph;
	    break;
#ifdef NOT_IMPLEMENTED
	case 'h':
	    if (streq(p, "hull"))
		pinfo->mode = l_hull;
	    break;
#endif
	case 'n':
	    if (streq(p, "node"))
		pinfo->mode = l_node;
	    break;
#ifdef NOT_IMPLEMENTED
	case 't':
	    if (streq(p, "tile"))
		pinfo->mode = l_tile;
	    break;
#endif
	}
    }

    if (Verbose) {
	fprintf (stderr, "pack info:\n");
	fprintf (stderr, "  mode   %d\n", pinfo->mode);
	if (pinfo->mode == l_aspect)
	    fprintf (stderr, "  aspect %f\n", pinfo->aspect);
	fprintf (stderr, "  size   %d\n", pinfo->sz);
	fprintf (stderr, "  flags  %d\n", pinfo->flags);
    }
    return pinfo->mode;
}

/* getPackModeInfo;
 * Return pack_mode of graph using "packmode" attribute.
 * If not defined, return dflt
 */
pack_mode 
getPackModeInfo(Agraph_t * g, pack_mode dflt, pack_info* pinfo)
{
    return parsePackModeInfo (agget(g, "packmode"), dflt, pinfo);
}

pack_mode 
getPackMode(Agraph_t * g, pack_mode dflt)
{
  pack_info info;
  return getPackModeInfo (g, dflt, &info);
}

/* getPack:
 * Return "pack" attribute of g.
 * If not defined or negative, return not_def.
 * If defined but not specified, return dflt.
 */
int getPack(Agraph_t * g, int not_def, int dflt)
{
    char *p;
    int i;
    int v = not_def;

    if ((p = agget(g, "pack"))) {
	if ((sscanf(p, "%d", &i) == 1) && (i >= 0))
	    v = i;
	else if ((*p == 't') || (*p == 'T'))
	    v = dflt;
    }

    return v;
}

pack_mode 
getPackInfo(Agraph_t * g, pack_mode dflt, int dfltMargin, pack_info* pinfo)
{
    assert (pinfo);

    pinfo->margin = getPack(g, dfltMargin, dfltMargin);
    if (Verbose) {
	fprintf (stderr, "  margin %d\n", pinfo->margin);
    }
    pinfo->doSplines = 0;
    pinfo->fixed = 0;
    getPackModeInfo(g, dflt, pinfo);

    return pinfo->mode;
}


