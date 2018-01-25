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


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#define DEBUG

#include <stddef.h>
#include <maze.h>
#include <partition.h>
#include <memory.h>
#include <arith.h>
/* #include <values.h> */

#define MARGIN 36;

#ifdef DEBUG
char* pre = "%!PS-Adobe-2.0\n\
/node {\n\
  /Y exch def\n\
  /X exch def\n\
  /y exch def\n\
  /x exch def\n\
  newpath\n\
  x y moveto\n\
  x Y lineto\n\
  X Y lineto\n\
  X y lineto\n\
  closepath fill\n\
} def\n\
/cell {\n\
  /Y exch def\n\
  /X exch def\n\
  /y exch def\n\
  /x exch def\n\
  newpath\n\
  x y moveto\n\
  x Y lineto\n\
  X Y lineto\n\
  X y lineto\n\
  closepath stroke\n\
} def\n";

char* post = "showpage\n";

static void
psdump (cell* gcells, int n_gcells, boxf BB, boxf* rects, int nrect)
{
    int i;
    boxf bb;
    box absbb;

    absbb.LL.y = absbb.LL.x = 10;
    absbb.UR.x = absbb.LL.x + BB.UR.x - BB.LL.x;
    absbb.UR.y = absbb.LL.y + BB.UR.y - BB.LL.y;
    fputs (pre, stderr);
    fprintf (stderr, "%%%%Page: 1 1\n%%%%PageBoundingBox: %d %d %d %d\n",
       absbb.LL.x, absbb.LL.y, absbb.UR.x, absbb.UR.y);
      

    fprintf (stderr, "%f %f translate\n", 10-BB.LL.x, 10-BB.LL.y);
    fputs ("0 0 1 setrgbcolor\n", stderr);
    for (i = 0; i < n_gcells; i++) {
      bb = gcells[i].bb;
      fprintf (stderr, "%f %f %f %f node\n", bb.LL.x, bb.LL.y, bb.UR.x, bb.UR.y);
    }
    fputs ("0 0 0 setrgbcolor\n", stderr);
    for (i = 0; i < nrect; i++) {
      bb = rects[i];
      fprintf (stderr, "%f %f %f %f cell\n", bb.LL.x, bb.LL.y, bb.UR.x, bb.UR.y);
    }
    fputs ("1 0 0 setrgbcolor\n", stderr);
    fprintf (stderr, "%f %f %f %f cell\n", BB.LL.x, BB.LL.y, BB.UR.x, BB.UR.y);
    fputs (post, stderr);
}
#endif

static int
vcmpid(Dt_t* d, pointf* key1, pointf* key2, Dtdisc_t* disc)
{
  if (key1->x > key2->x) return 1;
  else if (key1->x < key2->x) return -1;
  else if (key1->y > key2->y) return 1;
  else if (key1->y < key2->y) return -1;
  else return 0;
}   

static int
hcmpid(Dt_t* d, pointf* key1, pointf* key2, Dtdisc_t* disc)
{
  if (key1->y > key2->y) return 1;
  else if (key1->y < key2->y) return -1;
  else if (key1->x > key2->x) return 1;
  else if (key1->x < key2->x) return -1;
  else return 0;
}   

typedef struct {
    snode*    np;
    pointf    p;
    Dtlink_t  link;
} snodeitem;

static Dtdisc_t vdictDisc = {
    offsetof(snodeitem,p),
    sizeof(pointf),
    offsetof(snodeitem,link),
    0,
    0,
    (Dtcompar_f)vcmpid,
    0,
    0,
    0
};
static Dtdisc_t hdictDisc = {
    offsetof(snodeitem,p),
    sizeof(pointf),
    offsetof(snodeitem,link),
    0,
    0,
    (Dtcompar_f)hcmpid,
    0,
    0,
    0
};

#define delta 1        /* weight of length */
#define mu 500           /* weight of bends */

#define BEND(g,e) ((g->nodes + e->v1)->isVert != (g->nodes + e->v2)->isVert)
#define HORZ(g,e) ((g->nodes + e->v1)->isVert)
#define BIG 16384
#define CHANSZ(w) (((w)-3)/2)
#define IS_SMALL(v) (CHANSZ(v) < 2)
/* #define CHANSZ(w) (w) */

/* updateWt:
 * At present, we use a step function. When a bound is reached, the weight
 * becomes huge. We might consider bumping up the weight more gradually, the
 * thinner the channel, the faster the weight rises.
 */
