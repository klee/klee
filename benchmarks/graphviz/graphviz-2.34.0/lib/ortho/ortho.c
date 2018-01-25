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


/* TODO:
 * In dot, prefer bottom or top routing
 * In general, prefer closest side to closest side routing.
 * Edge labels
 * Ports/compass points
 * ordering attribute
 * Weights on edges in nodes
 * Edge concentrators?
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#define DEBUG
#include <stddef.h>
#include <setjmp.h>
#include <maze.h>
#include "fPQ.h"
#include "memory.h"
#include "geomprocs.h"
#include "globals.h"
#include "render.h"
#include "pointset.h"
typedef struct {
    int d;
    Agedge_t* e;
} epair_t;

static jmp_buf jbuf;

#ifdef DEBUG
static void emitSearchGraph (FILE* fp, sgraph* sg);
static void emitGraph (FILE* fp, maze* mp, int n_edges, route* route_list, epair_t[]);
int odb_flags;
#endif

#define CELL(n) ((cell*)ND_alg(n))
#define MID(a,b) (((a)+(b))/2.0)
#define SC 1

/* cellOf:
 * Given 2 snodes sharing a cell, return the cell.
 */
static cell*
cellOf (snode* p, snode* q)
{
    cell* cp = p->cells[0];
    if ((cp == q->cells[0]) || (cp == q->cells[1])) return cp; 
    else return p->cells[1];
}

static pointf
midPt (cell* cp)
{
    pointf p;
    p.x = MID(cp->bb.LL.x,cp->bb.UR.x);
    p.y = MID(cp->bb.LL.y,cp->bb.UR.y);
    return p;
}

/* sidePt:
 * Given a cell and an snode on one of its sides, return the
 * midpoint of the side.
 */
static pointf
sidePt (snode* ptr, cell* cp)
{
    pointf pt;
    if (cp == ptr->cells[1]) {
	if (ptr->isVert) {
	    pt.x = cp->bb.LL.x;
	    pt.y = MID(cp->bb.LL.y,cp->bb.UR.y);
	}
	else {
	    pt.x = MID(cp->bb.LL.x,cp->bb.UR.x);
	    pt.y = cp->bb.LL.y;
	}
    }
    else {
	if (ptr->isVert) {
	    pt.x = cp->bb.UR.x;
	    pt.y = MID(cp->bb.LL.y,cp->bb.UR.y);
	}
	else {
	    pt.x = MID(cp->bb.LL.x,cp->bb.UR.x);
	    pt.y = cp->bb.UR.y;
	}
    }
    return pt;
}

/* setSet:
 * Initialize and normalize segments.
 * p1 stores smaller value
 * Assume b1 != b2
 */
static void
setSeg (segment* sp, int dir, double fix, double b1, double b2, int l1, int l2)
{
    sp->isVert = dir;
    sp->comm_coord = fix;
    if (b1 < b2) {
	sp->p.p1 = b1;
	sp->p.p2 = b2;
	sp->l1 = l1;
	sp->l2 = l2;
	sp->flipped = 0;
    }
    else {
	sp->p.p2 = b1;
	sp->p.p1 = b2;
	sp->l2 = l1;
	sp->l1 = l2;
	sp->flipped = 1;
    }
}

/* Convert route in shortest path graph to route
 * of segments. This records the first and last cells,
 * plus cells where the path bends.
 * Note that the shortest path will always have at least 4 nodes:
 * the two dummy nodes representing the center of the two real nodes,
 * and the two nodes on the boundary of the two real nodes.
 */
#define PUSH(rte,P) (rte.p[rte.n++] = P)

static route
convertSPtoRoute (sgraph* g, snode* fst, snode* lst)
{
    route rte;
    snode* ptr;
    snode* next;
    snode* prev;  /* node in shortest path just previous to next */
    int i, sz = 0;
    cell* cp;
    cell* ncp;
    segment seg;
    double fix, b1, b2;
    int l1, l2;
    pointf bp1, bp2, prevbp;  /* bend points */

	/* count no. of nodes in shortest path */
    for (ptr = fst; ptr; ptr = N_DAD(ptr)) sz++;
    rte.n = 0;
    rte.segs = N_NEW(sz-2, segment);  /* at most sz-2 segments */

    seg.prev = seg.next = 0;
    ptr = prev = N_DAD(fst);
    next = N_DAD(ptr);
    if (IsNode(ptr->cells[0]))
	cp = ptr->cells[1];
    else
	cp = ptr->cells[0];
    bp1 = sidePt (ptr, cp);
    while (N_DAD(next)!=NULL) {
	ncp = cellOf (prev, next);
	updateWts (g, ncp, N_EDGE(ptr));

        /* add seg if path bends or at end */
	if ((ptr->isVert != next->isVert) || (N_DAD(next) == lst)) {
	    if (ptr->isVert != next->isVert)
		bp2 = midPt (ncp);
	    else
		bp2 = sidePt(next, ncp);
	    if (ptr->isVert) {   /* horizontal segment */
		if (ptr == N_DAD(fst)) l1 = B_NODE;
		else if (prevbp.y > bp1.y) l1 = B_UP;
		else l1 = B_DOWN; 
		if (ptr->isVert != next->isVert) {
		    if (next->cells[0] == ncp) l2 = B_UP;
		    else l2 = B_DOWN;
		}
		else l2 = B_NODE;
		fix = cp->bb.LL.y;
		b1 = cp->bb.LL.x;
		b2 = ncp->bb.LL.x;
	    }
	    else {   /* vertical segment */
		if (ptr == N_DAD(fst)) l1 = B_NODE;
		else if (prevbp.x > bp1.x) l1 = B_RIGHT;
		else l1 = B_LEFT; 
		if (ptr->isVert != next->isVert) {
		    if (next->cells[0] == ncp) l2 = B_RIGHT;
		    else l2 = B_LEFT;
		}
		else l2 = B_NODE;
		fix = cp->bb.LL.x;
		b1 = cp->bb.LL.y;
		b2 = ncp->bb.LL.y;
	    }
	    setSeg (&seg, !ptr->isVert, fix, b1, b2, l1, l2);
	    rte.segs[rte.n++] = seg;
	    cp = ncp;
	    prevbp = bp1;
	    bp1 = bp2;
	    if ((ptr->isVert != next->isVert) && (N_DAD(next) == lst)) {
		bp2 = sidePt(next, ncp);
		l2 = B_NODE;
		if (next->isVert) {   /* horizontal segment */
		    if (prevbp.y > bp1.y) l1 = B_UP;
		    else l1 = B_DOWN; 
		    fix = cp->bb.LL.y;
		    b1 = cp->bb.LL.x;
		    b2 = ncp->bb.LL.x;
		}
		else {
		    if (prevbp.x > bp1.x) l1 = B_RIGHT;
		    else l1 = B_LEFT; 
		    fix = cp->bb.LL.x;
		    b1 = cp->bb.LL.y;
		    b2 = ncp->bb.LL.y;
		}
		setSeg (&seg, !next->isVert, fix, b1, b2, l1, l2);
		rte.segs[rte.n++] = seg;
	    }
	    ptr = next;
	}
	prev = next;
	next = N_DAD(next);
    }

    rte.segs = realloc (rte.segs, rte.n*sizeof(segment));
    for (i=0; i<rte.n; i++) {
	if (i > 0)
	    rte.segs[i].prev = rte.segs + (i-1);
	if (i < rte.n-1)
	    rte.segs[i].next = rte.segs + (i+1);
    }

    return rte;
}

