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

#include <multispline.h>
#include <delaunay.h>
#include <neatoprocs.h>
#include <math.h>


static boolean spline_merge(node_t * n)
{
    return FALSE;
}

static boolean swap_ends_p(edge_t * e)
{
    return FALSE;
}

static splineInfo sinfo = { swap_ends_p, spline_merge };

typedef struct {
    int i, j;
} ipair;

typedef struct _tri {
    ipair v;
    struct _tri *nxttri;
} tri;

typedef struct {
    Ppoly_t poly;       /* base polygon used for routing an edge */
    tri **triMap;	/* triMap[j] is list of all opposite sides of 
			   triangles containing vertex j, represented
                           as the indices of the two points in poly */
} tripoly_t;

/*
 * Support for map from I x I -> I
 */
typedef struct {
    Dtlink_t link;		/* cdt data */
    int a[2];			/* key */
    int t;
} item;

static int cmpItem(Dt_t * d, int p1[], int p2[], Dtdisc_t * disc)
{
    NOTUSED(d);
    NOTUSED(disc);

    if (p1[0] < p2[0])
	return -1;
    else if (p1[0] > p2[0])
	return 1;
    else if (p1[1] < p2[1])
	return -1;
    else if (p1[1] > p2[1])
	return 1;
    else
	return 0;
}

/* newItem:
 */
static void *newItem(Dt_t * d, item * objp, Dtdisc_t * disc)
{
    item *newp = NEW(item);

    NOTUSED(disc);
    newp->a[0] = objp->a[0];
    newp->a[1] = objp->a[1];
    newp->t = objp->t;

    return newp;
}

static void freeItem(Dt_t * d, item * obj, Dtdisc_t * disc)
{
    free(obj);
}

static Dtdisc_t itemdisc = {
    offsetof(item, a),
    2 * sizeof(int),
    offsetof(item, link),
    (Dtmake_f) newItem,
    (Dtfree_f) freeItem,
    (Dtcompar_f) cmpItem,
    NIL(Dthash_f),
    NIL(Dtmemory_f),
    NIL(Dtevent_f)
};

static void addMap(Dt_t * map, int a, int b, int t)
{
    item it;
    int tmp;
    if (a > b) {
	tmp = a;
	a = b;
	b = tmp;
    }
    it.a[0] = a;
    it.a[1] = b;
    it.t = t;
    dtinsert(map, &it);
}

/* mapSegToTri:
 * Create mapping from indices of side endpoints to triangle id 
 * We use a set rather than a bag because the segments used for lookup
 * will always be a side of a polygon and hence have a unique triangle.
 */
static Dt_t *mapSegToTri(surface_t * sf)
{
    Dt_t *map = dtopen(&itemdisc, Dtoset);
    int i, a, b, c;
    int *ps = sf->faces;
    for (i = 0; i < sf->nfaces; i++) {
	a = *ps++;
	b = *ps++;
	c = *ps++;
	addMap(map, a, b, i);
	addMap(map, b, c, i);
	addMap(map, c, a, i);
    }
    return map;
}

static int findMap(Dt_t * map, int a, int b)
{
    item it;
    item *ip;
    if (a > b) {
	int tmp = a;
	a = b;
	b = tmp;
    }
    it.a[0] = a;
    it.a[1] = b;
    ip = (item *) dtsearch(map, &it);
    assert(ip);
    return ip->t;
}

/*
 * Support for map from I -> I
 */

typedef struct {
    Dtlink_t link;		/* cdt data */
    int i;			/* key */
    int j;
} Ipair;

static int cmpIpair(Dt_t * d, int *p1, int *p2, Dtdisc_t * disc)
{
    NOTUSED(d);
    NOTUSED(disc);

    return (*p1 - *p2);
}

static void *newIpair(Dt_t * d, Ipair * objp, Dtdisc_t * disc)
{
    Ipair *newp = NEW(Ipair);

    NOTUSED(disc);
    newp->i = objp->i;
    newp->j = objp->j;

    return newp;
}

static void freeIpair(Dt_t * d, Ipair * obj, Dtdisc_t * disc)
{
    free(obj);
}

static Dtdisc_t ipairdisc = {
    offsetof(Ipair, i),
    sizeof(int),
    offsetof(Ipair, link),
    (Dtmake_f) newIpair,
    (Dtfree_f) freeIpair,
    (Dtcompar_f) cmpIpair,
    NIL(Dthash_f),
    NIL(Dtmemory_f),
    NIL(Dtevent_f)
};

static void vmapAdd(Dt_t * map, int i, int j)
{
    Ipair obj;
    obj.i = i;
    obj.j = j;
    dtinsert(map, &obj);
}

static int vMap(Dt_t * map, int i)
{
    Ipair *ip;
    ip = (Ipair *) dtmatch(map, &i);
    return ip->j;
}

/* mapTri:
 * Map vertex indices from router_t to tripoly_t coordinates.
 */
static void mapTri(Dt_t * map, tri * tp)
{
    for (; tp; tp = tp->nxttri) {
	tp->v.i = vMap(map, tp->v.i);
	tp->v.j = vMap(map, tp->v.j);
    }
}

/* addTri:
 */
static tri *
addTri(int i, int j, tri * oldp)
{
    tri *tp = NEW(tri);
    tp->v.i = i;
    tp->v.j = j;
    tp->nxttri = oldp;
    return tp;
}

/* bisect:
 * Return the angle bisecting the angle pp--cp--np
 */
