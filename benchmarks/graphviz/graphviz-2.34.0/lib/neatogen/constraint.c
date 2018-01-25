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

#include "neato.h"
#include "adjust.h"

/* For precision, scale up before algorithms, then scale down */
#define SCALE 10   
#define SCALE2 (SCALE/2)

typedef struct nitem {
    Dtlink_t link;
    int val;
    point pos;			/* position for sorting */
    node_t *np;			/* base node */
    node_t *cnode;		/* corresponding node in constraint graph */
    node_t *vnode;		/* corresponding node in neighbor graph */
    box bb;
} nitem;

typedef int (*distfn) (box *, box *);
typedef int (*intersectfn) (nitem *, nitem *);

static int cmpitem(Dt_t * d, int *p1, int *p2, Dtdisc_t * disc)
{
    NOTUSED(d);
    NOTUSED(disc);

    return (*p1 - *p2);
}

static Dtdisc_t constr = {
    offsetof(nitem, val),
    sizeof(int),
    offsetof(nitem, link),
    NIL(Dtmake_f),
    NIL(Dtfree_f),
    (Dtcompar_f) cmpitem,
    NIL(Dthash_f),
    NIL(Dtmemory_f),
    NIL(Dtevent_f)
};

static int distY(box * b1, box * b2)
{
    return ((b1->UR.y - b1->LL.y) + (b2->UR.y - b2->LL.y)) / 2;
}

static int distX(box * b1, box * b2)
{
    return ((b1->UR.x - b1->LL.x) + (b2->UR.x - b2->LL.x)) / 2;
}

/* intersectX0:
 * Return true if boxes could overlap if shifted in y but don't,
 * or if actually overlap and an y move is smallest to remove overlap.
 * Otherwise (no x overlap or a x move is smaller), return false.
 * Assume q pos to above of p pos.
 */
static int intersectX0(nitem * p, nitem * q)
{
    int xdelta, ydelta;
    int v = ((p->bb.LL.x <= q->bb.UR.x) && (q->bb.LL.x <= p->bb.UR.x));
    if (v == 0)  /* no x overlap */
	return 0;
    if (p->bb.UR.y < q->bb.LL.y) /* but boxes don't really overlap */
	return 1;
    ydelta = distY(&p->bb,&q->bb) - (q->pos.y - p->pos.y);
    if (q->pos.x >= p->pos.x) 
	xdelta = distX(&p->bb,&q->bb) - (q->pos.x - p->pos.x); 
    else
	xdelta = distX(&p->bb,&q->bb) - (p->pos.x - q->pos.x); 
    return (ydelta <= xdelta);
}

/* intersectY0:
 * Return true if boxes could overlap if shifted in x but don't,
 * or if actually overlap and an x move is smallest to remove overlap.
 * Otherwise (no y overlap or a y move is smaller), return false.
 * Assume q pos to right of p pos.
 */
static int intersectY0(nitem * p, nitem * q)
{
    int xdelta, ydelta;
    int v = ((p->bb.LL.y <= q->bb.UR.y) && (q->bb.LL.y <= p->bb.UR.y));
    if (v == 0)  /* no y overlap */
	return 0;
    if (p->bb.UR.x < q->bb.LL.x) /* but boxes don't really overlap */
	return 1;
    xdelta = distX(&p->bb,&q->bb) - (q->pos.x - p->pos.x);
    if (q->pos.y >= p->pos.y) 
	ydelta = distY(&p->bb,&q->bb) - (q->pos.y - p->pos.y); 
    else
	ydelta = distY(&p->bb,&q->bb) - (p->pos.y - q->pos.y); 
    return (xdelta <= ydelta);
}

static int intersectY(nitem * p, nitem * q)
{
    return ((p->bb.LL.y <= q->bb.UR.y) && (q->bb.LL.y <= p->bb.UR.y));
}

static int intersectX(nitem * p, nitem * q)
{
    return ((p->bb.LL.x <= q->bb.UR.x) && (q->bb.LL.x <= p->bb.UR.x));
}

/* mapGraphs:
 */