typedef struct {
    Dtlink_t  link;
    double    v;
    Dt_t*     chans;
} chanItem;

static void
freeChannel (Dt_t* d, channel* cp, Dtdisc_t* disc)
{
    free_graph (cp->G);
    free (cp->seg_list);
    free (cp);
}

static void
freeChanItem (Dt_t* d, chanItem* cp, Dtdisc_t* disc)
{
    dtclose (cp->chans);
    free (cp);
}

/* chancmpid:
 * Compare intervals. Two intervals are equal if one contains
 * the other. Otherwise, the one with the smaller p1 value is
 * less. 
 * This combines two separate functions into one. Channels are
 * disjoint, so we really only need to key on p1.
 * When searching for a channel containing a segment, we rely on
 * interval containment to return the correct channel.
 */
static int
chancmpid(Dt_t* d, paird* key1, paird* key2, Dtdisc_t* disc)
{
  if (key1->p1 > key2->p1) {
    if (key1->p2 <= key2->p2) return 0;
    else return 1;
  }
  else if (key1->p1 < key2->p1) {
    if (key1->p2 >= key2->p2) return 0;
    else return -1;
  }
  else return 0;
}   

static int
dcmpid(Dt_t* d, double* key1, double* key2, Dtdisc_t* disc)
{
  if (*key1 > *key2) return 1;
  else if (*key1 < *key2) return -1;
  else return 0;
}   

static Dtdisc_t chanDisc = {
    offsetof(channel,p),
    sizeof(paird),
    offsetof(channel,link),
    0,
    (Dtfree_f)freeChannel,
    (Dtcompar_f)chancmpid,
    0,
    0,
    0
};

static Dtdisc_t chanItemDisc = {
    offsetof(chanItem,v),
    sizeof(double),
    offsetof(chanItem,link),
    0,
    (Dtfree_f)freeChanItem,
    (Dtcompar_f)dcmpid,
    0,
    0,
    0
};

static void
addChan (Dt_t* chdict, channel* cp, double j)
{
    chanItem* subd = dtmatch (chdict, &j);

    if (!subd) {
	subd = NEW (chanItem);
	subd->v = j;
	subd->chans = dtopen (&chanDisc, Dtoset);
	dtinsert (chdict, subd);
    }
    dtinsert (subd->chans, cp);
}

static Dt_t*
extractHChans (maze* mp)
{
    int i;
    snode* np;
    Dt_t* hchans = dtopen (&chanItemDisc, Dtoset);

    for (i = 0; i < mp->ncells; i++) {
	channel* chp;
	cell* cp = mp->cells+i;
	cell* nextcp;
	if (IsHScan(cp)) continue;

	/* move left */
	while ((np = cp->sides[M_LEFT]) && (nextcp = np->cells[0]) &&
	    !IsNode(nextcp)) {
	    cp = nextcp;
	}

	chp = NEW(channel);
	chp->cp = cp;
	chp->p.p1 = cp->bb.LL.x;

	/* move right */
	cp->flags |= MZ_HSCAN;
	while ((np = cp->sides[M_RIGHT]) && (nextcp = np->cells[1]) &&
	    !IsNode(nextcp)) {
	    cp = nextcp;
	    cp->flags |= MZ_HSCAN;
	}

        chp->p.p2 = cp->bb.UR.x;
	addChan (hchans, chp, chp->cp->bb.LL.y);
    }
    return hchans;
}

static Dt_t*
extractVChans (maze* mp)
{
    int i;
    snode* np;
    Dt_t* vchans = dtopen (&chanItemDisc, Dtoset);

    for (i = 0; i < mp->ncells; i++) {
	channel* chp;
	cell* cp = mp->cells+i;
	cell* nextcp;
	if (IsVScan(cp)) continue;

	/* move down */
	while ((np = cp->sides[M_BOTTOM]) && (nextcp = np->cells[0]) &&
	    !IsNode(nextcp)) {
	    cp = nextcp;
	}

	chp = NEW(channel);
	chp->cp = cp;
	chp->p.p1 = cp->bb.LL.y;

	/* move up */
	cp->flags |= MZ_VSCAN;
	while ((np = cp->sides[M_TOP]) && (nextcp = np->cells[1]) &&
	    !IsNode(nextcp)) {
	    cp = nextcp;
	    cp->flags |= MZ_VSCAN;
	}

        chp->p.p2 = cp->bb.UR.y;
	addChan (vchans, chp, chp->cp->bb.LL.x);
    }
    return vchans;
}

static void
insertChan (channel* chan, segment* seg)
{
    seg->ind_no = chan->cnt++;
    chan->seg_list = ALLOC(chan->cnt, chan->seg_list, segment*);
    chan->seg_list[chan->cnt-1] = seg;
}