static void
updateWt (cell* cp, sedge* ep, int sz)
{
    ep->cnt++;
    if (ep->cnt > sz) {
	ep->cnt = 0;
	ep->weight += BIG;
    }
}

/* updateWts:
 * Iterate over edges in a cell, adjust weights as necessary.
 * It always updates the bent edges belonging to a cell.
 * A horizontal/vertical edge is updated only if the edge traversed
 * is bent, or if it is the traversed edge.
 */
void
updateWts (sgraph* g, cell* cp, sedge* ep)
{
    int i;
    sedge* e;
    int isBend = BEND(g,ep);
    int hsz = CHANSZ (cp->bb.UR.y - cp->bb.LL.y);
    int vsz = CHANSZ (cp->bb.UR.x - cp->bb.LL.x);
    int minsz = MIN(hsz, vsz);

    /* Bend edges are added first */
    for (i = 0; i < cp->nedges; i++) {
	e = cp->edges[i];
	if (!BEND(g,e)) break;
	updateWt (cp, e, minsz);
}

    for (; i < cp->nedges; i++) {
	e = cp->edges[i];
	if (isBend || (e == ep)) updateWt (cp, e, (HORZ(g,e)?hsz:vsz));
    }
}

/* markSmall:
 * cp corresponds to a real node. If it is small, its associated cells should
 * be marked as usable.
 */
static void
markSmall (cell* cp, sgraph* g)
{
    int i;
    snode* onp;
    cell* ocp;

    if (IS_SMALL(cp->bb.UR.y-cp->bb.LL.y)) {
	for (i = 0; i < cp->nsides; i++) {
	    onp = cp->sides[i];
	    if (!onp->isVert) continue;
	    if (onp->cells[0] == cp) {  /* onp on the right of cp */
		ocp = onp->cells[1];
		ocp->flags |= MZ_SMALLV;
		while ((onp = ocp->sides[M_RIGHT]) && !IsNode(onp->cells[1])) {
		    ocp = onp->cells[1];
		    ocp->flags |= MZ_SMALLV;
		}
	    }
	    else {                      /* onp on the left of cp */
		ocp = onp->cells[0];
		ocp->flags |= MZ_SMALLV;
		while ((onp = ocp->sides[M_LEFT]) && !IsNode(onp->cells[0])) {
		    ocp = onp->cells[0];
		    ocp->flags |= MZ_SMALLV;
		}
	    }
	}
    }

    if (IS_SMALL(cp->bb.UR.x-cp->bb.LL.x)) {
	for (i = 0; i < cp->nsides; i++) {
	    onp = cp->sides[i];
	    if (onp->isVert) continue;
	    if (onp->cells[0] == cp) {  /* onp on the top of cp */
		ocp = onp->cells[1];
		ocp->flags |= MZ_SMALLH;
		while ((onp = ocp->sides[M_TOP]) && !IsNode(onp->cells[1])) {
		    ocp = onp->cells[1];
		    ocp->flags |= MZ_SMALLH;
		}
	    }
	    else {                      /* onp on the bottom of cp */
		ocp = onp->cells[0];
		ocp->flags |= MZ_SMALLH;
		while ((onp = ocp->sides[M_BOTTOM]) && !IsNode(onp->cells[0])) {
		    ocp = onp->cells[0];
		    ocp->flags |= MZ_SMALLH;
		}
	    }
	}
    }
}

static void
createSEdges (cell* cp, sgraph* g)
{
    boxf bb = cp->bb;
    double hwt = delta*(bb.UR.x-bb.LL.x);
    double vwt = delta*(bb.UR.y-bb.LL.y);
    double wt = (hwt + vwt)/2.0 + mu;
    
    /* We automatically make small channels have high cost to guide routes to
     * more spacious channels.
     */
    if (IS_SMALL(bb.UR.y-bb.LL.y) && !IsSmallV(cp)) {
	hwt = BIG;
	wt = BIG;
    }
    if (IS_SMALL(bb.UR.x-bb.LL.x) && !IsSmallH(cp)) {
	vwt = BIG;
	wt = BIG;
    }

    if (cp->sides[M_LEFT] && cp->sides[M_TOP])
	cp->edges[cp->nedges++] = createSEdge (g, cp->sides[M_LEFT], cp->sides[M_TOP], wt);
    if (cp->sides[M_TOP] && cp->sides[M_RIGHT])
	cp->edges[cp->nedges++] = createSEdge (g, cp->sides[M_TOP], cp->sides[M_RIGHT], wt);
    if (cp->sides[M_LEFT] && cp->sides[M_BOTTOM])
	cp->edges[cp->nedges++] = createSEdge (g, cp->sides[M_LEFT], cp->sides[M_BOTTOM], wt);
    if (cp->sides[M_BOTTOM] && cp->sides[M_RIGHT])
	cp->edges[cp->nedges++] = createSEdge (g, cp->sides[M_BOTTOM], cp->sides[M_RIGHT], wt);
    if (cp->sides[M_TOP] && cp->sides[M_BOTTOM])
	cp->edges[cp->nedges++] = createSEdge (g, cp->sides[M_TOP], cp->sides[M_BOTTOM], vwt);
    if (cp->sides[M_LEFT] && cp->sides[M_RIGHT])
	cp->edges[cp->nedges++] = createSEdge (g, cp->sides[M_LEFT], cp->sides[M_RIGHT], hwt);
}