static double bisect(pointf pp, pointf cp, pointf np)
{
    double theta, phi;
    theta = atan2(np.y - cp.y, np.x - cp.x);
    phi = atan2(pp.y - cp.y, pp.x - cp.x);
    return (theta + phi) / 2.0;
}

/* raySeg:
 * Check if ray v->w intersects segment a--b.
 */
static int raySeg(pointf v, pointf w, pointf a, pointf b)
{
    int wa = wind(v, w, a);
    int wb = wind(v, w, b);
    if (wa == wb)
	return 0;
    if (wa == 0) {
	return (wind(v, b, w) * wind(v, b, a) >= 0);
    } else {
	return (wind(v, a, w) * wind(v, a, b) >= 0);
    }
}

/* raySegIntersect:
 * Find the point p where ray v->w intersects segment ai-bi, if any.
 * Return 1 on success, 0 on failure
 */
static int
raySegIntersect(pointf v, pointf w, pointf a, pointf b, pointf * p)
{
    if (raySeg(v, w, a, b))
	return line_intersect(v, w, a, b, p);
    else
	return 0;
}

#ifdef DEVDBG
#include <psdbg.c>
#endif

/* triPoint:
 * Given the triangle vertex v, and point w so that v->w points
 * into the polygon, return where the ray v->w intersects the
 * polygon. The search uses all of the opposite sides of triangles
 * with v as vertex.
 * Return 0 on success; 1 on failure.
 */
static int
triPoint(tripoly_t * trip, int vx, pointf v, pointf w, pointf * ip)
{
    tri *tp;

    for (tp = trip->triMap[vx]; tp; tp = tp->nxttri) {
	if (raySegIntersect
	    (v, w, trip->poly.ps[tp->v.i], trip->poly.ps[tp->v.j], ip))
	    return 0;
    }
#ifdef DEVDBG
    psInit();
    psComment ("Failure in triPoint");
    psColor("0 0 1");
    psSeg (v, w);
    for (tp = trip->triMap[vx]; tp; tp = tp->nxttri) {
	psTri(v, trip->poly.ps[tp->v.i], trip->poly.ps[tp->v.j]);
    }
    psOut(stderr);
#endif
    return 1;
}

/* ctrlPtIdx:
 * Find the index of v in the points polys->ps.
 * We start at 1 since the point corresponding to 0 
 * will never be used as v.
 */
static int ctrlPtIdx(pointf v, Ppoly_t * polys)
{
    pointf w;
    int i;
    for (i = 1; i < polys->pn; i++) {
	w = polys->ps[i];
	if ((w.x == v.x) && (w.y == v.y))
	    return i;
    }
    return -1;
}

#define SEP 15

/* mkCtrlPts:
 * Generate mult points associated with v.
 * The points will lie on the ray bisecting the angle prev--v--nxt.
 * The first point will aways be v.
 * The rest are positioned equally spaced with maximum spacing SEP.
 * In addition, they all lie within the polygon trip->poly.
 * Parameter s gives the index after which a vertex lies on the
 * opposite side. This is necessary to get the "curvature" of the
 * path correct.
 */
static pointf *mkCtrlPts(int s, int mult, pointf prev, pointf v,
			   pointf nxt, tripoly_t * trip)
{
    pointf *ps;
    int idx = ctrlPtIdx(v, &(trip->poly));
    int i;
    double d, sep, theta, sinTheta, cosTheta;
    pointf q, w;

    if (idx < 0)
	return NULL;

    ps = N_GNEW(mult, pointf);
    theta = bisect(prev, v, nxt);
    sinTheta = sin(theta);
    cosTheta = cos(theta);
    w.x = v.x + 100 * cosTheta;
    w.y = v.y + 100 * sinTheta;
    if (idx > s) {
	if (wind(prev, v, w) != 1) {
	    sinTheta *= -1;
	    cosTheta *= -1;
	    w.x = v.x + 100 * cosTheta;
	    w.y = v.y + 100 * sinTheta;
	}
    } else if (wind(prev, v, w) != -1) {
	sinTheta *= -1;
	cosTheta *= -1;
	w.x = v.x + 100 * cosTheta;
	w.y = v.y + 100 * sinTheta;
    }

    if (triPoint(trip, idx, v, w, &q)) {
	return 0;
    }

    d = DIST(q, v);
    if (d >= mult * SEP)
	sep = SEP;
    else
	sep = d / mult;
    if (idx < s) {
	for (i = 0; i < mult; i++) {
	    ps[i].x = v.x + i * sep * cosTheta;
	    ps[i].y = v.y + i * sep * sinTheta;
	}
    } else {
	for (i = 0; i < mult; i++) {
	    ps[mult - i - 1].x = v.x + i * sep * cosTheta;
	    ps[mult - i - 1].y = v.y + i * sep * sinTheta;
	}
    }
    return ps;
}

/*
 * Simple graph structure for recording the triangle graph.
 */

typedef struct {
    int ne;         /* no. of edges. */
    int *edges;     /* indices of edges adjacent to node. */
    pointf ctr;     /* center of triangle. */
} tnode;

typedef struct {
    int t, h;       /* indices of head and tail nodes */
    ipair seg;      /* indices of points forming shared segment */
    double dist;    /* length of edge; usually distance between centers */
} tedge;

typedef struct {
    tnode *nodes;
    tedge *edges;
    int nedges;    /* no. of edges; no. of nodes is stored in router_t */
} tgraph;