static channel*
chanSearch (Dt_t* chans, segment* seg)
{
  channel* cp;
  chanItem* chani = dtmatch (chans, &seg->comm_coord);
  assert (chani);
  cp = dtmatch (chani->chans, &seg->p);
  assert (cp);
  return cp;
}

static void
assignSegs (int nrtes, route* route_list, maze* mp)
{
    channel* chan;
    int i, j;

    for (i=0;i<nrtes;i++) {
	route rte = route_list[i];
	for (j=0;j<rte.n;j++) {
	    segment* seg = rte.segs+j;
	    if (seg->isVert)
		chan = chanSearch(mp->vchans, seg);
	    else
		chan = chanSearch(mp->hchans, seg);
	    insertChan (chan, seg);
	}
    }
}

/* addLoop:
 * Add two temporary nodes to sgraph corresponding to two ends of a loop at cell cp, i
 * represented by dp and sp.
 */
static void
addLoop (sgraph* sg, cell* cp, snode* dp, snode* sp)
{
    int i;
    int onTop;
    pointf midp = midPt (cp);

    for (i = 0; i < cp->nsides; i++) {
	cell* ocp;
	pointf p;
	double wt;
	snode* onp = cp->sides[i];

	if (onp->isVert) continue;
	if (onp->cells[0] == cp) {
	    onTop = 1;
	    ocp = onp->cells[1];
	}
	else {
	    onTop = 0;
	    ocp = onp->cells[0];
	}
	p = sidePt (onp, ocp);
	wt = abs(p.x - midp.x) +  abs(p.y - midp.y);
	if (onTop)
	    createSEdge (sg, sp, onp, 0);  /* FIX weight */
	else
	    createSEdge (sg, dp, onp, 0);  /* FIX weight */
    }
    sg->nnodes += 2;
}

/* addNodeEdges:
 * Add temporary node to sgraph corresponding to cell cp, represented
 * by np.
 */
static void
addNodeEdges (sgraph* sg, cell* cp, snode* np)
{
    int i;
    pointf midp = midPt (cp);

    for (i = 0; i < cp->nsides; i++) {
	snode* onp = cp->sides[i];
	cell* ocp;
	pointf p;
	double wt;

	if (onp->cells[0] == cp)
	    ocp = onp->cells[1];
	else
	    ocp = onp->cells[0];
	p = sidePt (onp, ocp);
	wt = abs(p.x - midp.x) +  abs(p.y - midp.y);
	createSEdge (sg, np, onp, 0);  /* FIX weight */
    }
    sg->nnodes++;
#ifdef DEBUG
    np->cells[0] = np->cells[1] = cp;
#endif
}

#ifdef DEBUG

#include <intset.h>
static char* bendToStr (bend b)
{
  char* s;
  switch (b) {
  case B_NODE :
    s = "B_NODE";
    break;
  case B_UP :
    s = "B_UP";
    break;
  case B_LEFT :
    s = "B_LEFT";
    break;
  case B_DOWN :
    s = "B_DOWN";
    break;
  case B_RIGHT :
    s = "B_RIGHT";
    break;
  }
  return s;
}

static void putSeg (FILE* fp, segment* seg)
{
  if (seg->isVert)
    fprintf (fp, "((%f,%f),(%f,%f)) %s %s", seg->comm_coord, seg->p.p1,
      seg->comm_coord, seg->p.p2, bendToStr (seg->l1), bendToStr (seg->l2));
  else
    fprintf (fp, "((%f,%f),(%f,%f)) %s %s", seg->p.p1,seg->comm_coord, 
      seg->p.p2, seg->comm_coord, bendToStr (seg->l1), bendToStr (seg->l2));
}

static void
dumpChanG (channel* cp, int v)
{
  int k;
  intitem* ip;
  Dt_t* adj;

  if (cp->cnt < 2) return;
  fprintf (stderr, "channel %d (%f,%f)\n", v, cp->p.p1, cp->p.p2);
  for (k=0;k<cp->cnt;k++) {
    adj = cp->G->vertices[k].adj_list;
    if (dtsize(adj) == 0) continue;
    putSeg (stderr, cp->seg_list[k]);
    fputs (" ->\n", stderr);
    for (ip = (intitem*)dtfirst(adj); ip; ip = (intitem*)dtnext(adj, ip)) {
      fputs ("     ", stderr);
      putSeg (stderr, cp->seg_list[ip->id]);
      fputs ("\n", stderr);
    }
  }
}
#endif

static void
assignTrackNo (Dt_t* chans)
{
    Dt_t* lp;
    Dtlink_t* l1;
    Dtlink_t* l2;
    channel* cp;
    int k;

    for (l1 = dtflatten (chans); l1; l1 = dtlink(chans,l1)) {
	lp = ((chanItem*)l1)->chans;
	for (l2 = dtflatten (lp); l2; l2 = dtlink(lp,l2)) {
	    cp = (channel*)l2;
	    if (cp->cnt) {
#ifdef DEBUG
    if (odb_flags & ODB_CHANG) dumpChanG (cp, ((chanItem*)l1)->v);
#endif
		top_sort (cp->G);
		for (k=0;k<cp->cnt;k++)
		    cp->seg_list[k]->track_no = cp->G->vertices[k].topsort_order+1;
	    }
   	}
    }
}

static void
create_graphs(Dt_t* chans)
{
    Dt_t* lp;
    Dtlink_t* l1;
    Dtlink_t* l2;
    channel* cp;

    for (l1 = dtflatten (chans); l1; l1 = dtlink(chans,l1)) {
	lp = ((chanItem*)l1)->chans;
	for (l2 = dtflatten (lp); l2; l2 = dtlink(lp,l2)) {
	    cp = (channel*)l2;
	    cp->G = make_graph (cp->cnt);
   	}
    }
}

static int
eqEndSeg (bend S1l2, bend S2l2, bend T1, bend T2)
{
    if (((S1l2==T2)&&(S2l2=!T2))
     || ((S1l2==B_NODE)&&(S2l2==T1)))
	return(0);
    else
	return(-1);
}