static void mapGraphs(graph_t * g, graph_t * cg, distfn dist)
{
    node_t *n;
    edge_t *e;
    edge_t *ce;
    node_t *t;
    node_t *h;
    nitem *tp;
    nitem *hp;
    int delta;

    for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	tp = (nitem *) ND_alg(n);
	t = tp->cnode;
	for (e = agfstout(g, n); e; e = agnxtout(g, e)) {
	    hp = (nitem *) ND_alg(aghead(e));
	    delta = dist(&tp->bb, &hp->bb);
	    h = hp->cnode;
#ifndef WITH_CGRAPH
	    ce = agedge(cg, t, h);
#else
	    ce = agedge(cg, t, h, NULL, 1);
	    agbindrec(ce, "Agedgeinfo_t", sizeof(Agedgeinfo_t), TRUE);
#endif
	    ED_weight(ce) = 1;
	    if (ED_minlen(ce) < delta) {
		if (ED_minlen(ce) == 0.0) {
		    elist_append(ce, ND_out(t));
		    elist_append(ce, ND_in(h));
		}
		ED_minlen(ce) = delta;
	    }
	}
    }
}

#ifdef DEBUG
static int
indegree (graph_t * g, node_t *n)
{
  edge_t *e;
  int cnt = 0;
  for (e = agfstin(g,n); e; e = agnxtin(g,e)) cnt++;
  return cnt; 
}

static int
outdegree (graph_t * g, node_t *n)
{
  edge_t *e;
  int cnt = 0;
  for (e = agfstout(g,n); e; e = agnxtout(g,e)) cnt++;
  return cnt; 
}

static void
validate(graph_t * g)
{
    node_t *n;
    edge_t *e;
    int    i, cnt;
  
    cnt = 0;
    for (n = GD_nlist(g);n; n = ND_next(n)) {
      assert(outdegree(g,n) == ND_out(n).size);
      for (i = 0; (e = ND_out(n).list[i]); i++) {
        assert(agtail(e) == n);
        assert( e == agfindedge(g, n, aghead(e))); 
      }
      assert(indegree(g,n) == ND_in(n).size);
      for (i = 0; (e = ND_in(n).list[i]); i++) {
        assert(aghead(e) == n);
        assert( e == agfindedge(g, agtail(e), n)); 
      }
      cnt++;
    }

    assert (agnnodes(g) == cnt); 
}
#endif

#ifdef OLD
static node_t *newNode(graph_t * g)
{
    static int id = 0;
    char buf[100];

    sprintf(buf, "n%d", id++);
    return agnode(g, buf);
}
#endif

/* mkNConstraintG:
 * Similar to mkConstraintG, except it doesn't enforce orthogonal
 * ordering. If there is overlap, as defined by intersect, the
 * nodes will kept/pushed apart in the current order. If not, no
 * constraint is enforced. If a constraint edge is added, and it
 * corresponds to a real edge, we increase the weight in an attempt
 * to keep the resulting shift short. 
 */