struct router_s {
    int pn;     /* no. of points */
    pointf *ps;	/* all points in configuration */
    int *obs;	/* indices in obstacle i are obs[i]...obs[i+1]-1 */
    int *tris;	/* indices of triangle i are tris[3*i]...tris[3*i+2] */
    Dt_t *trimap; /* map from obstacle side (a,b) to index of adj. triangle */
    int tn;	  /* no. of nodes in tg */
    tgraph *tg;	  /* graph of triangles */
};

/* triCenter:
 * Given an array of points and 3 integer indices,
 * compute and return the center of the triangle.
 */
static pointf triCenter(pointf * pts, int *idxs)
{
    pointf a = pts[*idxs++];
    pointf b = pts[*idxs++];
    pointf c = pts[*idxs++];
    pointf p;
    p.x = (a.x + b.x + c.x) / 3.0;
    p.y = (a.y + b.y + c.y) / 3.0;
    return p;
}

#define MARGIN 32

/* bbox:
 * Compute bounding box of polygons, and return it
 * with an added margin of MARGIN.
 * Store total number of points in *np.
 */
static boxf bbox(Ppoly_t** obsp, int npoly, int *np)
{
    boxf bb;
    int i, j, cnt = 0;
    pointf p;
    Ppoly_t* obs;

    bb.LL.x = bb.LL.y = MAXDOUBLE;
    bb.UR.x = bb.UR.y = -MAXDOUBLE;

    for (i = 0; i < npoly; i++) {
	obs = *obsp++;
	for (j = 0; j < obs->pn; j++) {
	    p = obs->ps[j];
	    if (p.x < bb.LL.x)
		bb.LL.x = p.x;
	    if (p.x > bb.UR.x)
		bb.UR.x = p.x;
	    if (p.y < bb.LL.y)
		bb.LL.y = p.y;
	    if (p.y > bb.UR.y)
		bb.UR.y = p.y;
	    cnt++;
	}
    }

    *np = cnt;

    bb.LL.x -= MARGIN;
    bb.LL.y -= MARGIN;
    bb.UR.x += MARGIN;
    bb.UR.y += MARGIN;

    return bb;
}

static int *mkTriIndices(surface_t * sf)
{
    int *tris = N_GNEW(3 * sf->nfaces, int);
    memcpy(tris, sf->faces, 3 * sf->nfaces * sizeof(int));
    return tris;
}

/* degT:
 * Given a pointer to an array of 3 integers, return
 * the number not equal to -1
 */
static int degT(int *ip)
{
    ip++;
    if (*ip++ == -1)
	return 1;
    if (*ip == -1)
	return 2;
    else
	return 3;
}

/* sharedEdge:
 * Returns a pair of integer (x,y), x < y, where x and y are the
 * indices of the two vertices of the shared edge.
 */
static ipair sharedEdge(int *p, int *q)
{
    ipair pt;
    int tmp, p1, p2;
    p1 = *p;
    p2 = *(p + 1);
    if (p1 == *q) {
	if ((p2 != *(q + 1)) && (p2 != *(q + 2))) {
	    p2 = *(p + 2);
	}
    } else if (p1 == *(q + 1)) {
	if ((p2 != *q) && (p2 != *(q + 2))) {
	    p2 = *(p + 2);
	}
    } else if (p1 == *(q + 2)) {
	if ((p2 != *q) && (p2 != *(q + 1))) {
	    p2 = *(p + 2);
	}
    } else {
	p1 = *(p + 2);
    }

    if (p1 > p2) {
	tmp = p1;
	p1 = p2;
	p2 = tmp;
    }
    pt.i = p1;
    pt.j = p2;
    return pt;
}

/* addTriEdge:
 * Add an edge to g, with tail t, head h, distance d, and shared
 * segment seg.
 */
static void addTriEdge(tgraph * g, int t, int h, double d, ipair seg)
{
    tedge *ep = g->edges + g->nedges;
    tnode *tp = g->nodes + t;
    tnode *hp = g->nodes + h;

    ep->t = t;
    ep->h = h;
    ep->dist = DIST(tp->ctr, hp->ctr);
    ep->seg = seg;

    tp->edges[tp->ne++] = g->nedges;
    hp->edges[hp->ne++] = g->nedges;

    g->nedges++;
}

static void freeTriGraph(tgraph * tg)
{
    free(tg->nodes->edges);
    free(tg->nodes);
    free(tg->edges);
    free(tg);
}

/* mkTriGraph:
 * Generate graph with triangles as nodes and an edge iff two triangles
 * share an edge.
 */