static int
overlapSeg (segment* S1, segment* S2, bend T1, bend T2)
{
	if(S1->p.p2<S2->p.p2) {
		if(S1->l2==T1&&S2->l1==T2) return(-1);
		else if(S1->l2==T2&&S2->l1==T1) return(1);
		else return(0);
	}
	else if(S1->p.p2==S2->p.p2) {
		if(S2->l1==T2) return eqEndSeg (S1->l2, S2->l2, T1, T2);
		else return -1*eqEndSeg (S2->l2, S1->l2, T1, T2);
	}
	else { /* S1->p.p2>S2->p.p2 */
		if(S2->l1==T2&&S2->l2==T2) return(-1);
		else if (S2->l1==T1&&S2->l2==T1) return(1);
		else return(0);
	}
}

static int
ellSeg (bend S1l1, bend S1l2, bend T)
{
    if (S1l1 == T) {
	if (S1l2== T) return -1;
	else return 0;
    }
    else return 1;
}

static int
segCmp (segment* S1, segment* S2, bend T1, bend T2)
{
	/* no overlap */
    if((S1->p.p2<S2->p.p1)||(S1->p.p1>S2->p.p2)) return(0);
	/* left endpoint of S2 inside S1 */
    if(S1->p.p1<S2->p.p1&&S2->p.p1<S1->p.p2)
	return overlapSeg (S1, S2, T1, T2);
	/* left endpoint of S1 inside S2 */
    else if(S2->p.p1<S1->p.p1&&S1->p.p1<S2->p.p2)
	return -1*overlapSeg (S2, S1, T1, T2);
    else if(S1->p.p1==S2->p.p1) {
	if(S1->p.p2==S2->p.p2) {
	    if((S1->l1==S2->l1)&&(S1->l2==S2->l2))
		return(0);
	    else if (S2->l1==S2->l2) {
		if(S2->l1==T1) return(1);
		else if(S2->l1==T2) return(-1);
		else if ((S1->l1!=T1)&&(S1->l2!=T1)) return (1);
		else if ((S1->l1!=T2)&&(S1->l2!=T2)) return (-1);
		else return 0;
	    }
	    else if ((S2->l1==T1)&&(S2->l2==T2)) {
		if ((S1->l1!=T1)&&(S1->l2==T2)) return 1;
		else if ((S1->l1==T1)&&(S1->l2!=T2)) return -1;
		else return 0;
	    }
	    else if ((S2->l2==T1)&&(S2->l1==T2)) {
		if ((S1->l2!=T1)&&(S1->l1==T2)) return 1;
		else if ((S1->l2==T1)&&(S1->l1!=T2)) return -1;
		else return 0;
	    }
	    else if ((S2->l1==B_NODE)&&(S2->l2==T1)) {
		return ellSeg (S1->l1, S1->l2, T1);
	    }
	    else if ((S2->l1==B_NODE)&&(S2->l2==T2)) {
		return -1*ellSeg (S1->l1, S1->l2, T2);
	    }
	    else if ((S2->l1==T1)&&(S2->l2==B_NODE)) {
		return ellSeg (S1->l2, S1->l1, T1);
	    }
	    else { /* ((S2->l1==T2)&&(S2->l2==B_NODE)) */
		return -1*ellSeg (S1->l2, S1->l1, T2);
	    }
	}
	else if(S1->p.p2<S2->p.p2) {
	    if(S1->l2==T1)
		return eqEndSeg (S2->l1, S1->l1, T1, T2);
	    else
		return -1*eqEndSeg (S2->l1, S1->l1, T1, T2);
	}
	else { /* S1->p.p2>S2->p.p2 */
	    if(S2->l2==T2)
		return eqEndSeg (S1->l1, S2->l1, T1, T2);
	    else
		return -1*eqEndSeg (S1->l1, S2->l1, T1, T2);
	}
    }
    else if(S1->p.p2==S2->p.p1) {
	if(S1->l2==S2->l1) return(0);
	else if(S1->l2==T2) return(1);
	else return(-1);
    }
    else { /* S1->p.p1==S2->p.p2 */
	if(S1->l1==S2->l2) return(0);
	else if(S1->l1==T2) return(1);
	else return(-1);
    }
    assert(0);
    return 0;
}

/* Function seg_cmp returns
 *  -1 if S1 HAS TO BE to the right/below S2 to avoid a crossing, 
 *   0 if a crossing is unavoidable or there is no crossing at all or 
 *     the segments are parallel,
 *   1 if S1 HAS TO BE to the left/above S2 to avoid a crossing
 *
 * Note: This definition means horizontal segments have track numbers
 * increasing as y decreases, while vertical segments have track numbers
 * increasing as x increases. It would be good to make this consistent,
 * with horizontal track numbers increasing with y. This can be done by
 * switching B_DOWN and B_UP in the first call to segCmp. At present,
 * though, I'm not sure what assumptions are made in handling parallel
 * segments, so we leave the code alone for the time being.
 */
static int
seg_cmp(segment* S1, segment* S2)		
{
    if(S1->isVert!=S2->isVert||S1->comm_coord!=S2->comm_coord) {
	agerr (AGERR, "incomparable segments !! -- Aborting\n");
	longjmp(jbuf, 1);
    }
    if(S1->isVert)
	return segCmp (S1, S2, B_RIGHT, B_LEFT);
    else
	return segCmp (S1, S2, B_DOWN, B_UP);
}

static void 
add_edges_in_G(channel* cp)
{
    int x,y;
    segment** seg_list = cp->seg_list;
    int size = cp->cnt;
    rawgraph* G = cp->G;

    for(x=0;x+1<size;x++) {
	for(y=x+1;y<size;y++) {
	    switch (seg_cmp(seg_list[x],seg_list[y])) {
	    case 1:
		insert_edge(G,x,y);
		break;
	    case 0:
		break;
	    case -1:
		insert_edge(G,y,x);
		break;
	    }
	}
    }
}

static void
add_np_edges (Dt_t* chans)
{
    Dt_t* lp;
    Dtlink_t* l1;
    Dtlink_t* l2;
    channel* cp;

    for (l1 = dtflatten (chans); l1; l1 = dtlink(chans,l1)) {
	lp = ((chanItem*)l1)->chans;
	for (l2 = dtflatten (lp); l2; l2 = dtlink(lp,l2)) {
	    cp = (channel*)l2;
	    if (cp->cnt)
		add_edges_in_G(cp);
   	}
    }
}