static graph_t *mkNConstraintG(graph_t * g, Dt_t * list,
			      intersectfn intersect, distfn dist)
{
    nitem *p;
    nitem *nxp;
    node_t *n;
    edge_t *e;
    node_t *lastn = NULL;
#ifndef WITH_CGRAPH
    graph_t *cg = agopen("cg", AGDIGRAPHSTRICT);
#else
    graph_t *cg = agopen("cg", Agstrictdirected, NIL(Agdisc_t *));
    agbindrec(cg, "Agraphinfo_t", sizeof(Agraphinfo_t), TRUE);  // graph custom data
#endif

    for (p = (nitem *) dtflatten(list); p;
	 p = (nitem *) dtlink(list, (Dtlink_t *) p)) {
#ifndef WITH_CGRAPH
	n = agnode(cg, agnameof(p->np));	/* FIX */
#else
	n = agnode(cg, agnameof(p->np), 1);      /* FIX */
	agbindrec(n, "Agnodeinfo_t", sizeof(Agnodeinfo_t), TRUE); //node custom data
#endif
	ND_alg(n) = p;
	p->cnode = n;
	alloc_elist(0, ND_in(n));
	alloc_elist(0, ND_out(n));
	if (lastn) {
	    ND_next(lastn) = n;
	    lastn = n;
	} else {
	    lastn = GD_nlist(cg) = n;
	}
    }
    for (p = (nitem *) dtflatten(list); p;
	 p = (nitem *) dtlink(list, (Dtlink_t *) p)) {
	for (nxp = (nitem *) dtlink(link, (Dtlink_t *) p); nxp;
	     nxp = (nitem *) dtlink(list, (Dtlink_t *) nxp)) {
	    e = NULL;
	    if (intersect(p, nxp)) {
	        double delta = dist(&p->bb, &nxp->bb);
#ifndef WITH_CGRAPH
	        e = agedge(cg, p->cnode, nxp->cnode);
#else
	        e = agedge(cg, p->cnode, nxp->cnode, NULL, 1);
		agbindrec(e, "Agedgeinfo_t", sizeof(Agedgeinfo_t), TRUE);   // edge custom data
#endif
		assert (delta <= 0xFFFF);
		ED_minlen(e) = delta;
		ED_weight(e) = 1;
	    }
	    if (e && agfindedge(g,p->np, nxp->np)) {
		ED_weight(e) = 100;
            }
#if 0
	    if (agfindedge(g,p->np, nxp->np)) {
		if (e == NULL)
	            e = agedge(cg, p->cnode, nxp->cnode);
		ED_weight(e) = 100;
                /* If minlen < SCALE, the nodes can't conflict or there's
                 * an overlap but it will be removed in the orthogonal pass.
                 * So we just keep the node's basically where they are.
                 */
		if (SCALE > ED_minlen(e))
		    ED_minlen(e) = SCALE;
	    }
#endif
	}
    }
   
    for (p = (nitem *) dtflatten(list); p;
	 p = (nitem *) dtlink(list, (Dtlink_t *) p)) {
	n = p->cnode;
	for (e = agfstout(cg,n); e; e = agnxtout(cg,e)) {
	    elist_append(e, ND_out(n));
	    elist_append(e, ND_in(aghead(e)));
	}
    }

    /* We could remove redundant constraints here. However, the cost of doing 
     * this may be a good deal more than the time saved in network simplex. 
     * Also, if the graph is changed, the ND_in and ND_out data has to be 
     * updated.
     */
    return cg;
}
/* mkConstraintG:
 */