static tgraph *mkTriGraph(surface_t * sf, int maxv, pointf * pts)
{
    tgraph *g;
    tnode *np;
    int j, i, ne = 0;
    int *edgei;
    int *jp;

    /* ne is twice no. of edges */
    for (i = 0; i < 3 * sf->nfaces; i++)
	if (sf->neigh[i] != -1)
	    ne++;

    g = GNEW(tgraph);

    /* plus 2 for nodes added as endpoints of an edge */
    g->nodes = N_GNEW(sf->nfaces + 2, tnode);

    /* allow 1 possible extra edge per triangle, plus 
     * obstacles can have at most maxv triangles touching 
     */
    edgei = N_GNEW(sf->nfaces + ne + 2 * maxv, int);
    g->edges = N_GNEW(ne/2 + 2 * maxv, tedge);
    g->nedges = 0;

    for (i = 0; i < sf->nfaces; i++) {
	np = g->nodes + i;
	np->ne = 0;
	np->edges = edgei;
	np->ctr = triCenter(pts, sf->faces + 3 * i);
	edgei += degT(sf->neigh + 3 * i) + 1;
    }
    /* initialize variable nodes */
    np = g->nodes + i;
    np->ne = 0;
    np->edges = edgei;
    np++;
    np->ne = 0;
    np->edges = edgei + maxv;

    for (i = 0; i < sf->nfaces; i++) {
	np = g->nodes + i;
	jp = sf->neigh + 3 * i;
        ne = 0;
	while ((ne < 3) && ((j = *jp++) != -1)) {
	    if (i < j) {
		double dist = DIST(np->ctr, (g->nodes + j)->ctr);
		ipair seg =
		    sharedEdge(sf->faces + 3 * i, sf->faces + 3 * j);
		addTriEdge(g, i, j, dist, seg);
	    }
	    ne++;
	}
    }

    return g;
}

void freeRouter(router_t * rtr)
{
    free(rtr->ps);
    free(rtr->obs);
    free(rtr->tris);
    dtclose(rtr->trimap);
    freeTriGraph(rtr->tg);
    free(rtr);
}

#ifdef DEVDBG
static void
prTriPoly (tripoly_t *poly, int si, int ei)
{
    FILE* fp = fopen ("dumppoly","w");
    
    psInit();
    psPoly (&(poly->poly));
    psPoint (poly->poly.ps[si]);
    psPoint (poly->poly.ps[ei]);
    psOut(fp);
    fclose(fp);
}

static void
prTriGraph (router_t* rtr, int n)
{
    FILE* fp = fopen ("dump","w");
    int i;
    pointf* pts = rtr->ps;
    tnode* nodes = rtr->tg->nodes;
    char buf[BUFSIZ];
    
    psInit();
    for (i=0;i < rtr->tn; i++) {
        pointf a = pts[rtr->tris[3*i]];
        pointf b = pts[rtr->tris[3*i+1]];
        pointf c = pts[rtr->tris[3*i+2]];
	psTri (a, b,c);
	sprintf (buf, "%d", i);
        psTxt (buf, nodes[i].ctr);
    }
    for (i=rtr->tn;i < n; i++) {
	sprintf (buf, "%d", i);
        psTxt (buf, nodes[i].ctr);
    }
    psColor ("1 0 0");
    for (i=0;i < rtr->tg->nedges; i++) {
        tedge* e = rtr->tg->edges+i;
        psSeg (nodes[e->t].ctr, nodes[e->h].ctr);
    }
    psOut(fp);
    fclose(fp);
}
#endif

router_t *mkRouter(Ppoly_t** obsp, int npoly)
{
    router_t *rtr = NEW(router_t);
    Ppoly_t* obs;
    boxf bb;
    pointf *pts;
    int npts;
    surface_t *sf;
    int *segs;
    double *x;
    double *y;
    int maxv = 4; /* default max. no. of vertices in an obstacle; set below */
    /* points in obstacle i have indices obsi[i] through obsi[i+1]-1 in pts
     */
    int *obsi = N_NEW(npoly + 1, int);
    int i, j, ix = 4, six = 0;

    bb = bbox(obsp, npoly, &npts);
    npts += 4;			/* 4 points of bounding box */
    pts = N_GNEW(npts, pointf);	/* all points are stored in pts */
    segs = N_GNEW(2 * npts, int);	/* indices of points forming segments */

    /* store bounding box in CCW order */
    pts[0] = bb.LL;
    pts[1].x = bb.UR.x;
    pts[1].y = bb.LL.y;
    pts[2] = bb.UR;
    pts[3].x = bb.LL.x;
    pts[3].y = bb.UR.y;
    for (i = 1; i <= 4; i++) {
	segs[six++] = i - 1;
	if (i < 4)
	    segs[six++] = i;
	else
	    segs[six++] = 0;
    }

    /* store obstacles in CW order and generate constraint segments */
    for (i = 0; i < npoly; i++) {
	obsi[i] = ix;
        obs = *obsp++;
	for (j = 1; j <= obs->pn; j++) {
	    segs[six++] = ix;
	    if (j < obs->pn)
		segs[six++] = ix + 1;
	    else
		segs[six++] = obsi[i];
	    pts[ix++] = obs->ps[j - 1];
	}
	if (obs->pn > maxv)
	    maxv = obs->pn;
    }
    obsi[i] = ix;

    /* copy points into coordinate arrays */
    x = N_GNEW(npts, double);
    y = N_GNEW(npts, double);
    for (i = 0; i < npts; i++) {
	x[i] = pts[i].x;
	y[i] = pts[i].y;
    }
    sf = mkSurface(x, y, npts, segs, npts);
    free(x);
    free(y);
    free(segs);

    rtr->ps = pts;
    rtr->pn = npts;
    rtr->obs = obsi;
    rtr->tris = mkTriIndices(sf);
    rtr->trimap = mapSegToTri(sf);
    rtr->tn = sf->nfaces;
    rtr->tg = mkTriGraph(sf, maxv, pts);

    freeSurface(sf);
    return rtr;
}

/* finishEdge:
 * Finish edge generation, clipping to nodes and adding arrowhead
 * if necessary, and adding edge labels
 */