static segment*
next_seg(segment* seg, int dir)
{
    assert(seg);
    if (!dir)
        return(seg->prev);
    else
        return(seg->next);
}

/* propagate_prec propagates the precedence relationship along 
 * a series of parallel segments on 2 edges
 */
static int
propagate_prec(segment* seg, int prec, int hops, int dir)
{
    int x;
    int ans=prec;
    segment* next;
    segment* current;

    current = seg;
    for(x=1;x<=hops;x++) {
	next = next_seg(current, dir);
	if(!current->isVert) {
	    if(next->comm_coord==current->p.p1) {
		if(current->l1==B_UP) ans *= -1;
	    }
	    else {
		if(current->l2==B_DOWN) ans *= -1;
	    }
	}
	else {
	    if(next->comm_coord==current->p.p1) {
		if(current->l1==B_RIGHT) ans *= -1;
	    }
	    else {
		if(current->l2==B_LEFT) ans *= -1;
	    }
	}
	current = next;
    }
    return(ans);
}

static int
is_parallel(segment* s1, segment* s2)
{
    assert (s1->comm_coord==s2->comm_coord);
    return ((s1->p.p1==s2->p.p1)&&
            (s1->p.p2==s2->p.p2)&&
            (s1->l1==s2->l1)&&
            (s1->l2==s2->l2));
}

/* decide_point returns the number of hops needed in the given directions 
 * along the 2 edges to get to a deciding point (or NODES) and also puts 
 * into prec the appropriate dependency (follows same convention as seg_cmp)
 */
static pair 
decide_point(segment* si, segment* sj, int dir1, int dir2)
{
    int prec, ans = 0, temp;
    pair ret;
    segment* np1;
    segment* np2;
    
    while ((np1 = next_seg(si,dir1)) && (np2 = next_seg(sj,dir2)) &&
	is_parallel(np1, np2)) {
	ans++;
	si = np1;
	sj = np2;
    }
    if (!np1)
	prec = 0;
    else if (!np2)
	assert(0); /* FIXME */
    else {
	temp = seg_cmp(np1, np2);
	prec = propagate_prec(np1, temp, ans+1, 1-dir1);
    }
		
    ret.a = ans;
    ret.b = prec;
    return(ret);
}

/* sets the edges for a series of parallel segments along two edges starting 
 * from segment i, segment j. It is assumed that the edge should be from 
 * segment i to segment j - the dependency is appropriately propagated
 */
static void
set_parallel_edges (segment* seg1, segment* seg2, int dir1, int dir2, int hops,
    maze* mp)
{
    int x;
    channel* chan;
    channel* nchan;
    segment* prev1;
    segment* prev2;

    if (seg1->isVert)
	chan = chanSearch(mp->vchans, seg1);
    else
	chan = chanSearch(mp->hchans, seg1);
    insert_edge(chan->G, seg1->ind_no, seg2->ind_no);

    for (x=1;x<=hops;x++) {
	prev1 = next_seg(seg1, dir1);
	prev2 = next_seg(seg2, dir2);
	if(!seg1->isVert) {
	    nchan = chanSearch(mp->vchans, prev1);
	    if(prev1->comm_coord==seg1->p.p1) {
		if(seg1->l1==B_UP) {
		    if(edge_exists(chan->G, seg1->ind_no, seg2->ind_no))
			insert_edge(nchan->G, prev2->ind_no, prev1->ind_no);
		    else
			insert_edge(nchan->G, prev1->ind_no, prev2->ind_no);
		}
		else {
		    if(edge_exists(chan->G, seg1->ind_no, seg2->ind_no))
			insert_edge(nchan->G, prev1->ind_no, prev2->ind_no);
		    else
			insert_edge(nchan->G, prev2->ind_no, prev1->ind_no);
		}
	    }
	    else {
		if(seg1->l2==B_UP) {
		    if(edge_exists(chan->G, seg1->ind_no, seg2->ind_no))
			insert_edge(nchan->G,prev1->ind_no, prev2->ind_no);
		    else
			insert_edge(nchan->G,prev2->ind_no, prev1->ind_no);
		}
		else {
		    if(edge_exists(chan->G, seg1->ind_no, seg2->ind_no))
			insert_edge(nchan->G, prev2->ind_no, prev1->ind_no);
		    else
			insert_edge(nchan->G, prev1->ind_no, prev2->ind_no);
		}
	    }
	}
	else {
	    nchan = chanSearch(mp->hchans, prev1);
	    if(prev1->comm_coord==seg1->p.p1) {
		if(seg1->l1==B_LEFT) {
		    if(edge_exists(chan->G, seg1->ind_no, seg2->ind_no))
			insert_edge(nchan->G, prev1->ind_no, prev2->ind_no);
		    else
			insert_edge(nchan->G, prev2->ind_no, prev1->ind_no);
		}
		else {
		    if(edge_exists(chan->G, seg1->ind_no, seg2->ind_no))
			insert_edge(nchan->G, prev2->ind_no, prev1->ind_no);
		    else
			insert_edge(nchan->G, prev1->ind_no, prev2->ind_no);
		}
	    }
	    else {
		if(seg1->l2==B_LEFT) {
		    if(edge_exists(chan->G, seg1->ind_no, seg2->ind_no))
			insert_edge(nchan->G, prev2->ind_no, prev1->ind_no);
		    else
			insert_edge(nchan->G, prev1->ind_no, prev2->ind_no);
		}
		else {
		    if(edge_exists(chan->G, seg1->ind_no, seg2->ind_no))
			insert_edge(nchan->G, prev1->ind_no, prev2->ind_no);
		    else
			insert_edge(nchan->G, prev2->ind_no, prev1->ind_no);
		}
	    }
	}
	chan = nchan;
	seg1 = prev1;
	seg2 = prev2;
    }
}

/* removes the edge between segments after the resolution of a conflict
 */
static void
removeEdge(segment* seg1, segment* seg2, int dir, maze* mp)
{
    segment* ptr1;
    segment* ptr2;
    channel* chan;

    ptr1 = seg1;
    ptr2 = seg2;
    while(is_parallel(ptr1, ptr2)) {
	ptr1 = next_seg(ptr1, 1);
	ptr2 = next_seg(ptr2, dir);
    }
    if(ptr1->isVert)
	chan = chanSearch(mp->vchans, ptr1);
    else
	chan = chanSearch(mp->hchans, ptr1);
    remove_redge (chan->G, ptr1->ind_no, ptr2->ind_no);
}