static graph_t *mkConstraintG(graph_t * g, Dt_t * list,
			      intersectfn intersect, distfn dist)
{
    nitem *p;
    nitem *nxt = NULL;
    nitem *nxp;
    graph_t *vg;
    node_t *prev = NULL;
    node_t *root = NULL;
    node_t *n = NULL;
    edge_t *e;
    int lcnt, cnt;
    int oldval = -INT_MAX;
#ifdef OLD
    double root_val;
#endif
    node_t *lastn = NULL;
#ifndef WITH_CGRAPH
    graph_t *cg = agopen("cg", AGDIGRAPHSTRICT);
#else
    graph_t *cg = agopen("cg", Agstrictdirected, NIL(Agdisc_t *));
    agbindrec(cg, "Agraphinfo_t", sizeof(Agraphinfo_t), TRUE);  // graph custom data
#endif

    /* count distinct nodes */
    cnt = 0;
    for (p = (nitem *) dtflatten(list); p;
	 p = (nitem *) dtlink(list, (Dtlink_t *) p)) {
	if (oldval != p->val) {
	    oldval = p->val;
	    cnt++;
	}
    }

    /* construct basic chain to enforce left to right order */
    oldval = -INT_MAX;
    lcnt = 0;
    for (p = (nitem *) dtflatten(list); p;
	 p = (nitem *) dtlink(list, (Dtlink_t *) p)) {
	if (oldval != p->val) {
	    oldval = p->val;
	    /* n = newNode (cg); */
#ifndef WITH_CGRAPH
	    n = agnode(cg, agnameof(p->np));	/* FIX */
#else
	    n = agnode(cg, agnameof(p->np), 1);	/* FIX */
	    agbindrec(n, "Agnodeinfo_t", sizeof(Agnodeinfo_t), TRUE); //node custom data
#endif
	    ND_alg(n) = p;
	    if (root) {
		ND_next(lastn) = n;
		lastn = n;
	    } else {
		root = n;
#ifdef OLD
		root_val = p->val;
#endif
		lastn = GD_nlist(cg) = n;
	    }
	    alloc_elist(lcnt, ND_in(n));
	    if (prev) {
		if (prev == root)
		    alloc_elist(2 * (cnt - 1), ND_out(prev));
		else
		    alloc_elist(cnt - lcnt - 1, ND_out(prev));
#ifndef WITH_CGRAPH
		e = agedge(cg, prev, n);
#else
		e = agedge(cg, prev, n, NULL, 1);
		agbindrec(e, "Agedgeinfo_t", sizeof(Agedgeinfo_t), TRUE);   // edge custom data
#endif
		ED_minlen(e) = SCALE;
		ED_weight(e) = 1;
		elist_append(e, ND_out(prev));
		elist_append(e, ND_in(n));
	    }
	    lcnt++;
	    prev = n;
	}
	p->cnode = n;
    }
    alloc_elist(0, ND_out(prev));

    /* add immediate right neighbor constraints
     * Construct visibility graph, then perform transitive reduction.
     * Remaining outedges are immediate right neighbors.
     * FIX: Incremental algorithm to construct trans. reduction?
     */
#ifndef WITH_CGRAPH
    vg = agopen("vg", AGDIGRAPHSTRICT);
#else
    vg = agopen("vg", Agstrictdirected, NIL(Agdisc_t *));
#endif
    for (p = (nitem *) dtflatten(list); p;
	 p = (nitem *) dtlink(list, (Dtlink_t *) p)) {
#ifndef WITH_CGRAPH
	n = agnode(vg, agnameof(p->np));     /* FIX */
#else
	n = agnode(vg, agnameof(p->np), 1);  /* FIX */
	agbindrec(n, "Agnodeinfo_t", sizeof(Agnodeinfo_t), TRUE);  //node custom data
#endif
	p->vnode = n;
	ND_alg(n) = p;
    }
    oldval = -INT_MAX;
    for (p = (nitem *) dtflatten(list); p;
	 p = (nitem *) dtlink(list, (Dtlink_t *) p)) {
	if (oldval != p->val) {	/* new pos: reset nxt */
	    oldval = p->val;
	    for (nxt = (nitem *) dtlink(link, (Dtlink_t *) p); nxt;
		 nxt = (nitem *) dtlink(list, (Dtlink_t *) nxt)) {
		if (nxt->val != oldval)
		    break;
	    }
	    if (!nxt)
		break;
	}
	for (nxp = nxt; nxp;
	     nxp = (nitem *) dtlink(list, (Dtlink_t *) nxp)) {
	    if (intersect(p, nxp))
#ifndef WITH_CGRAPH
		agedge(vg, p->vnode, nxp->vnode);
#else
		agedge(vg, p->vnode, nxp->vnode, NULL, 1);
#endif
	}
    }

    /* Remove redundant constraints here. However, the cost of doing this
     * may be a good deal more than the time saved in network simplex. Also,
     * if the graph is changed, the ND_in and ND_out data has to be updated.
     */
    mapGraphs(vg, cg, dist);
    agclose(vg);

    /* add dummy constraints for absolute values and initial positions */
#ifdef OLD
    for (n = agfstnode(cg); n; n = agnxtnode(cg, n)) {
	node_t *vn;		/* slack node for absolute value */
	node_t *an;		/* node representing original position */

	p = (nitem *) ND_alg(n);
	if ((n == root) || (!p))
	    continue;
	vn = newNode(cg);
	ND_next(lastn) = vn;
	lastn = vn;
	alloc_elist(0, ND_out(vn));
	alloc_elist(2, ND_in(vn));
	an = newNode(cg);
	ND_next(lastn) = an;
	lastn = an;
	alloc_elist(1, ND_in(an));
	alloc_elist(1, ND_out(an));

#ifndef WITH_CGRAPH
	e = agedge(cg, root, an);
#else
	e = agedge(cg, root, an, 1);
#endif
	ED_minlen(e) = p->val - root_val;
	elist_append(e, ND_out(root));
	elist_append(e, ND_in(an));

#ifndef WITH_CGRAPH
	e = agedge(cg, an, vn);
#else
	e = agedge(cg, an, vn, 1);
#endif
	elist_append(e, ND_out(an));
	elist_append(e, ND_in(vn));

#ifndef WITH_CGRAPH
	e = agedge(cg, n, vn);
#else
	e = agedge(cg, n, vn, 1);
#endif
	elist_append(e, ND_out(n));
	elist_append(e, ND_in(vn));
    }
#endif  /* OLD */

    return cg;
}

