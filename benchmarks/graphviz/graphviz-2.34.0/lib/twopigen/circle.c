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


#include    "circle.h"
#include    <ctype.h>
#include    <stdlib.h>
#define DEF_RANKSEP 1.00
#define UNSET 10.00

/* dfs to set distance from a particular leaf.
 * Note that termination is implicit in the test
 * for reduced number of steps. Proof?
 */
static void setNStepsToLeaf(Agraph_t * g, Agnode_t * n, Agnode_t * prev)
{
    Agnode_t *next;
    Agedge_t *ep;

    int nsteps = SLEAF(n) + 1;

    for (ep = agfstedge(g, n); ep; ep = agnxtedge(g, ep, n)) {
	if ((next = agtail(ep)) == n)
	    next = aghead(ep);

	if (prev == next)
	    continue;

	if (nsteps < SLEAF(next)) {	/* handles loops and multiedges */
	    SLEAF(next) = nsteps;
	    setNStepsToLeaf(g, next, n);
	}
    }
}

/* isLeaf:
 * Return true if n is a leaf node.
 */
static int isLeaf(Agraph_t * g, Agnode_t * n)
{
    Agedge_t *ep;
    Agnode_t *neighp = 0;
    Agnode_t *np;

    for (ep = agfstedge(g, n); ep; ep = agnxtedge(g, ep, n)) {
	if ((np = agtail(ep)) == n)
	    np = aghead(ep);
	if (n == np)
	    continue;		/* loop */
	if (neighp) {
	    if (neighp != np)
		return 0;	/* two different neighbors */
	} else
	    neighp = np;
    }
    return 1;
}

static void initLayout(Agraph_t * g)
{
    Agnode_t *n;
    int nnodes = agnnodes(g);
    int INF = nnodes * nnodes;

    for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	/* STSIZE(n) = 0; */
	/* NCHILD(n) = 0; */
	SCENTER(n) = INF;
	THETA(n) = UNSET;	/* marks theta as unset, since 0 <= theta <= 2PI */
	if (isLeaf(g, n))
	    SLEAF(n) = 0;
	else
	    SLEAF(n) = INF;
    }
}

/*
 * Working recursively in from each leaf node (ie, each node
 * with nStepsToLeaf == 0; see initLayout), set the
 * minimum value of nStepsToLeaf for each node.  Using
 * that information, assign some node to be the centerNode.
*/
static Agnode_t *findCenterNode(Agraph_t * g)
{
    Agnode_t *n;
    Agnode_t *center = NULL;
    int maxNStepsToLeaf = 0;

    /* With just 1 or 2 nodes, return anything. */
    if (agnnodes(g) <= 2)
	return (agfstnode(g));

    /* dfs from each leaf node */
    for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	if (SLEAF(n) == 0)
	    setNStepsToLeaf(g, n, 0);
    }

    for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	if (SLEAF(n) > maxNStepsToLeaf) {
	    maxNStepsToLeaf = SLEAF(n);
	    center = n;
	}
    }
    return center;
}

#if 0
/* dfs to set distance from center
 * Note that termination is implicit in the test
 * for reduced number of steps. Proof?
 */
static void setNStepsToCenter(Agraph_t * g, Agnode_t * n, Agnode_t * prev)
{
    Agnode_t *next;
    Agedge_t *ep;
    int nsteps = SCENTER(n) + 1;

    for (ep = agfstedge(g, n); ep; ep = agnxtedge(g, ep, n)) {
	if ((next = agtail(ep)) == n)
	    next = aghead(ep);

	if (prev == next)
	    continue;

	if (nsteps < SCENTER(next)) {	/* handles loops and multiedges */
	    SCENTER(next) = nsteps;
	    if (SPARENT(next))
		NCHILD(SPARENT(next))--;
	    SPARENT(next) = n;
	    NCHILD(n)++;
	    setNStepsToCenter(g, next, n);
	}
    }
}
#endif