static void
addPEdges (channel* cp, maze* mp)
{
    int i,j;
    /* dir[1,2] are used to figure out whether we should use prev 
     * pointers or next pointers -- 0 : decrease, 1 : increase
     */
    int dir;
    /* number of hops along the route to get to the deciding points */
    pair hops;
    /* precedences of the deciding points : same convention as 
     * seg_cmp function 
     */
    int prec1, prec2;
    pair p;
    rawgraph* G = cp->G;
    segment** segs = cp->seg_list;

    for(i=0;i+1<cp->cnt;i++) {
	for(j=i+1;j<cp->cnt;j++) {
	    if (!edge_exists(G,i,j) && !edge_exists(G,j,i)) {
		if (is_parallel(segs[i], segs[j])) {
		/* get_directions */
		    if(segs[i]->prev==0) {
			if(segs[j]->prev==0)
			    dir = 0;
			else
			    dir = 1;
		    }
		    else if(segs[j]->prev==0) {
			dir = 1;
		    }
		    else {
			if(segs[i]->prev->comm_coord==segs[j]->prev->comm_coord)
			    dir = 0;
			else
			    dir = 1;
		    }

		    p = decide_point(segs[i], segs[j], 0, dir);
		    hops.a = p.a;
		    prec1 = p.b;
		    p = decide_point(segs[i], segs[j], 1, 1-dir);
		    hops.b = p.a;
		    prec2 = p.b;

		    switch(prec1) {
		    case -1 :
			set_parallel_edges (segs[j], segs[i], dir, 0, hops.a, mp);
			set_parallel_edges (segs[j], segs[i], 1-dir, 1, hops.b, mp);
			if(prec2==1)
			    removeEdge (segs[i], segs[j], 1-dir, mp);
			break;
		    case 0 :
			switch(prec2) {
			case -1:
			    set_parallel_edges (segs[j], segs[i], dir, 0, hops.a, mp);
			    set_parallel_edges (segs[j], segs[i], 1-dir, 1, hops.b, mp);
			    break;
			case 0 :
			    set_parallel_edges (segs[i], segs[j], 0, dir, hops.a, mp);
			    set_parallel_edges (segs[i], segs[j], 1, 1-dir, hops.b, mp);
			    break;
			case 1:
			    set_parallel_edges (segs[i], segs[j], 0, dir, hops.a, mp);
			    set_parallel_edges (segs[i], segs[j], 1, 1-dir, hops.b, mp);
			    break;
			}
			break;
		    case 1 :
			set_parallel_edges (segs[i], segs[j], 0, dir, hops.a, mp);
			set_parallel_edges (segs[i], segs[j], 1, 1-dir, hops.b, mp);
			if(prec2==-1)
			    removeEdge (segs[i], segs[j], 1-dir, mp);
			break;
		    }
		}
	    }
	}
    }
}

static void 
add_p_edges (Dt_t* chans, maze* mp)
{
    Dt_t* lp;
    Dtlink_t* l1;
    Dtlink_t* l2;

    for (l1 = dtflatten (chans); l1; l1 = dtlink(chans,l1)) {
	lp = ((chanItem*)l1)->chans;
	for (l2 = dtflatten (lp); l2; l2 = dtlink(lp,l2)) {
	    addPEdges ((channel*)l2, mp);
   	}
    }
}

static void
assignTracks (int nrtes, route* route_list, maze* mp)
{
    /* Create the graphs for each channel */
    create_graphs(mp->hchans);
    create_graphs(mp->vchans);

    /* add edges between non-parallel segments */
    add_np_edges(mp->hchans);
    add_np_edges(mp->vchans);

    /* add edges between parallel segments + remove appropriate edges */
    add_p_edges(mp->hchans, mp);
    add_p_edges(mp->vchans, mp);

    /* Assign the tracks after a top sort */
    assignTrackNo (mp->hchans);
    assignTrackNo (mp->vchans);
}

static double
vtrack (segment* seg, maze* m)
{
  channel* chp = chanSearch(m->vchans, seg);
  double f = ((double)seg->track_no)/(chp->cnt+1); 
  double left = chp->cp->bb.LL.x;
  double right = chp->cp->bb.UR.x;
  return left + f*(right-left);
}

static int
htrack (segment* seg, maze* m)
{
  channel* chp = chanSearch(m->hchans, seg);
  double f = 1.0 - ((double)seg->track_no)/(chp->cnt+1); 
  double lo = chp->cp->bb.LL.y;
  double hi = chp->cp->bb.UR.y;
  return lo + f*(hi-lo);
}

static pointf
addPoints(pointf p0, pointf p1)
{
    p0.x += p1.x;
    p0.y += p1.y;
    return p0;
}

static void
attachOrthoEdges (Agraph_t* g, maze* mp, int n_edges, route* route_list, splineInfo *sinfo, epair_t es[], int doLbls)
{
    int irte = 0;
    int i, ipt, npts;
    pointf* ispline = 0;
    int splsz = 0;
    pointf p, p1, q1;
    route rte;
    segment* seg;
    Agedge_t* e;
    textlabel_t* lbl;

    for (; irte < n_edges; irte++) {
	e = es[irte].e;
	p1 = addPoints(ND_coord(agtail(e)), ED_tail_port(e).p);
	q1 = addPoints(ND_coord(aghead(e)), ED_head_port(e).p);

	rte = route_list[irte];
	npts = 1 + 3*rte.n;
	if (npts > splsz) {
		if (ispline) free (ispline);
		ispline = N_GNEW(npts, pointf);
		splsz = npts;
	}
	    
	seg = rte.segs;
	if (seg->isVert) {
		p.x = vtrack(seg, mp);
		p.y = p1.y;
	}
	else {
		p.y = htrack(seg, mp);
		p.x = p1.x;
	}
	ispline[0] = ispline[1] = p;
	ipt = 2;

	for (i = 1;i<rte.n;i++) {
		seg = rte.segs+i;
		if (seg->isVert)
		    p.x = vtrack(seg, mp);
		else
		    p.y = htrack(seg, mp);
		ispline[ipt+2] = ispline[ipt+1] = ispline[ipt] = p;
		ipt += 3;
	}

	if (seg->isVert) {
		p.x = vtrack(seg, mp);
		p.y = q1.y;
	}
	else {
		p.y = htrack(seg, mp);
		p.x = q1.x;
	}
	ispline[ipt] = ispline[ipt+1] = p;
	if (Verbose > 1)
	    fprintf(stderr, "ortho %s %s\n", agnameof(agtail(e)),agnameof(aghead(e)));
	clip_and_install(e, aghead(e), ispline, npts, sinfo);
	if (doLbls && (lbl = ED_label(e)) && !lbl->set)
	    addEdgeLabels(g, e, p1, q1);
    }
    free(ispline);
}