static void closeGraph(graph_t * cg)
{
    node_t *n;
    for (n = agfstnode(cg); n; n = agnxtnode(cg, n)) {
	free_list(ND_in(n));
	free_list(ND_out(n));
    }
    agclose(cg);
}

/* constrainX:
 * Create the X constrains and solve. We use a linear objective function
 * (absolute values rather than squares), so we can reuse network simplex.
 * The constraints are encoded as a dag with edges having a minimum length.
 */
static void constrainX(graph_t* g, nitem* nlist, int nnodes, intersectfn ifn,
                       int ortho)
{
    Dt_t *list = dtopen(&constr, Dtobag);
    nitem *p = nlist;
    graph_t *cg;
    int i;

    for (i = 0; i < nnodes; i++) {
	p->val = p->pos.x;
	dtinsert(list, p);
	p++;
    }
    if (ortho)
	cg = mkConstraintG(g, list, ifn, distX);
    else
	cg = mkNConstraintG(g, list, ifn, distX);
    rank(cg, 2, INT_MAX);

    p = nlist;
    for (i = 0; i < nnodes; i++) {
	int newpos, oldpos, delta;
	oldpos = p->pos.x;
	newpos = ND_rank(p->cnode);
	delta = newpos - oldpos;
	p->pos.x = newpos;
	p->bb.LL.x += delta;
	p->bb.UR.x += delta;
	p++;
    }

    closeGraph(cg);
    dtclose(list);
}

/* constrainY:
 * See constrainX.
 */
static void constrainY(graph_t* g, nitem* nlist, int nnodes, intersectfn ifn,
                       int ortho)
{
    Dt_t *list = dtopen(&constr, Dtobag);
    nitem *p = nlist;
    graph_t *cg;
    int i;

    for (i = 0; i < nnodes; i++) {
	p->val = p->pos.y;
	dtinsert(list, p);
	p++;
    }
    if (ortho)
	cg = mkConstraintG(g, list, ifn, distY);
    else
	cg = mkNConstraintG(g, list, ifn, distY);
    rank(cg, 2, INT_MAX);
#ifdef DEBUG
    {
#ifndef WITH_CGRAPH
	Agsym_t *mlsym = agedgeattr(cg, "minlen", "");
	Agsym_t *rksym = agnodeattr(cg, "rank", "");
#else
	Agsym_t *mlsym = agattr(cg, AGEDGE, "minlen", "");
	Agsym_t *rksym = agattr(cg, AGNODE, "rank", "");
#endif
	char buf[100];
	node_t *n;
	edge_t *e;
	for (n = agfstnode(cg); n; n = agnxtnode(cg, n)) {
	    sprintf(buf, "%d", ND_rank(n));
#ifndef WITH_CGRAPH
	    agxset(n, rksym->index, buf);
#else
	    agxset(n, rksym, buf);
#endif
	    for (e = agfstedge(cg, n); e; e = agnxtedge(cg, e, n)) {
		sprintf(buf, "%d", ED_minlen(e));
#ifndef WITH_CGRAPH
		agxset(e, mlsym->index, buf);
#else
		agxset(e, mlsym, buf);
#endif
	    }
	}
    }
#endif

    p = nlist;
    for (i = 0; i < nnodes; i++) {
	int newpos, oldpos, delta;
	oldpos = p->pos.y;
	newpos = ND_rank(p->cnode);
	delta = newpos - oldpos;
	p->pos.y = newpos;
	p->bb.LL.y += delta;
	p->bb.UR.y += delta;
	p++;
    }

    closeGraph(cg);
    dtclose(list);
}