static void
finishEdge (graph_t* g, edge_t* e, Ppoly_t spl, int flip, pointf p, pointf q)
{
    int j;
    pointf *spline = N_GNEW(spl.pn, pointf);
    pointf p1, q1;

    if (flip) {
	for (j = 0; j < spl.pn; j++) {
	    spline[spl.pn - 1 - j] = spl.ps[j];
	}
	p1 = q;
	q1 = p;
    }
    else {
	for (j = 0; j < spl.pn; j++) {
	    spline[j] = spl.ps[j];
	}
	p1 = p;
	q1 = q;
    }
    if (Verbose > 1)
	fprintf(stderr, "spline %s %s\n", agnameof(agtail(e)), agnameof(aghead(e)));
    clip_and_install(e, aghead(e), spline, spl.pn, &sinfo);
    free(spline);

    addEdgeLabels(g, e, p1, q1);
}

#define EQPT(p,q) (((p).x==(q).x)&&((p).y==(q).y))

/* tweakEnd:
 * Hack because path routing doesn't know about the interiors
 * of polygons. If the first or last segment of the shortest path
 * lies along one of the polygon boundaries, the path may flip
 * inside the polygon. To avoid this, we shift the point a bit.
 *
 * If the edge p(=poly.ps[s])-q of the shortest path is also an
 * edge of the border polygon, move p slightly inside the polygon
 * and return it. If prv and nxt are the two vertices adjacent to
 * p in the polygon, let m be the midpoint of prv--nxt. We then
 * move a tiny bit along the ray p->m.
 *
 * Otherwise, return p unchanged.
 */
static Ppoint_t
tweakEnd (Ppoly_t poly, int s, Ppolyline_t pl, Ppoint_t q)
{
    Ppoint_t prv, nxt, p;

    p = poly.ps[s];
    nxt = poly.ps[(s + 1) % poly.pn];
    if (s == 0)
	prv = poly.ps[poly.pn-1];
    else
	prv = poly.ps[s - 1];
    if (EQPT(q, nxt) || EQPT(q, prv) ){
	Ppoint_t m;
	double d;
	m.x = (nxt.x + prv.x)/2.0 - p.x;
	m.y = (nxt.y + prv.y)/2.0 - p.y;
	d = LEN(m.x,m.y);
	p.x += 0.1*m.x/d;
	p.y += 0.1*m.y/d;
    }
    return p;
}

static void
tweakPath (Ppoly_t poly, int s, int t, Ppolyline_t pl)
{
    pl.ps[0] = tweakEnd (poly, s, pl, pl.ps[1]);
    pl.ps[pl.pn-1] = tweakEnd (poly, t, pl, pl.ps[pl.pn-2]);
}


/* genroute:
 * Generate splines for e and cohorts.
 * Edges go from s to t.
 * Return 0 on success.
 */
static int 
genroute(graph_t* g, tripoly_t * trip, int s, int t, edge_t * e, int doPolyline)
{
    pointf eps[2];
    Pvector_t evs[2];
    pointf **cpts = NULL;		/* lists of control points */
    Ppoly_t poly;
    Ppolyline_t pl, spl;
    int i, j;
    Ppolyline_t mmpl;
    Pedge_t *medges = N_GNEW(trip->poly.pn, Pedge_t);
    int pn;
    int mult = ED_count(e);
    node_t* head = aghead(e);
    int rv = 0;

    poly.ps = NULL;
    pl.pn = 0;
    eps[0].x = trip->poly.ps[s].x, eps[0].y = trip->poly.ps[s].y;
    eps[1].x = trip->poly.ps[t].x, eps[1].y = trip->poly.ps[t].y;
    if (Pshortestpath(&(trip->poly), eps, &pl) < 0) {
	agerr(AGWARN, "Could not create control points for multiple spline for edge (%s,%s)\n", agnameof(agtail(e)), agnameof(aghead(e)));
	rv = 1;
	goto finish;
    }

    if (pl.pn == 2) {
	makeStraightEdge(agraphof(head), e, doPolyline, &sinfo);
	goto finish;
    }

    evs[0].x = evs[0].y = 0;
    evs[1].x = evs[1].y = 0;

    if ((mult == 1) || Concentrate) {
	poly = trip->poly;
	for (j = 0; j < poly.pn; j++) {
	    medges[j].a = poly.ps[j];
	    medges[j].b = poly.ps[(j + 1) % poly.pn];
	}
	tweakPath (poly, s, t, pl);
	if (Proutespline(medges, poly.pn, pl, evs, &spl) < 0) {
	    agerr(AGWARN, "Could not create control points for multiple spline for edge (%s,%s)\n", agnameof(agtail(e)), agnameof(aghead(e)));
	    rv = 1;
	    goto finish;
	}
	finishEdge (g, e, spl, aghead(e) != head, eps[0], eps[1]);
	free(medges);

	return 0;
    }
    
    pn = 2 * (pl.pn - 1);

    cpts = N_NEW(pl.pn - 2, pointf *);
    for (i = 0; i < pl.pn - 2; i++) {
	cpts[i] =
	    mkCtrlPts(t, mult+1, pl.ps[i], pl.ps[i + 1], pl.ps[i + 2], trip);
	if (!cpts[i]) {
	    agerr(AGWARN, "Could not create control points for multiple spline for edge (%s,%s)\n", agnameof(agtail(e)), agnameof(aghead(e)));
	    rv = 1;
	    goto finish;
	}
    }

    poly.ps = N_GNEW(pn, pointf);
    poly.pn = pn;

    for (i = 0; i < mult; i++) {
	poly.ps[0] = eps[0];
	for (j = 1; j < pl.pn - 1; j++) {
	    poly.ps[j] = cpts[j - 1][i];
	}
	poly.ps[pl.pn - 1] = eps[1];
	for (j = 1; j < pl.pn - 1; j++) {
	    poly.ps[pn - j] = cpts[j - 1][i + 1];
	}
	if (Pshortestpath(&poly, eps, &mmpl) < 0) {
	    agerr(AGWARN, "Could not create control points for multiple spline for edge (%s,%s)\n", agnameof(agtail(e)), agnameof(aghead(e)));
	    rv = 1;
	    goto finish;
	}

	if (doPolyline) {
	    make_polyline (mmpl, &spl);
	}
	else {
	    for (j = 0; j < poly.pn; j++) {
		medges[j].a = poly.ps[j];
		medges[j].b = poly.ps[(j + 1) % poly.pn];
	    }
	    tweakPath (poly, 0, pl.pn-1, mmpl);
	    if (Proutespline(medges, poly.pn, mmpl, evs, &spl) < 0) {
		agerr(AGWARN, "Could not create control points for multiple spline for edge (%s,%s)\n", 
		    agnameof(agtail(e)), agnameof(aghead(e)));
		rv = 1;
		goto finish;
	    }
	}
	finishEdge (g, e, spl, aghead(e) != head, eps[0], eps[1]);

	e = ED_to_virt(e);
    }

finish :
    if (cpts) {
	for (i = 0; i < pl.pn - 2; i++)
	    free(cpts[i]);
	free(cpts);
    }
    free(medges);
    free(poly.ps);
    return rv;
}