static int
edgeLen (Agedge_t* e)
{
    pointf p = ND_coord(agtail(e));
    pointf q = ND_coord(aghead(e));
    return (int)DIST2(p,q);
}

static int edgecmp(epair_t* e0, epair_t* e1)
{
    return (e0->d - e1->d);
}

static boolean spline_merge(node_t * n)
{
    return FALSE;
}

static boolean swap_ends_p(edge_t * e)
{
    return FALSE;
}

static splineInfo sinfo = { swap_ends_p, spline_merge, 1, 1 };

/* orthoEdges:
 * For edges without position information, construct an orthogonal routing.
 * If doLbls is true, use edge label info when available to guide routing, 
 * and set label pos for those edges for which this info is not available.
 */
void
orthoEdges (Agraph_t* g, int doLbls)
{
    sgraph* sg;
    maze* mp;
    int n_edges;
    route* route_list;
    int i, gstart;
    Agnode_t* n;
    Agedge_t* e;
    snode* sn;
    snode* dn;
    epair_t* es = N_GNEW(agnedges(g), epair_t);
    cell* start;
    cell* dest;
    PointSet* ps;
    textlabel_t* lbl;

    if (Concentrate) 
	ps = newPS();

#ifdef DEBUG
    {
	char* s = agget(g, "odb");
        char c;
	odb_flags = 0;
	if (s && (*s != '\0')) {
	    while ((c = *s++)) {
		switch (c) {
		case 'c' :
		    odb_flags |= ODB_CHANG;     // emit channel graph 
		    break;
		case 'i' :
		    odb_flags |= (ODB_SGRAPH|ODB_IGRAPH);  // emit search graphs
		    break;
		case 'm' :
		    odb_flags |= ODB_MAZE;      // emit maze
		    break;
		case 'r' :
		    odb_flags |= ODB_ROUTE;     // emit routes in maze
		    break;
		case 's' :
		    odb_flags |= ODB_SGRAPH;    // emit search graph 
		    break;
		}
	    }
	}
    }
#endif
    if (doLbls) {
	agerr(AGWARN, "Orthogonal edges do not currently handle edge labels. Try using xlabels.\n");
	doLbls = 0;
    }
    mp = mkMaze (g, doLbls);
    sg = mp->sg;
#ifdef DEBUG
    if (odb_flags & ODB_SGRAPH) emitSearchGraph (stderr, sg);
#endif

    /* store edges to be routed in es, along with their lengths */
    n_edges = 0;
    for (n = agfstnode (g); n; n = agnxtnode(g, n)) {
        for (e = agfstout(g, n); e; e = agnxtout(g,e)) {
	    if ((Nop == 2) && ED_spl(e)) continue;
	    if (Concentrate) {
		int ti = AGSEQ(agtail(e));
		int hi = AGSEQ(aghead(e));
		if (ti <= hi) {
		    if (isInPS (ps,ti,hi)) continue;
		    else addPS (ps,ti,hi);
		}
		else {
		    if (isInPS (ps,hi,ti)) continue;
		    else addPS (ps,hi,ti);
		}
	    }
	    es[n_edges].e = e;
	    es[n_edges].d = edgeLen (e);
	    n_edges++;
	}
    }

    route_list = N_NEW (n_edges, route);

    qsort((char *)es, n_edges, sizeof(epair_t), (qsort_cmpf) edgecmp);

    gstart = sg->nnodes;
    PQgen (sg->nnodes+2);
    sn = &sg->nodes[gstart];
    dn = &sg->nodes[gstart+1];
    for (i = 0; i < n_edges; i++) {
#ifdef DEBUG
	if ((i > 0) && (odb_flags & ODB_IGRAPH)) emitSearchGraph (stderr, sg);
#endif
	e = es[i].e;
        start = CELL(agtail(e));
        dest = CELL(aghead(e));

	if (doLbls && (lbl = ED_label(e)) && lbl->set) {
	}
	else {
	    if (start == dest)
		addLoop (sg, start, dn, sn);
	    else {
       		addNodeEdges (sg, dest, dn);
		addNodeEdges (sg, start, sn);
	    }
       	    if (shortPath (sg, dn, sn)) goto orthofinish;
	}
	    
       	route_list[i] = convertSPtoRoute(sg, sn, dn);
       	reset (sg);
    }
    PQfree ();

    mp->hchans = extractHChans (mp);
    mp->vchans = extractVChans (mp);
    assignSegs (n_edges, route_list, mp);
    if (setjmp(jbuf))
	goto orthofinish;
    assignTracks (n_edges, route_list, mp);
#ifdef DEBUG
    if (odb_flags & ODB_ROUTE) emitGraph (stderr, mp, n_edges, route_list, es);
#endif
    attachOrthoEdges (g, mp, n_edges, route_list, &sinfo, es, doLbls);

orthofinish:
    if (Concentrate)
	freePS (ps);

    for (i=0; i < n_edges; i++)
	free (route_list[i].segs);
    free (route_list);
    freeMaze (mp);
}

#ifdef DEBUG
#include <arith.h>
/* #include <values.h> */
#define TRANS 10

static char* prolog2 =
"%%!PS-Adobe-2.0\n\
%%%%BoundingBox: (atend)\n\
/point {\n\
  /Y exch def\n\
  /X exch def\n\
  newpath\n\
  X Y 3 0 360 arc fill\n\
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
} def\n\
/node {\n\
 /u exch def\n\
 /r exch def\n\
 /d exch def\n\
 /l exch def\n\
 newpath l d moveto\n\
 r d lineto r u lineto l u lineto\n\
 closepath fill\n\
} def\n\
\n";