#define overlap(pb,qb) \
  ((pb.LL.x <= qb.UR.x) && (qb.LL.x <= pb.UR.x) && \
          (pb.LL.y <= qb.UR.y) && (qb.LL.y <= pb.UR.y))

/* overlaps:
 */
static int overlaps(nitem * p, int cnt)
{
    int i, j;
    nitem *pi = p;
    nitem *pj;

    for (i = 0; i < cnt - 1; i++) {
	pj = pi + 1;
	for (j = i + 1; j < cnt; j++) {
	    if (overlap(pi->bb, pj->bb))
		return 1;
	    pj++;
	}
	pi++;
    }
    return 0;
}

/* initItem:
 */
static void initItem(node_t * n, nitem * p, expand_t margin)
{
    int x = POINTS(SCALE * ND_pos(n)[0]);
    int y = POINTS(SCALE * ND_pos(n)[1]);
    int w2, h2;
    box b;

    if (margin.doAdd) {
	w2 = SCALE * (POINTS(ND_width(n)/2.0) + margin.x);
	h2 = SCALE * (POINTS(ND_height(n)/2.0) + margin.y);
    }
    else {
	w2 = POINTS(margin.x * SCALE2 * ND_width(n));
	h2 = POINTS(margin.y * SCALE2 * ND_height(n));
    }

    b.LL.x = x - w2;
    b.LL.y = y - h2;
    b.UR.x = x + w2;
    b.UR.y = y + h2;

    p->pos.x = x;
    p->pos.y = y;
    p->np = n;
    p->bb = b;
}

/* cAdjust:
 * Use optimization to remove overlaps.
 * Modifications;
 *  - do y;x then x;y and use the better one
 *  - for all overlaps (or if overlap with leftmost nodes), add a constraint;
 *     constraint could move both x and y away, or the smallest, or some
 *     mixture.
 *  - follow by a scale down using actual shapes
 * We use an optimization based on Marriott, Stuckey, Tam and He,
 * "Removing Node Overlapping in Graph Layout Using Constrained Optimization",
 * Constraints,8(2):143--172, 2003.
 * We solve 2 constraint problem, one in X, one in Y. In each dimension,
 * we require relative positions to remain the same. That is, if two nodes
 * have the same x originally, they have the same x at the end, and if one
 * node is to the left of another, it remains to the left. In addition, if
 * two nodes could overlap by moving their X coordinates, we insert a constraint * to keep the two nodes sufficiently apart. Similarly, for Y.
 * 
 * mode = AM_ORTHOXY => first X, then Y
 * mode = AM_ORTHOYX => first Y, then X
 * mode = AM_ORTHO   => first X, then Y
 * mode = AM_ORTHO_YX   => first Y, then X
 * In the last 2 cases, relax the constraints as follows: during the X pass,
 * if two nodes actually intersect and a smaller move in the Y direction
 * will remove the overlap, we don't force the nodes apart in the X direction,
 * but leave it for the Y pass to remove any remaining overlaps. Without this,
 * the X pass will remove all overlaps, and the Y pass only compresses in the
 * Y direction, causing a skewing of the aspect ratio.
 * 
 * mode = AM_ORTHOXY => first X, then Y
 * mode = AM_ORTHOYX => first Y, then X
 * mode = AM_ORTHO   => first X, then Y
 * mode = AM_ORTHO_YX   => first Y, then X
 */