typedef struct item_s {
    void* p;
    struct item_s* s;
} item_t;
typedef struct {
    item_t* head;
    item_t* tail;
} queue;
static void push(queue* q, void* p)
{
    item_t* ip = NEW(item_t);
    ip->p = p;
    if (q->tail) {  /* non-empty q */
	q->tail->s = ip;
	q->tail = ip;
    }
    else
	q->tail = q->head = ip;
}
static void* pull(queue* q)
{
    item_t* ip;
    void* p;
    if ((ip = q->head)) {
	p = ip->p;
	q->head = ip->s;
	free (ip);
	if (!q->head)
	    q->tail = NULL; 
	return p;
    }
    else
	return NULL;
}

/* bfs to create tree structure */
static void setNStepsToCenter(Agraph_t * g, Agnode_t * n)
{
    Agnode_t *next;
    Agedge_t *ep;
    Agsym_t* wt = agfindedgeattr(g,"weight");
    queue qd;
    queue* q = &qd;

    qd.head = qd.tail = NULL;
    push(q,n);
    while ((n = (Agnode_t*)pull(q))) {
	int nsteps = SCENTER(n) + 1;
	for (ep = agfstedge(g, n); ep; ep = agnxtedge(g, ep, n)) {
	    if (wt && streq(ag_xget(ep,wt),"0")) continue;
	    if ((next = agtail(ep)) == n)
		next = aghead(ep);
	    if (nsteps < SCENTER(next)) {
		SCENTER(next) = nsteps;
		SPARENT(next) = n;
		NCHILD(n)++;
		push (q, next);
	    }
	}
    }
}

/*
 * Work out from the center and determine the value of
 * nStepsToCenter and parent node for each node.
 * Return -1 if some node was not reached.
 */
static int setParentNodes(Agraph_t * sg, Agnode_t * center)
{
    Agnode_t *n;
    int maxn = 0;
    int unset = SCENTER(center);

    SCENTER(center) = 0;
    SPARENT(center) = 0;
    setNStepsToCenter(sg, center);

    /* find the maximum number of steps from the center */
    for (n = agfstnode(sg); n; n = agnxtnode(sg, n)) {
	if (SCENTER(n) == unset) {
	    return -1;
	}
	else if (SCENTER(n) > maxn) {
	    maxn = SCENTER(n);
	}
    }
    return maxn;
}

/* Sets each node's subtreeSize, which counts the number of 
 * leaves in subtree rooted at the node.
 * At present, this is done bottom-up.
 */
static void setSubtreeSize(Agraph_t * g)
{
    Agnode_t *n;
    Agnode_t *parent;

    for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	if (NCHILD(n) > 0)
	    continue;
	STSIZE(n)++;
	parent = SPARENT(n);
	while (parent) {
	    STSIZE(parent)++;
	    parent = SPARENT(parent);
	}
    }
}

static void setChildSubtreeSpans(Agraph_t * g, Agnode_t * n)
{
    Agedge_t *ep;
    Agnode_t *next;
    double ratio;

    ratio = SPAN(n) / STSIZE(n);
    for (ep = agfstedge(g, n); ep; ep = agnxtedge(g, ep, n)) {
	if ((next = agtail(ep)) == n)
	    next = aghead(ep);
	if (SPARENT(next) != n)
	    continue;		/* handles loops */

	if (SPAN(next) != 0.0)
	    continue;		/* multiedges */
	(SPAN(next) = ratio * STSIZE(next));

	if (NCHILD(next) > 0) {
	    setChildSubtreeSpans(g, next);
	}
    }
}

static void setSubtreeSpans(Agraph_t * sg, Agnode_t * center)
{
    SPAN(center) = 2 * M_PI;
    setChildSubtreeSpans(sg, center);
}

 /* Set the node positions for the 2nd and later rings. */
static void setChildPositions(Agraph_t * sg, Agnode_t * n)
{
    Agnode_t *next;
    Agedge_t *ep;
    double theta;		/* theta is the lower boundary radius of the fan */

    if (SPARENT(n) == 0)	/* center */
	theta = 0;
    else
	theta = THETA(n) - SPAN(n) / 2;

    for (ep = agfstedge(sg, n); ep; ep = agnxtedge(sg, ep, n)) {
	if ((next = agtail(ep)) == n)
	    next = aghead(ep);
	if (SPARENT(next) != n)
	    continue;		/* handles loops */
	if (THETA(next) != UNSET)
	    continue;		/* handles multiedges */

	THETA(next) = theta + SPAN(next) / 2.0;
	theta += SPAN(next);

	if (NCHILD(next) > 0)
	    setChildPositions(sg, next);
    }
}