#define NSMALL -0.0000000001

/* inCone:
 * Returns true iff q is in the convex cone a-b-c
 */
static int
inCone (pointf a, pointf b, pointf c, pointf q)
{
    return ((area2(q,a,b) >= NSMALL) && (area2(q,b,c) >= NSMALL));
}

static pointf north = {0, 1};
static pointf northeast = {1, 1};
static pointf east = {1, 0};
static pointf southeast = {1, -1};
static pointf south = {0, -1};
static pointf southwest = {-1, -1};
static pointf west = {-1, 0};
static pointf northwest = {-1, 1};

/* addEndpoint:
 * Add node to graph representing spline end point p inside obstruction obs_id.
 * For each side of obstruction, add edge from p to corresponding triangle.
 * The node id of the new node in the graph is v_id.
 * If p lies on the side of its node (sides != 0), we limit the triangles
 * to those within 45 degrees of each side of the natural direction of p.
 */
static void addEndpoint(router_t * rtr, pointf p, node_t* v, int v_id, int sides)
{
    int obs_id = ND_lim(v);
    int starti = rtr->obs[obs_id];
    int endi = rtr->obs[obs_id + 1];
    pointf* pts = rtr->ps;
    int i, t;
    double d;
    pointf vr, v0, v1;

    switch (sides) {
    case TOP :
	vr = add_pointf (p, north);
	v0 = add_pointf (p, northwest);
	v1 = add_pointf (p, northeast);
	break;
    case TOP|RIGHT :
	vr = add_pointf (p, northeast);
	v0 = add_pointf (p, north);
	v1 = add_pointf (p, east);
	break;
    case RIGHT :
	vr = add_pointf (p, east);
	v0 = add_pointf (p, northeast);
	v1 = add_pointf (p, southeast);
	break;
    case BOTTOM|RIGHT :
	vr = add_pointf (p, southeast);
	v0 = add_pointf (p, east);
	v1 = add_pointf (p, south);
	break;
    case BOTTOM :
	vr = add_pointf (p, south);
	v0 = add_pointf (p, southeast);
	v1 = add_pointf (p, southwest);
	break;
    case BOTTOM|LEFT :
	vr = add_pointf (p, southwest);
	v0 = add_pointf (p, south);
	v1 = add_pointf (p, west);
	break;
    case LEFT :
	vr = add_pointf (p, west);
	v0 = add_pointf (p, southwest);
	v1 = add_pointf (p, northwest);
	break;
    case TOP|LEFT :
	vr = add_pointf (p, northwest);
	v0 = add_pointf (p, west);
	v1 = add_pointf (p, north);
	break;
    case 0 :
	break;
    default :
	assert (0);
	break;
    }

    rtr->tg->nodes[v_id].ne = 0;
    rtr->tg->nodes[v_id].ctr = p;
    for (i = starti; i < endi; i++) {
	ipair seg;
	seg.i = i;
	if (i < endi - 1)
	    seg.j = i + 1;
	else
	    seg.j = starti;
	t = findMap(rtr->trimap, seg.i, seg.j);
	if (sides && !inCone (v0, p, v1, pts[seg.i]) && !inCone (v0, p, v1, pts[seg.j]) && !raySeg(p,vr,pts[seg.i],pts[seg.j]))
	    continue;
	d = DIST(p, (rtr->tg->nodes + t)->ctr);
	addTriEdge(rtr->tg, v_id, t, d, seg);
    }
}

/* edgeToSeg:
 * Given edge from i to j, find segment associated
 * with the edge.
 *
 * This lookup could be made faster by modifying the 
 * shortest path algorithm to store the edges rather than
 * the nodes.
 */