int cAdjust(graph_t * g, int mode)
{
    expand_t margin;
    int ret, i, nnodes = agnnodes(g);
    nitem *nlist = N_GNEW(nnodes, nitem);
    nitem *p = nlist;
    node_t *n;

    margin = sepFactor (g);

    for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	initItem(n, p, margin);
	p++;
    }

    if (overlaps(nlist, nnodes)) {
	point pt;

	switch ((adjust_mode)mode) {
	case AM_ORTHOXY:
	    constrainX(g, nlist, nnodes, intersectY, 1);
	    constrainY(g, nlist, nnodes, intersectX, 1);
	    break;
	case AM_ORTHOYX:
	    constrainY(g, nlist, nnodes, intersectX, 1);
	    constrainX(g, nlist, nnodes, intersectY, 1);
	    break;
	case AM_ORTHO :
	    constrainX(g, nlist, nnodes, intersectY0, 1);
	    constrainY(g, nlist, nnodes, intersectX, 1);
	case AM_ORTHO_YX :
	    constrainY(g, nlist, nnodes, intersectX0, 1);
	    constrainX(g, nlist, nnodes, intersectY, 1);
	case AM_PORTHOXY:
	    constrainX(g, nlist, nnodes, intersectY, 0);
	    constrainY(g, nlist, nnodes, intersectX, 0);
	    break;
	case AM_PORTHOYX:
	    constrainY(g, nlist, nnodes, intersectX, 0);
	    constrainX(g, nlist, nnodes, intersectY, 0);
	    break;
	case AM_PORTHO_YX :
	    constrainY(g, nlist, nnodes, intersectX0, 0);
	    constrainX(g, nlist, nnodes, intersectY, 0);
	    break;
	case AM_PORTHO :
	default :
	    constrainX(g, nlist, nnodes, intersectY0, 0);
	    constrainY(g, nlist, nnodes, intersectX, 0);
	    break;
	}
	p = nlist;
	for (i = 0; i < nnodes; i++) {
	    n = p->np;
	    pt = p->pos;
	    ND_pos(n)[0] = PS2INCH(pt.x) / SCALE;
	    ND_pos(n)[1] = PS2INCH(pt.y) / SCALE;
	    p++;
	}
	ret = 1;
    }
    else ret = 0;
    free(nlist);
    return ret;
}

typedef struct {
    pointf pos;			/* position for sorting */
    boxf bb;
    double wd2;
    double ht2;
    node_t *np;
} info;

typedef int (*sortfn_t) (const void *, const void *);

static int sortf(pointf * p, pointf * q)
{
    if (p->x < q->x)
	return -1;
    else if (p->x > q->x)
	return 1;
    else if (p->y < q->y)
	return -1;
    else if (p->y > q->y)
	return 1;
    else
	return 0;
}

static double compress(info * nl, int nn)
{
    info *p = nl;
    info *q;
    int i, j;
    double s, sc = 0;
    pointf pt;

    for (i = 0; i < nn; i++) {
	q = p + 1;
	for (j = i + 1; j < nn; j++) {
	    if (overlap(p->bb, q->bb))
		return 0;
	    if (p->pos.x == q->pos.x)
		pt.x = HUGE_VAL;
	    else {
		pt.x = (p->wd2 + q->wd2) / fabs(p->pos.x - q->pos.x);
	    }
	    if (p->pos.y == q->pos.y)
		pt.y = HUGE_VAL;
	    else {
		pt.y = (p->ht2 + q->ht2) / fabs(p->pos.y - q->pos.y);
	    }
	    if (pt.y < pt.x)
		s = pt.y;
	    else
		s = pt.x;
	    if (s > sc)
		sc = s;
	    q++;
	}
	p++;
    }
    return sc;
}

static pointf *mkOverlapSet(info * nl, int nn, int *cntp)
{
    info *p = nl;
    info *q;
    int sz = nn;
    pointf *S = N_GNEW(sz + 1, pointf);
    int i, j;
    int cnt = 0;

    for (i = 0; i < nn; i++) {
	q = p + 1;
	for (j = i + 1; j < nn; j++) {
	    if (overlap(p->bb, q->bb)) {
		pointf pt;
		if (cnt == sz) {
		    sz += nn;
		    S = RALLOC(sz + 1, S, pointf);
		}
		if (p->pos.x == q->pos.x)
		    pt.x = HUGE_VAL;
		else {
		    pt.x = (p->wd2 + q->wd2) / fabs(p->pos.x - q->pos.x);
		    if (pt.x < 1)
			pt.x = 1;
		}
		if (p->pos.y == q->pos.y)
		    pt.y = HUGE_VAL;
		else {
		    pt.y = (p->ht2 + q->ht2) / fabs(p->pos.y - q->pos.y);
		    if (pt.y < 1)
			pt.y = 1;
		}
		S[++cnt] = pt;
	    }
	    q++;
	}
	p++;
    }

    S = RALLOC(cnt + 1, S, pointf);
    *cntp = cnt;
    return S;
}

