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


#include    "circular.h"
#include    "blocktree.h"
#include    "circpos.h"
#include	<string.h>

#define		MINDIST			1.0

/* initGraphAttrs:
 * Set attributes based on original root graph.
 * This is obtained by taking a node of g, finding its node
 * in the original graph, and finding that node's graph.
 */
static void initGraphAttrs(Agraph_t * g, circ_state * state)
{
    static Agraph_t *rootg;
    static attrsym_t *N_artpos;
    static attrsym_t *N_root;
    static double min_dist;
    static char *rootname;
    Agraph_t *rg;
    attrsym_t *G_mindist;
    node_t *n = agfstnode(g);

    rg = agraphof(ORIGN(n));
    if (rg != rootg) {		/* new root graph */
	state->blockCount = 0;
	rootg = rg;
#ifndef WITH_CGRAPH
	G_mindist = agfindattr(rootg, "mindist");
#else /* WITH_CGRAPH */
	G_mindist = agattr(rootg,AGRAPH, "mindist", NULL);
#endif /* WITH_CGRAPH */
	min_dist = late_double(rootg, G_mindist, MINDIST, 0.0);
#ifndef WITH_CGRAPH
	N_artpos = agfindattr(rootg->proto->n, "articulation_pos");
	N_root = agfindattr(rootg->proto->n, "root");
#else /* WITH_CGRAPH */
	N_artpos = agattr(rootg,AGNODE, "articulation_pos", NULL);
	N_root = agattr(rootg,AGNODE, "root", NULL);
#endif /* WITH_CGRAPH */
	rootname = agget(rootg, "root");
    }
    initBlocklist(&state->bl);
    state->orderCount = 1;
    state->min_dist = min_dist;
    state->N_artpos = N_artpos;
    state->N_root = N_root;
    state->rootname = rootname;
}

/* cleanup:
 * We need to cleanup objects created in initGraphAttrs
 * and all blocks. All graph objects are components of the
 * initial derived graph and will be freed when it is closed. 
 */
static void cleanup(block_t * root, circ_state * sp)
{
    freeBlocktree(root);
}

static block_t*
createOneBlock(Agraph_t * g, circ_state * state)
{
    Agraph_t *subg;
    char name[SMALLBUF];
    block_t *bp;
    Agnode_t* n;

    sprintf(name, "_block_%d", state->blockCount++);
#ifdef WITH_CGRAPH
    subg = agsubg(g, name, 1);
#else
    subg = agsubg(g, name);
#endif
    bp = mkBlock(subg);

    for (n = agfstnode(g); n; n = agnxtnode(g,n)) {
#ifdef WITH_CGRAPH
	agsubnode(bp->sub_graph, n, 1);
#else
	aginsert(bp->sub_graph, n);
#endif
	BLOCK(n) = bp;
    }

    return bp;
}

/* circularLayout:
 * Do circular layout of g.
 * Assume g is strict.
 * g is a "connected" component of the derived graph of the
 * original graph.
 * We make state static so that it keeps a record of block numbers used
 * in a graph; it gets reset when a new root graph is used.
 */
void circularLayout(Agraph_t * g, Agraph_t* realg)
{
    block_t *root;
    static circ_state state;

    if (agnnodes(g) == 1) {
	Agnode_t *n = agfstnode(g);
	ND_pos(n)[0] = 0;
	ND_pos(n)[1] = 0;
	return;
    }

    initGraphAttrs(g, &state);

    if (mapbool(agget(realg, "oneblock")))
	root = createOneBlock(g, &state);
    else
	root = createBlocktree(g, &state);
    circPos(g, root, &state);

    cleanup(root, &state);
}

#ifdef DEBUG
void prGraph(Agraph_t * g)
{
    Agnode_t *n;
    Agedge_t *e;

    fprintf(stderr, "%s\n", agnameof(g));
    for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	fprintf(stderr, "%s (%x)\n", agnameof(n), (unsigned int) n);
	for (e = agfstout(g, n); e; e = agnxtout(g, e)) {
	    fprintf(stderr, "%s", agnameof(n));
	    fprintf(stderr, " -- %s (%x)\n", agnameof(aghead(e)),
		    (unsigned int) e);
	}
    }
}

cdata *cvt(Agnode_t * n)
{
    return DATA(n);
}

void prData(Agnode_t * n, int pass)
{
    char *pname;
    char *bname;
    char *tname;
    char *name1;
    char *name2;
    int dist1, dist2;

    if (PARENT(n))
	pname = agnameof(PARENT(n));
    else
	pname = "<P0>";
    if (BLOCK(n))
	bname = agnameof(BLOCK(n)->sub_graph);
    else
	pname = "<B0>";
    fprintf(stderr, "%s: %x %s %s ", agnameof(n), FLAGS(n), pname, bname);
    switch (pass) {
    case 0:
	fprintf(stderr, "%d %d\n", VAL(n), LOWVAL(n));
	break;
    case 1:
	if (TPARENT(n))
	    tname = agnameof(TPARENT(n));
	else
	    tname = "<ROOT>";
	dist1 = DISTONE(n);
	if (dist1 > 0)
	    name1 = agnameof(LEAFONE(n));
	else
	    name1 = "<null>";
	dist2 = DISTTWO(n);
	if (dist2 > 0)
	    name2 = agnameof(LEAFTWO(n));
	else
	    name2 = "<null>";
	fprintf(stderr, "%s %s %d %s %d\n", tname, name1, dist1, name2,
		dist2);
	break;
    default:
	fprintf(stderr, "%d\n", POSITION(n));
	break;
    }
}
#endif