static snode*
findSVert (sgraph* g, Dt_t* cdt, pointf p, snodeitem* ditems, boolean isVert)
{
    snodeitem* n = dtmatch (cdt, &p);

    if (!n) {
        snode* np = createSNode (g);
        assert(ditems);
        n = ditems + np->index;
	n->p = p;
	n->np = np;
	np->isVert = isVert;
	dtinsert (cdt, n);
    }

    return n->np;
}

static void
chkSgraph (sgraph* g)
{
  int i;
  snode* np;

  for (i = 0; i < g->nnodes; i++) {
    np = g->nodes+i;
    if (!np->cells[0]) fprintf (stderr, "failed at node %d[0]\n", i); 
    assert (np->cells[0]);
    if (!np->cells[1]) fprintf (stderr, "failed at node %d[1]\n", i); 
    assert (np->cells[1]);
  }
    
}

/* mkMazeGraph:
 */
static sgraph*
mkMazeGraph (maze* mp, boxf bb)
{
    int nsides, i, ncnt, maxdeg;
    int bound = 4*mp->ncells;
    sgraph* g = createSGraph (bound + 2);
    Dt_t* vdict = dtopen(&vdictDisc,Dtoset);
    Dt_t* hdict = dtopen(&hdictDisc,Dtoset);
    snodeitem* ditems = N_NEW(bound, snodeitem);
    snode** sides;

    /* For each cell, create if necessary and attach a node in search
     * corresponding to each internal face. The node also gets
     * a pointer to the cell.
     */
    sides = N_NEW(4*mp->ncells, snode*);
    ncnt = 0;
    for (i = 0; i < mp->ncells; i++) {
	cell* cp = mp->cells+i;
        snode* np;
	pointf pt;

	cp->nsides = 4; 
	cp->sides = sides + 4*i;
	if (cp->bb.UR.x < bb.UR.x) {
	    pt.x = cp->bb.UR.x;
	    pt.y = cp->bb.LL.y;
	    np = findSVert (g, vdict, pt, ditems, TRUE);
	    np->cells[0] = cp;
	    cp->sides[M_RIGHT] = np;
	}
	if (cp->bb.UR.y < bb.UR.y) {
	    pt.x = cp->bb.LL.x;
	    pt.y = cp->bb.UR.y;
	    np = findSVert (g, hdict, pt, ditems, FALSE);
	    np->cells[0] = cp;
	    cp->sides[M_TOP] = np;
	}
	if (cp->bb.LL.x > bb.LL.x) {
	    np = findSVert (g, vdict, cp->bb.LL, ditems, TRUE);
	    np->cells[1] = cp;
	    cp->sides[M_LEFT] = np;
	}
	if (cp->bb.LL.y > bb.LL.y) {
	    np = findSVert (g, hdict, cp->bb.LL, ditems, FALSE);
	    np->cells[1] = cp;
	    cp->sides[M_BOTTOM] = np;
	}
    }

    /* For each gcell, corresponding to a node in the input graph,
     * connect it to its corresponding search nodes.
     */
    maxdeg = 0;
    sides = N_NEW(g->nnodes, snode*);
    nsides = 0;
    for (i = 0; i < mp->ngcells; i++) {
	cell* cp = mp->gcells+i;
        pointf pt; 
	snodeitem* np;

	cp->sides = sides+nsides;
        pt = cp->bb.LL;
	np = dtmatch (hdict, &pt);
	for (; np && np->p.x < cp->bb.UR.x; np = dtnext (hdict, np)) {
	    cp->sides[cp->nsides++] = np->np;
	    np->np->cells[1] = cp;
	}
	np = dtmatch (vdict, &pt);
	for (; np && np->p.y < cp->bb.UR.y; np = dtnext (vdict, np)) {
	    cp->sides[cp->nsides++] = np->np;
	    np->np->cells[1] = cp;
	}
	pt.y = cp->bb.UR.y;
	np = dtmatch (hdict, &pt);
	for (; np && np->p.x < cp->bb.UR.x; np = dtnext (hdict, np)) {
	    cp->sides[cp->nsides++] = np->np;
	    np->np->cells[0] = cp;
	}
	pt.x = cp->bb.UR.x;
	pt.y = cp->bb.LL.y;
	np = dtmatch (vdict, &pt);
	for (; np && np->p.y < cp->bb.UR.y; np = dtnext (vdict, np)) {
	    cp->sides[cp->nsides++] = np->np;
	    np->np->cells[0] = cp;
	}
	nsides += cp->nsides;
        if (cp->nsides > maxdeg) maxdeg = cp->nsides;
    }
    /* sides = RALLOC (nsides, sides, snode*); */

    /* Mark cells that are small because of a small node, not because of the close
     * alignment of two rectangles.
     */
    for (i = 0; i < mp->ngcells; i++) {
	cell* cp = mp->gcells+i;
	markSmall (cp, g);
    }

    /* Set index of two dummy nodes used for real nodes */
    g->nodes[g->nnodes].index = g->nnodes;
    g->nodes[g->nnodes+1].index = g->nnodes+1;
    
    /* create edges
     * For each ordinary cell, there can be at most 6 edges.
     * At most 2 gcells will be used at a time, and each of these
     * can have at most degree maxdeg.
     */
    initSEdges (g, maxdeg);
    for (i = 0; i < mp->ncells; i++) {
	cell* cp = mp->cells+i;
	createSEdges (cp, g);
    }

    /* tidy up memory */
    /* g->nodes = RALLOC (g->nnodes+2, g->nodes, snode); */
    /* g->edges = RALLOC (g->nedges+2*maxdeg, g->edges, sedge); */
    dtclose (vdict);
    dtclose (hdict);
    free (ditems);

chkSgraph (g);
    /* save core graph state */
    gsave(g);
    return g;
}