static void setPositions(Agraph_t * sg, Agnode_t * center)
{
    THETA(center) = 0;
    setChildPositions(sg, center);
}

/* getRankseps:
 * Return array of doubles of size maxrank+1 containing the radius of each
 * rank.  Position 0 always contains 0. Use the colon-separated list of 
 * doubles provided by ranksep to get the deltas for each additional rank.
 * If not enough values are provided, the last value is repeated.
 * If the ranksep attribute is not provided, use DEF_RANKSEP for all values. 
 */ 
static double*
getRankseps (Agraph_t* g, int maxrank)
{
    char *p;
    char *endp;
    char c;
    int i, rk = 1;
    double* ranks = N_NEW(maxrank+1, double);
    double xf = 0, delx, d;

    if ((p = late_string(g, agfindgraphattr(g->root, "ranksep"), NULL))) {
	while ((rk <= maxrank) && ((d = strtod (p, &endp)) > 0)) {
	    delx = MAX(d, MIN_RANKSEP);
	    xf += delx;
	    ranks[rk++] = xf;
	    p = endp;
	    while ((c = *p) && (isspace(c) || (c == ':')))
		p++;
	}
    }
    else {
	delx = DEF_RANKSEP;
    }

    for (i = rk; i <= maxrank; i++) {
	xf += delx;
	ranks[i] = xf;
    }

    return ranks;
}

static void setAbsolutePos(Agraph_t * g, int maxrank)
{
    Agnode_t *n;
    double hyp;
    double* ranksep;
    int i;

    ranksep = getRankseps (g, maxrank);
    if (Verbose) {
	fputs ("Rank separation = ", stderr);
	for (i = 0; i <= maxrank; i++)
	    fprintf (stderr, "%.03lf ", ranksep[i]);
	fputs ("\n", stderr);
    }

    /* Convert circular to cartesian coordinates */
    for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	hyp = ranksep[SCENTER(n)];
	ND_pos(n)[0] = hyp * cos(THETA(n));
	ND_pos(n)[1] = hyp * sin(THETA(n));
    }
    free (ranksep);
}

#if 0				/* not used */
static void dumpGraph(Agraph_t * g)
{
    Agnode_t *n;
    char *p;

    fprintf(stderr,
	    "     :  leaf  stsz nkids  cntr parent   span  theta\n");
    for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	if (SPARENT(n))
	    p = SPARENT(n)->name;
	else
	    p = "<C>";
	fprintf(stderr, "%4s :%6d%6d%6d%6d%7s%7.3f%7.3f%8.3f%8.3f\n",
		n->name, SLEAF(n), STSIZE(n), NCHILD(n),
		SCENTER(n), p, SPAN(n), THETA(n), ND_pos(n)[0],
		ND_pos(n)[1]);
    }
}
#endif

/* circleLayout:
 *  We assume sg is is connected and non-empty.
 *  Also, if center != 0, we are guaranteed that center is
 *  in the graph.
 */
Agnode_t* circleLayout(Agraph_t * sg, Agnode_t * center)
{
    int maxNStepsToCenter;

    if (agnnodes(sg) == 1) {
	Agnode_t *n = agfstnode(sg);
	ND_pos(n)[0] = 0;
	ND_pos(n)[1] = 0;
	return center;
    }

    initLayout(sg);

    if (!center)
	center = findCenterNode(sg);
    if (Verbose)
	fprintf(stderr, "root = %s\n", agnameof(center));

    maxNStepsToCenter = setParentNodes(sg,center);
    if (maxNStepsToCenter < 0) {
	agerr(AGERR, "twopi: use of weight=0 creates disconnected component.\n");
	return center;
    }

    setSubtreeSize(sg);

    setSubtreeSpans(sg, center);

    setPositions(sg, center);

    setAbsolutePos(sg, maxNStepsToCenter);
    /* dumpGraph (sg); */
    return center;
}