static char* epilog2 =
"showpage\n\
%%%%Trailer\n\
%%%%BoundingBox: %d %d %d %d\n";

static point
coordOf (cell* cp, snode* np)
{
    point p;
    if (cp->sides[M_TOP] == np) {
	p.x = (cp->bb.LL.x + cp->bb.UR.x)/2;
	p.y = cp->bb.UR.y;
    }
    else if (cp->sides[M_BOTTOM] == np) {
	p.x = (cp->bb.LL.x + cp->bb.UR.x)/2;
	p.y = cp->bb.LL.y;
    }
    else if (cp->sides[M_LEFT] == np) {
	p.y = (cp->bb.LL.y + cp->bb.UR.y)/2;
	p.x = cp->bb.LL.x;
    }
    else if (cp->sides[M_RIGHT] == np) {
	p.y = (cp->bb.LL.y + cp->bb.UR.y)/2;
	p.x = cp->bb.UR.x;
    }
    return p;
}

static boxf
emitEdge (FILE* fp, Agedge_t* e, route rte, maze* m, int ix, boxf bb)
{
    int i, x, y;
    boxf n = CELL(agtail(e))->bb;
    segment* seg = rte.segs;
    if (seg->isVert) {
	x = vtrack(seg, m);
	y = (n.UR.y + n.LL.y)/2;
    }
    else {
	y = htrack(seg, m);
	x = (n.UR.x + n.LL.x)/2;
    }
    bb.LL.x = MIN(bb.LL.x, SC*x);
    bb.LL.y = MIN(bb.LL.y, SC*y);
    bb.UR.x = MAX(bb.UR.x, SC*x);
    bb.UR.y = MAX(bb.UR.y, SC*y);
    fprintf (fp, "newpath %d %d moveto\n", SC*x, SC*y);

    for (i = 1;i<rte.n;i++) {
	seg = rte.segs+i;
	if (seg->isVert) {
	    x = vtrack(seg, m);
	}
	else {
	    y = htrack(seg, m);
	}
	bb.LL.x = MIN(bb.LL.x, SC*x);
	bb.LL.y = MIN(bb.LL.y, SC*y);
	bb.UR.x = MAX(bb.UR.x, SC*x);
	bb.UR.y = MAX(bb.UR.y, SC*y);
	fprintf (fp, "%d %d lineto\n", SC*x, SC*y);
    }

    n = CELL(aghead(e))->bb;
    if (seg->isVert) {
	x = vtrack(seg, m);
	y = (n.UR.y + n.LL.y)/2;
    }
    else {
	y = htrack(seg, m);
	x = (n.LL.x + n.UR.x)/2;
    }
    bb.LL.x = MIN(bb.LL.x, SC*x);
    bb.LL.y = MIN(bb.LL.y, SC*y);
    bb.UR.x = MAX(bb.UR.x, SC*x);
    bb.UR.y = MAX(bb.UR.y, SC*y);
    fprintf (fp, "%d %d lineto stroke\n", SC*x, SC*y);

    return bb;
}

static void
emitSearchGraph (FILE* fp, sgraph* sg)
{
    cell* cp;
    snode* np;
    sedge* ep;
    point p;
    int i;
    fputs ("graph G {\n", fp);
    fputs (" node[shape=point]\n", fp);
    for (i = 0; i < sg->nnodes; i++) {
	np = sg->nodes+i;
	cp = np->cells[0];
	if (cp == np->cells[1]) {
	    pointf pf = midPt (cp);
	    p.x = pf.x;
	    p.y = pf.y;
	}
	else {
	    if (IsNode(cp)) cp = np->cells[1];
	    p = coordOf (cp, np);
	}
	fprintf (fp, "  %d [pos=\"%d,%d\"]\n", i, p.x, p.y);
    }
    for (i = 0; i < sg->nedges; i++) {
	ep = sg->edges+i;
	fprintf (fp, "  %d -- %d[len=\"%f\"]\n", ep->v1, ep->v2, ep->weight);
    }
    fputs ("}\n", fp);
}

static void
emitGraph (FILE* fp, maze* mp, int n_edges, route* route_list, epair_t es[])
{
    int i;
    boxf bb, absbb;
    box bbox;

    absbb.LL.x = absbb.LL.y = MAXDOUBLE;
    absbb.UR.x = absbb.UR.y = -MAXDOUBLE;

    fprintf (fp, "%s", prolog2);
    fprintf (fp, "%d %d translate\n", TRANS, TRANS);

    fputs ("0 0 1 setrgbcolor\n", fp);
    for (i = 0; i < mp->ngcells; i++) {
      bb = mp->gcells[i].bb;
      fprintf (fp, "%f %f %f %f node\n", bb.LL.x, bb.LL.y, bb.UR.x, bb.UR.y);
    }

    for (i = 0; i < n_edges; i++) {
	absbb = emitEdge (fp, es[i].e, route_list[i], mp, i, absbb);
    }
    
    fputs ("0.8 0.8 0.8 setrgbcolor\n", fp);
    for (i = 0; i < mp->ncells; i++) {
      bb = mp->cells[i].bb;
      fprintf (fp, "%f %f %f %f cell\n", bb.LL.x, bb.LL.y, bb.UR.x, bb.UR.y);
      absbb.LL.x = MIN(absbb.LL.x, bb.LL.x);
      absbb.LL.y = MIN(absbb.LL.y, bb.LL.y);
      absbb.UR.x = MAX(absbb.UR.x, bb.UR.x);
      absbb.UR.y = MAX(absbb.UR.y, bb.UR.y);
    }

    bbox.LL.x = absbb.LL.x + TRANS;
    bbox.LL.y = absbb.LL.y + TRANS;
    bbox.UR.x = absbb.UR.x + TRANS;
    bbox.UR.y = absbb.UR.y + TRANS;
    fprintf (fp, epilog2, bbox.LL.x, bbox.LL.y,  bbox.UR.x, bbox.UR.y);
}
#endif