static ipair edgeToSeg(tgraph * tg, int i, int j)
{
    ipair ip;
    tnode *np = tg->nodes + i;
    tedge *ep;

    for (i = 0; i < np->ne; i++) {
	ep = tg->edges + np->edges[i];
	if ((ep->t == j) || (ep->h == j))
	    return (ep->seg);
    }

    assert(0);
    return ip;
}

static void
freeTripoly (tripoly_t* trip)
{
    int i;
    tri* tp;
    tri* nxt;

    free (trip->poly.ps);
    for (i = 0; i < trip->poly.pn; i++) {
	for (tp = trip->triMap[i]; tp; tp = nxt) {
	    nxt = tp->nxttri;
	    free (tp);
	}
    }
    free (trip->triMap);
    free (trip);
}

/* Auxiliary data structure used to translate a path of rectangles
 * into a polygon. Each side_t represents a vertex on one side of
 * the polygon. v is the index of the vertex in the global router_t,
 * and ts is a linked list of the indices of segments of sides opposite
 * to v in some triangle on the path. These lists will be translated
 * to polygon indices by mapTri, and stored in tripoly_t.triMap.
 */
typedef struct {
    int v;
    tri *ts;
} side_t;

/* mkPoly:
 * Construct simple polygon from shortest path from t to s in g.
 * dad gives the indices of the triangles on path.
 * sx used to store index of s in points.
 * index of t is always 0
 */
static tripoly_t *mkPoly(router_t * rtr, int *dad, int s, int t,
			 pointf p_s, pointf p_t, int *sx)
{
    tripoly_t *ps;
    int nxt;
    ipair p;
    int nt = 0;
    side_t *side1;
    side_t *side2;
    int i, idx;
    int cnt1 = 0;
    int cnt2 = 0;
    pointf *pts;
    pointf *pps;
    /* maps vertex index used in router_t to vertex index used in tripoly */
    Dt_t *vmap;
    tri **trim;

    /* count number of triangles in path */
    for (nxt = dad[t]; nxt != s; nxt = dad[nxt])
	nt++;

    side1 = N_NEW(nt + 4, side_t);
    side2 = N_NEW(nt + 4, side_t);

    nxt = dad[t];
    p = edgeToSeg(rtr->tg, nxt, t);
    side1[cnt1].ts = addTri(-1, p.j, NULL);
    side1[cnt1++].v = p.i;
    side2[cnt2].ts = addTri(-1, p.i, NULL);
    side2[cnt2++].v = p.j;

    t = nxt;
    for (nxt = dad[t]; nxt >= 0; nxt = dad[nxt]) {
	p = edgeToSeg(rtr->tg, t, nxt);
	if (p.i == side1[cnt1 - 1].v) {
	    side1[cnt1 - 1].ts =
		addTri(side2[cnt2 - 1].v, p.j, side1[cnt1 - 1].ts);
	    side2[cnt2 - 1].ts =
		addTri(side1[cnt1 - 1].v, p.j, side2[cnt2 - 1].ts);
	    side2[cnt2].ts =
		addTri(side2[cnt2 - 1].v, side1[cnt1 - 1].v, NULL);
	    side2[cnt2++].v = p.j;
	} else if (p.i == side2[cnt2 - 1].v) {
	    side1[cnt1 - 1].ts =
		addTri(side2[cnt2 - 1].v, p.j, side1[cnt1 - 1].ts);
	    side2[cnt2 - 1].ts =
		addTri(side1[cnt1 - 1].v, p.j, side2[cnt2 - 1].ts);
	    side1[cnt1].ts =
		addTri(side2[cnt2 - 1].v, side1[cnt1 - 1].v, NULL);
	    side1[cnt1++].v = p.j;
	} else if (p.j == side1[cnt1 - 1].v) {
	    side1[cnt1 - 1].ts =
		addTri(side2[cnt2 - 1].v, p.i, side1[cnt1 - 1].ts);
	    side2[cnt2 - 1].ts =
		addTri(side1[cnt1 - 1].v, p.i, side2[cnt2 - 1].ts);
	    side2[cnt2].ts =
		addTri(side2[cnt2 - 1].v, side1[cnt1 - 1].v, NULL);
	    side2[cnt2++].v = p.i;
	} else {
	    side1[cnt1 - 1].ts =
		addTri(side2[cnt2 - 1].v, p.i, side1[cnt1 - 1].ts);
	    side2[cnt2 - 1].ts =
		addTri(side1[cnt1 - 1].v, p.i, side2[cnt2 - 1].ts);
	    side1[cnt1].ts =
		addTri(side2[cnt2 - 1].v, side1[cnt1 - 1].v, NULL);
	    side1[cnt1++].v = p.i;
	}
	t = nxt;
    }
    side1[cnt1 - 1].ts = addTri(-2, side2[cnt2 - 1].v, side1[cnt1 - 1].ts);
    side2[cnt2 - 1].ts = addTri(-2, side1[cnt1 - 1].v, side2[cnt2 - 1].ts);

    /* store points in pts starting with t in 0, 
     * then side1, then s, then side2 
     */
    vmap = dtopen(&ipairdisc, Dtoset);
    vmapAdd(vmap, -1, 0);
    vmapAdd(vmap, -2, cnt1 + 1);
    pps = pts = N_GNEW(nt + 4, pointf);
    trim = N_NEW(nt + 4, tri *);
    *pps++ = p_t;
    idx = 1;
    for (i = 0; i < cnt1; i++) {
	vmapAdd(vmap, side1[i].v, idx);
	*pps++ = rtr->ps[side1[i].v];
	trim[idx++] = side1[i].ts;
    }
    *pps++ = p_s;
    idx++;
    for (i = cnt2 - 1; i >= 0; i--) {
	vmapAdd(vmap, side2[i].v, idx);
	*pps++ = rtr->ps[side2[i].v];
	trim[idx++] = side2[i].ts;
    }

    for (i = 0; i < nt + 4; i++) {
	mapTri(vmap, trim[i]);
    }

    ps = NEW(tripoly_t);
    ps->poly.pn = nt + 4;  /* nt triangles gives nt+2 points plus s and t */
    ps->poly.ps = pts;
    ps->triMap = trim;

    free (side1);
    free (side2);
    dtclose(vmap);
    *sx = cnt1 + 1;		/* index of s in ps */
    return ps;
}