/* mkMaze:
 */
maze*
mkMaze (graph_t* g, int doLbls)
{
    node_t* n;
    maze* mp = NEW(maze);
    boxf* rects;
    int i, nrect;
    cell* cp;
    double w2, h2;
    boxf bb, BB;

    mp->ngcells = agnnodes(g);
    cp = mp->gcells = N_NEW(mp->ngcells, cell);

    BB.LL.x = BB.LL.y = MAXDOUBLE;
    BB.UR.x = BB.UR.y = -MAXDOUBLE;
    for (n = agfstnode (g); n; n = agnxtnode(g,n)) {
        w2 = ND_xsize(n)/2.0;
	if (w2 < 1) w2 = 1;
        h2 = ND_ysize(n)/2.0;
	if (h2 < 1) h2 = 1;
        bb.LL.x = ND_coord(n).x - w2;
        bb.UR.x = ND_coord(n).x + w2;
        bb.LL.y = ND_coord(n).y - h2;
        bb.UR.y = ND_coord(n).y + h2;
	BB.LL.x = MIN(BB.LL.x, bb.LL.x);
	BB.LL.y = MIN(BB.LL.y, bb.LL.y);
	BB.UR.x = MAX(BB.UR.x, bb.UR.x);
	BB.UR.y = MAX(BB.UR.y, bb.UR.y);
        cp->bb = bb;
	cp->flags |= MZ_ISNODE;
        ND_alg(n) = cp;
	cp++;
    }

    if (doLbls) {
    }

    BB.LL.x -= MARGIN;
    BB.LL.y -= MARGIN;
    BB.UR.x += MARGIN;
    BB.UR.y += MARGIN;
    rects = partition (mp->gcells, mp->ngcells, &nrect, BB);

#ifdef DEBUG
    if (odb_flags & ODB_MAZE) psdump (mp->gcells, mp->ngcells, BB, rects, nrect);
#endif
    mp->cells = N_NEW(nrect, cell);
    mp->ncells = nrect;
    for (i = 0; i < nrect; i++) {
	mp->cells[i].bb = rects[i];
    }
    free (rects);

    mp->sg = mkMazeGraph (mp, BB);
    return mp;
}

void freeMaze (maze* mp)
{
    free (mp->cells[0].sides);
    free (mp->gcells[0].sides);
    free (mp->cells);
    free (mp->gcells);
    freeSGraph (mp->sg);
    dtclose (mp->hchans);
    dtclose (mp->vchans);
    free (mp);
}