static pointf computeScaleXY(pointf * aarr, int m)
{
    pointf *barr;
    double cost, bestcost;
    int k, best = 0;
    pointf scale;

    aarr[0].x = 1;
    aarr[0].y = HUGE_VAL;
    qsort(aarr + 1, m, sizeof(pointf), (sortfn_t) sortf);

    barr = N_GNEW(m + 1, pointf);
    barr[m].x = aarr[m].x;
    barr[m].y = 1;
    for (k = m - 1; k >= 0; k--) {
	barr[k].x = aarr[k].x;
	barr[k].y = MAX(aarr[k + 1].y, barr[k + 1].y);
    }

    bestcost = HUGE_VAL;
    for (k = 0; k <= m; k++) {
	cost = barr[k].x * barr[k].y;
	if (cost < bestcost) {
	    bestcost = cost;
	    best = k;
	}
    }
    assert(bestcost < HUGE_VAL);
    scale.x = barr[best].x;
    scale.y = barr[best].y;

    return scale;
}

/* computeScale:
 * For each (x,y) in aarr, scale has to be bigger than the smallest one.
 * So, the scale is the max min.
 */
static double computeScale(pointf * aarr, int m)
{
    int i;
    double sc = 0;
    double v;
    pointf p;

    aarr++;
    for (i = 1; i <= m; i++) {
	p = *aarr++;
	v = MIN(p.x, p.y);
	if (v > sc)
	    sc = v;
    }
    return sc;
}

/* scAdjust:
 * Scale the layout.
 * equal > 0  => scale uniformly in x and y to remove overlaps
 * equal = 0  => scale separately in x and y to remove overlaps
 * equal < 0  => scale down uniformly in x and y to remove excess space
 * The last assumes there are no overlaps at present.
 * Based on Marriott, Stuckey, Tam and He,
 * "Removing Node Overlapping in Graph Layout Using Constrained Optimization",
 * Constraints,8(2):143--172, 2003.
 */
int scAdjust(graph_t * g, int equal)
{
    int nnodes = agnnodes(g);
    info *nlist = N_GNEW(nnodes, info);
    info *p = nlist;
    node_t *n;
    pointf s;
    int i;
    expand_t margin;
    pointf *aarr;
    int m;

    margin = sepFactor (g);
    if (margin.doAdd) {
	/* we use inches below */
	margin.x = PS2INCH(margin.x);
	margin.y = PS2INCH(margin.y);
    }

    for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	double w2, h2;
	if (margin.doAdd) {
	    w2 = (ND_width(n) / 2.0) + margin.x;
	    h2 = (ND_height(n) / 2.0) + margin.y;
	}
	else {
	    w2 = margin.x * ND_width(n) / 2.0;
	    h2 = margin.y * ND_height(n) / 2.0;
	}
	p->pos.x = ND_pos(n)[0];
	p->pos.y = ND_pos(n)[1];
	p->bb.LL.x = p->pos.x - w2;
	p->bb.LL.y = p->pos.y - h2;
	p->bb.UR.x = p->pos.x + w2;
	p->bb.UR.y = p->pos.y + h2;
	p->wd2 = w2;
	p->ht2 = h2;
	p->np = n;
	p++;
    }

    if (equal < 0) {
	s.x = s.y = compress(nlist, nnodes);
	if (s.x == 0) {		/* overlaps exist */
	    free(nlist);
	    return 0;
	}
	fprintf(stderr, "compress %g \n", s.x);
    } else {
	aarr = mkOverlapSet(nlist, nnodes, &m);

	if (m == 0) {		/* no overlaps */
	    free(aarr);
	    free(nlist);
	    return 0;
	}

	if (equal) {
	    s.x = s.y = computeScale(aarr, m);
	} else {
	    s = computeScaleXY(aarr, m);
	}
	free(aarr);
    }

    p = nlist;
    for (i = 0; i < nnodes; i++) {
	ND_pos(p->np)[0] = s.x * p->pos.x;
	ND_pos(p->np)[1] = s.y * p->pos.y;
	p++;
    }

    free(nlist);
    return 1;
}