/* resetGraph:
 * Remove edges and nodes added for current edge routing
 */
static void resetGraph(tgraph * g, int ncnt, int ecnt)
{
    int i;
    tnode *np = g->nodes;
    g->nedges = ecnt;
    for (i = 0; i < ncnt; i++) {
	if (np->edges + np->ne == (np + 1)->edges)
	    np->ne--;
	np++;
    }
}

#define PQTYPE int
#define PQVTYPE float

#define PQ_TYPES
#include "fPQ.h"
#undef PQ_TYPES

typedef struct {
    PQ pq;
    PQVTYPE *vals;
    int *idxs;
} PPQ;

#define N_VAL(pq,n) ((PPQ*)pq)->vals[n]
#define N_IDX(pq,n) ((PPQ*)pq)->idxs[n]

#define PQ_CODE
#include "fPQ.h"
#undef PQ_CODE

#define N_DAD(n) dad[n]
#define E_WT(e) (e->dist)
#define UNSEEN -MAXFLOAT

/* triPath:
 * Find the shortest path with lengths in g from
 * v0 to v1. The returned vector (dad) encodes the
 * shorted path from v1 to v0. That path is given by
 * v1, dad[v1], dad[dad[v1]], ..., v0.
 */
static int *
triPath(tgraph * g, int n, int v0, int v1, PQ * pq)
{
    int i, j, adjn;
    double d;
    tnode *np;
    tedge *e;
    int *dad = N_NEW(n, int);

    for (i = 0; i < pq->PQsize; i++)
	N_VAL(pq, i) = UNSEEN;

    PQinit(pq);
    N_DAD(v0) = -1;
    N_VAL(pq, v0) = 0;
    if (PQinsert(pq, v0))
	return NULL;

    while ((i = PQremove(pq)) != -1) {
	N_VAL(pq, i) *= -1;
	if (i == v1)
	    break;
	np = g->nodes + i;
	for (j = 0; j < np->ne; j++) {
	    e = g->edges + np->edges[j];
	    if (e->t == i)
		adjn = e->h;
	    else
		adjn = e->t;
	    if (N_VAL(pq, adjn) < 0) {
		d = -(N_VAL(pq, i) + E_WT(e));
		if (N_VAL(pq, adjn) == UNSEEN) {
		    N_VAL(pq, adjn) = d;
		    N_DAD(adjn) = i;
		    if (PQinsert(pq, adjn)) return NULL;
		} else if (N_VAL(pq, adjn) < d) {
		    PQupdate(pq, adjn, d);
		    N_DAD(adjn) = i;
		}
	    }
	}
    }
    return dad;
}

/* makeMultiSpline:
 * FIX: we don't really use the shortest path provided by ED_path,
 * so avoid in neato spline code.
 * Return 0 on success.
 */
int makeMultiSpline(graph_t* g,  edge_t* e, router_t * rtr, int doPolyline)
{
    Ppolyline_t line = ED_path(e);
    node_t *t = agtail(e);
    node_t *h = aghead(e);
    pointf t_p = line.ps[0];
    pointf h_p = line.ps[line.pn - 1];
    tripoly_t *poly;
    int idx;
    int *sp;
    int t_id = rtr->tn;
    int h_id = rtr->tn + 1;
    int ecnt = rtr->tg->nedges;
    PPQ pq;
    PQTYPE *idxs;
    PQVTYPE *vals;
    int ret;

	/* Add endpoints to triangle graph */
    addEndpoint(rtr, t_p, t, t_id, ED_tail_port(e).side);
    addEndpoint(rtr, h_p, h, h_id, ED_head_port(e).side);

	/* Initialize priority queue */
    PQgen(&pq.pq, rtr->tn + 2, -1);
    idxs = N_GNEW(pq.pq.PQsize + 1, PQTYPE);
    vals = N_GNEW(pq.pq.PQsize + 1, PQVTYPE);
    vals[0] = 0;
    pq.vals = vals + 1;
    pq.idxs = idxs + 1;

	/* Find shortest path of triangles */
    sp = triPath(rtr->tg, rtr->tn+2, h_id, t_id, (PQ *) & pq);

    free(vals);
    free(idxs);
    PQfree(&(pq.pq), 0);

	/* Use path of triangles to generate guiding polygon */
    if (sp) {
	poly = mkPoly(rtr, sp, h_id, t_id, h_p, t_p, &idx);
	free(sp);

	/* Generate multiple splines using polygon */
	ret = genroute(g, poly, 0, idx, e, doPolyline);
	freeTripoly (poly);
    }
    else ret = -1;

    resetGraph(rtr->tg, rtr->tn, ecnt);
    return ret;
}
