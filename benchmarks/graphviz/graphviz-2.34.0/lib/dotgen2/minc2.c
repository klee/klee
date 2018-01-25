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

#include "dot2.h"
#include "utils.h"
#include "minc.h"
#include "groups.h"

#define MARK(v)		(ND_mark(v))


/* #include "transpose.h" */
/* #include "stdio.h" */
#ifdef DEBUG_CAIRO
static void drawMatrix(mcGraph * mcG, char *fileName, char *label);
#define DOC_HEIGHT  1600
#define DOC_WIDTH  2400
#endif
#define DEBUG
#ifdef DEBUG
static void dumpGraph (mcGraph* mcG, char* fname);
#endif

static void _transpose(mcGraph * mcG, int reverse);
static void fast_node(graph_t * g, Agnode_t * n);

static int MAGIC_NUMBER = 4;

static int higher_cross(mcNode * v, mcNode * w)
{
    mcEdge *e;
    mcEdge *e2;
    double x1, xx1;
    double x2, xx2;
    int idx, idx2;
    int cnt = 0;
    for (idx = 0; idx < v->high_edgs_cnt; idx++) {
	e = v->high_edgs[idx];
	x1 = e->lowerPort.p.x + e->lowerN->order;
	xx1 = e->higherPort.p.x + e->higherN->order;
	for (idx2 = 0; idx2 < w->high_edgs_cnt; idx2++) {
	    e2 = w->high_edgs[idx2];

	    x2 = e2->lowerPort.p.x + e2->lowerN->order;
	    xx2 = e2->higherPort.p.x + e2->higherN->order;
	    if (((x1 < x2) && (xx1 > xx2)) || ((x2 < x1) && (xx2 > xx1))) {
		if (e->penalty > e2->penalty)
		    cnt = cnt + (int)e->penalty;
		else
		    cnt = cnt + (int)e2->penalty;
	    }
	}
    }
    return cnt;
}

static int lower_cross(mcNode * v, mcNode * w)
{

    mcEdge *e;
    mcEdge *e2;
    double x1, xx1;
    double x2, xx2;
    int idx, idx2;
    int cnt = 0;
    for (idx = 0; idx < v->low_edgs_cnt; idx++) {
	e = v->low_edgs[idx];
	x1 = e->lowerPort.p.x + e->lowerN->order;
	xx1 = e->higherPort.p.x + e->higherN->order;
	for (idx2 = 0; idx2 < w->low_edgs_cnt; idx2++) {
	    e2 = w->low_edgs[idx2];

	    x2 = e2->lowerPort.p.x + e2->lowerN->order;
	    xx2 = e2->higherPort.p.x + e2->higherN->order;
	    if (((x1 < x2) && (xx1 > xx2)) || ((x2 < x1) && (xx2 > xx1))) {
		if (e->penalty > e2->penalty)
		    cnt = cnt +(int) e->penalty;
		else
		    cnt = cnt + (int)e2->penalty;
	    }
	}
    }
    return cnt;
}

static int pair_cross(mcNode * v, mcNode * w, int lowToHigh)
{
    if (lowToHigh)
	return lower_cross(v, w);
    else
	return higher_cross(v, w);
}

/*counts all crossings*/
static int countAllX(mcGraph * mcG)
{
    mcLevel *l;
    mcNode *n;
    mcNode *n2;
    int cnt = 0;
    int id0, id1;
    int idx;

    for (idx = 0; idx < mcG->lvl_cnt; idx++) {
	l = mcG->lvls[idx];
	for (id0 = 0; id0 < l->nodecnt; id0++) {
	    n = l->nodes[id0];
	    for (id1 = 0; id1 < l->nodecnt; id1++) {
		n2 = l->nodes[id1];
		if (id0 != id1) {
		    int aa;
		    aa = pair_cross(n, n2, 1);
		    if (aa > 0) {
			aa = pair_cross(n, n2, 1);
			cnt = cnt + aa;

		    }
		}
	    }
	}
    }
    return cnt / 2;

}

int node_cmp(const mcNode ** a, const mcNode ** b)
{
    float ia;
    float ib;

    ia = (*a)->order;
    ib = (*b)->order;
    if (ia < ib)
	return -1;
    else if (ia > ib)
	return 1;
    else
	return 0;
}

void reOrderLevel(mcLevel * l)
{
    qsort(l->nodes, l->nodecnt, sizeof(mcNode *), (qsort_cmpf) node_cmp);
}

/*returns relative improvement*/

static int transpose(mcLevel * l, int lowToHigh)
{
    mcNode *n;
    mcNode *n2;
    float temp;
    int id;
    int improvement = 0;

     /*DEBUG*/
/*    
    for (n=dtfirst(l->nodes);n;n=dtnext(l->nodes,n))
	printf("%s %f\n",agnameof(n->node),n->order);
*/
	 /*DEBUG*/ for (id = 0; id < l->nodecnt - 1; id++) {

	int c, cc, d, dd;
	n = l->nodes[id];
	n2 = l->nodes[id + 1];
//        printf ("comparing %s %s \n",agnameof(n->node),agnameof(n2->node));
	c = pair_cross(n, n2, lowToHigh);
	d = pair_cross(n, n2, !lowToHigh);
	/*test swap */
	temp = n2->order;
	n2->order = n->order;
	n->order = temp;

	cc = pair_cross(n, n2, lowToHigh);
	dd = pair_cross(n, n2, !lowToHigh);
//      if(((  cc-c) + (dd-d)) >= 0)
	if ((c - cc) <= 0) {
	    temp = n2->order;
	    n2->order = n->order;
	    n->order = temp;
	    improvement = 0;
	} else {
	    improvement = (cc - c);
	    l->nodes[id] = n2;
	    l->nodes[id + 1] = n;
	}
    }
    reOrderLevel(l);
//    printf ("improvement value :%d\n",improvement);
    return improvement;
}

/*
    lowToHigh=1;
    0 0 0 0 0   <-----------l
    XX||XX||

    lowToHigh=0;
    XX||XX||
    0 0 0 0 0   <-----------l
*/
#if 0
static int countx(mcLevel * l, int lowToHigh)
{
    mcNode *n;
    mcNode *n2;
    int cnt = 0;
    int id0, id1;
    for (id0 = 0; id0 < l->nodecnt; id0++) {
	n = l->nodes[id0];
	for (id1 = 0; id1 < l->nodecnt; id1++) {
	    n2 = l->nodes[id1];
	    if (id0 != id1) {
		int aa;
		aa = pair_cross(n, n2, lowToHigh);
		if (aa > 0) {
		    aa = pair_cross(n, n2, lowToHigh);
		    cnt = cnt + aa;

		}
	    }
	}
    }
    return cnt;
}
#endif

/*
    adds maxRank-minRank nodes to given edge
    g:newly created graph, not the original one ,
    e:edge needs to be splitted in new graph
    reference edge in original graph

*/

static void add_node(Agraph_t * g, Agedge_t * e, int labels)
{
    /*find out how the ranks increase ,it may not always be from tail to head! */

    int rankDiff;
    int i;
    Agnode_t *tmpN;
    Agedge_t *tmpE;
    Agnode_t *tail;
    Agnode_t *head;
    Agraph_t *cluster = NULL;
    Agraph_t *tempC1 = NULL;
    Agraph_t *tempC2 = NULL;

    int type = INTNODE;

    head = aghead(e);
    tail = agtail(e);
#if 0
    if ((strcmp(agnameof(head), "T1") == 0)
	&& (strcmp(agnameof(tail), "23") == 0))
	fprintf(stderr, "found \n");
    if ((strcmp(agnameof(head), "23") == 0)
	&& (strcmp(agnameof(tail), "T1") == 0))
	fprintf(stderr, "found \n");
#endif

    /*check whether edge is a cluster */
    tempC1 = MND_highCluster(head);
    tempC2 = MND_highCluster(tail);

    if (tempC1 == tempC2)
	cluster = tempC1;

    rankDiff = MND_rank(head) - MND_rank(tail);
    if ((rankDiff == 1) || (rankDiff == -1))
	return;
    if (rankDiff == 0)
	return;

    if (rankDiff > 0)		/*expected */
	for (i = MND_rank(tail) + 1; i < MND_rank(head); i++) {

	    if (labels) {
		if (type == LBLNODE)
		    type = INTNODE;
		else
		    type = LBLNODE;
	    } else
		type = INTNODE;
	    tmpN = mkMCNode (g, type, (char *) 0);

	    MND_rank(tmpN) = i;
	    MND_type(tmpN) = type;
	    MND_highCluster(tmpN) = cluster;
	    MND_orderr(tmpN) = MND_orderr(tail);
	    /*create internal edge */
	    tmpE = mkMCEdge (g, tail, tmpN, 0, VIRTUAL, MED_orig(e));
	    tail = tmpN;
    } else
	for (i = MND_rank(tail) - 1; i > MND_rank(head); i--) {
	    if (labels) {
		if (type == LBLNODE)
		    type = INTNODE;
		else
		    type = LBLNODE;
	    } else
		type = INTNODE;
	    tmpN = mkMCNode (g, type, (char *) 0);

	    MND_rank(tmpN) = i;
	    MND_type(tmpN) = type;
	    MND_highCluster(tmpN) = cluster;
	    /*create internal edge */
	    tmpE = mkMCEdge (g, tail, tmpN, 0, VIRTUAL, MED_orig(e));
	    tail = tmpN;
	}
    tmpE = mkMCEdge (g, tail, head, 0, VIRTUAL, MED_orig(e));
    /*mark the long edge for deletion */
    MED_deleteFlag(e) = 1;
}

/*
  creates temporary nodes to the graph , 
  if hasLabels is true (1) all ranks are doubled and every edge is splitted to 2
*/
static void addNodes(Agraph_t * g, int hasLabels)
{
    Agnode_t *v;
    Agedge_t *e;
    Agedge_t *ee = NULL;
    Agedge_t **eList = NULL;		/*for technical reasons i add dummy edges after the loop */
    int tempEdgeCnt = 0;
    int id = 0;

    for (v = agfstnode(g); v; v = agnxtnode(g, v)) {
	int type = MND_type(v);
	if (type != STDNODE)
	    continue;
	for (e = agfstout(g, v); e; e = agnxtout(g, e)) {
	    if (MED_type(e) == VIRTUAL)
		continue;
	    eList = RALLOC(tempEdgeCnt + 1, eList, Agedge_t *);
	    eList[tempEdgeCnt] = e;
	    tempEdgeCnt++;
	}
    }
    for (id = 0; id < tempEdgeCnt; id++)
	add_node(g, eList[id], hasLabels);

    /*delete long edges */
    for (v = agfstnode(g); v; v = agnxtnode(g, v)) {
	for (e = agfstout(g, v); e; e = agnxtout(g, e)) {
	    if (ee)
		agdelete(g, ee);
	    if (MED_deleteFlag(e) == 1)
		ee = e;
	    else
		ee = NULL;
	}
    }
}

/*double all ranks if one or more edge have label*/
static void double_ranks(Agraph_t * g)
{
    Agnode_t *v;
    for (v = agfstnode(g); v; v = agnxtnode(g, v))
	MND_rank(v) = MND_rank(v) * 2;
}

/*constructions for data structe */

static void
free_mcGroup (mcGroup* g)
{
    dtclose (g->nodes);
#if 0
    dtclose (g->children);
#endif
    free (g);
}

static void
free_mcEdge (mcEdge* e)
{
    free (e);
}

static void
free_mcNode (mcNode* n)
{
    int i;

    if (n->isLeader)
	free_mcGroup (n->group);
    for (i = 0; i < n->low_edgs_cnt; i++)
	free_mcEdge (n->low_edgs[i]);
    for (i = 0; i < n->high_edgs_cnt; i++)
	free_mcEdge (n->high_edgs[i]);
    free (n->low_edgs);
    free (n->high_edgs);
    free (n->name);
    free (n);
}

static void
free_mcLevel (mcLevel* l)
{
    int i;

    for (i = 0; i < l->nodecnt; i++)
	free_mcNode (l->nodes[i]);
    l->nodecnt++;
    free (l->nodes);
    free (l);
}

static void
free_mcGraph (mcGraph* mcg)
{
    int idx;

    for (idx = 0; idx < mcg->lvl_cnt; idx++)
	free_mcLevel (mcg->lvls[idx]);
 
    free (mcg->lvls);
    free (mcg);
}

static mcNode *initMcNode(Agnode_t * n)
{
    mcNode *rv;
    rv = NEW(mcNode);
    rv->group = NULL;
    if (MND_orderr(n) < 0)
	MND_orderr(n) = 0;
    rv->order = MND_orderr(n);
    rv->w = 0;
    rv->node = n;
    MND_mcn(n) = rv;
    MND_type(n) = STDNODE;
    rv->groupOrder = 0;
    rv->groupOrder = -1;
    rv->isLeader = 0;
    rv->leader = NULL;
    rv->name = strdup(agnameof(n));
    rv->high_edgs_cnt = 0;
    rv->low_edgs = NULL;
    rv->low_edgs_cnt = 0;
    rv->high_edgs = NULL;
    return rv;
}

static mcEdge *initMcEdge(Agedge_t * e)
{
    mcEdge *rv;
    rv = NEW(mcEdge);
    rv->e = e;
    rv->lowerN = NULL;
    rv->higherN = NULL;
    rv->penalty = 1;
    return rv;

}

static mcLevel *initMcLevel(int rank)
{
    mcLevel *rv;
    rv = NEW(mcLevel);
    rv->nodes = NULL;
    rv->nodecnt = 0;
    rv->rank = rank;
    rv->higherEdgeCount = 0;
    rv->lowerEdgeCount = 0;
    rv->highCnt = 1000000000;
    rv->lowCnt = 1000000000;
    return rv;
}

static void addEdgeToNode(mcNode * n, mcEdge * e, int lowEdge)
{
    if (lowEdge) {
	n->low_edgs_cnt++;
	n->low_edgs = RALLOC(n->low_edgs_cnt, n->low_edgs, mcEdge *);
	n->low_edgs[n->low_edgs_cnt - 1] = e;
    } else {
	n->high_edgs_cnt++;
	n->high_edgs = RALLOC(n->high_edgs_cnt, n->high_edgs, mcEdge *);
	n->high_edgs[n->high_edgs_cnt - 1] = e;
    }
}

static void addAdjEdges(Agnode_t * n)
{
    Agnode_t *av;
    Agedge_t *e;
    Agraph_t *g;
    int rank;
    mcNode *mcN;
    mcEdge *mcE;
    mcLevel *l;
    g = agraphof(n);
    rank = MND_rank(n);
    mcN = MND_mcn(n);
    l = mcN->level;

    for (e = agfstout(g, n); e; e = agnxtout(g, e)) {
	av = aghead(e);
	mcE = initMcEdge(e);
	if (MND_rank(av) <= rank) {
	    addEdgeToNode(mcN, mcE, 1);
	    mcE->lowerN = MND_mcn(av);
	    mcE->higherN = mcN;
	    mcE->lowerPort = MED_head_port(e);
	    mcE->higherPort = MED_tail_port(e);

	} else {
	    addEdgeToNode(mcN, mcE, 0);
	    mcE->higherN = MND_mcn(av);
	    mcE->lowerN = mcN;
	    mcE->lowerPort = MED_tail_port(e);
	    mcE->higherPort = MED_head_port(e);
	}
    }
    for (e = agfstin(g, n); e; e = agnxtin(g, e)) {
	av = agtail(e);
	mcE = initMcEdge(e);
	if (MND_rank(av) < rank) {
	    addEdgeToNode(mcN, mcE, 1);
	    mcE->lowerN = MND_mcn(av);
	    mcE->higherN = mcN;
	    mcE->lowerPort = MED_tail_port(e);
	    mcE->higherPort = MED_head_port(e);
	} else {
	    addEdgeToNode(mcN, mcE, 0);
	    mcE->higherN = MND_mcn(av);
	    mcE->lowerN = mcN;
	    mcE->lowerPort = MED_head_port(e);
	    mcE->higherPort = MED_tail_port(e);
	}
    }

    l->higherEdgeCount = mcN->high_edgs_cnt;
    l->lowerEdgeCount = mcN->high_edgs_cnt;

}

#if 0
static void addToMcGroup(mcGroup * group, mcNode * mcN)
{
    if ((!group) || (!mcN))
	return;
    dtappend(group->nodes, mcN);
}
#endif

static mcGraph *newMcGraph(Agraph_t * g)
{
    int maxRank = 0;
    Agnode_t *v;
    mcGraph *rv;
    int ind = 0;

    for (v = agfstnode(g); v; v = agnxtnode(g, v)) {
	if (MND_rank(v) > maxRank)
	    maxRank = MND_rank(v);
    }

    rv = NEW(mcGraph);

    rv->lvls = NULL;
    rv->lvl_cnt = 0;
    rv->crossCnt = 0;
    rv->g = g;

    for (ind = 0; ind <= maxRank; ind++) {
	mcLevel *l = initMcLevel(ind);
	rv->lvl_cnt++;
	rv->lvls = RALLOC(rv->lvl_cnt, rv->lvls, mcLevel*);
	rv->lvls[rv->lvl_cnt - 1] = l;
    }
    return rv;

}
static void addToLevel(mcNode * n, mcGraph * mcG)
{
    int rank = MND_rank(n->node);
    mcLevel *l;
    l = mcG->lvls[rank];
    n->order = l->nodecnt;
    l->nodecnt++;
    l->nodes = RALLOC (l->nodecnt, l->nodes, mcNode*);
    l->nodes[l->nodecnt - 1] = n;
    MND_orderr(n->node) = n->order;
    n->level = l;
}

/*
 creates the data structure for mincross algorithms
 -mcGraph
 --mcLevel
 ---mcNode
 ----mcNode (lower adj. nodes)
 ----mcNode (upper adj. nodes)
*/
static int inEdgeCnt(Agnode_t * n, Agraph_t * g)
{
    Agedge_t *e;
    int cnt = 0;
    for (e = agfstin(g, n); e; e = agnxtin(g, e))
	cnt++;
    return cnt;
}

static int outEdgeCnt(Agnode_t * n, Agraph_t * g)
{
    Agedge_t *e;
    int cnt = 0;
    for (e = agfstout(g, n); e; e = agnxtout(g, e))
	cnt++;
    return cnt;
}

static void install_node(Agnode_t * n, mcGraph * mcg)
{
    static int cnt = 0;
    mcNode *mcn;
    mcn = initMcNode(n);
    addToLevel(mcn, mcg);
    cnt++;
#ifdef _DEBUG
    printf ("%s\n",agnameof(n));

#endif

}

Agnode_t *nodeByName(Agraph_t * g, char *name)
{
    Agnode_t *v;
    for (v = agfstnode(g); v; v = agnxtnode(g, v)) {
	if (strcmp(agnameof(v), name) == 0)
	    return v;
    }
    return NULL;
}


#if 0
static createMatrixWithNames(Agraph_t * g, mcGraph * mcG)
{
    char *nodeNames[] = { "asasdasdsad", "adadsd" };
    int cnt = 23;
    int id;
    Agnode_t *v;
    for (id = 0; id < cnt; id++) {
	v = nodeByName(g, nodeNames[id]);
	if (v)
	    install_node(v, mcG);
    }

    for (id = 0; id < cnt; id++) {
	v = nodeByName(g, nodeNames[id]);
	if (v)
	    addAdjEdges(v);
    }
    id = countAllX(mcG);
}
#endif

static int transposeAllL2H(mcGraph * mcG)
{
    int cnt = 0;
    mcLevel *l;
    int rank;
    int improved = 0;
    int idx;
    for (idx = 0; idx < mcG->lvl_cnt; idx++)
	mcG->lvls[idx]->candidate = 1;
    rank = 0;
    l = mcG->lvls[0];
    idx = 0;
    do {
	if (l->candidate) {
	    int a;

	    a = transpose(l, 1);
	    cnt = cnt + a;
	    if (a < 0) {
		l->candidate = 1;
		if (idx > 0)
		    mcG->lvls[idx - 1]->candidate = 1;
		if (idx < (mcG->lvl_cnt - 1))
		    mcG->lvls[idx + 1]->candidate = 1;

		improved = 1;
	    } else
		l->candidate = 0;

	}
	idx++;
	if (idx == mcG->lvl_cnt) {
	    if (!improved)
		break;

	    idx = 0;
	    improved = 0;
	}
	l = mcG->lvls[idx];

    } while (1);
    return cnt;
}
static int transposeAllH2L(mcGraph * mcG)
{
    int cnt = 0;
    mcLevel *l;
    int rank;
    int improved = 0;
    int idx;
    for (idx = 0; idx < mcG->lvl_cnt; idx++)
	mcG->lvls[idx]->candidate = 1;
    rank = 0;
    l = mcG->lvls[mcG->lvl_cnt - 1];
    idx = mcG->lvl_cnt - 1;
    do {
	if (l->candidate) {
	    int a;

	    a = transpose(l, 0);
	    cnt = cnt + a;
	    if (a < 0) {
		l->candidate = 1;
		if (idx > 0)
		    mcG->lvls[idx - 1]->candidate = 1;
		if (idx < (mcG->lvl_cnt - 1))
		    mcG->lvls[idx + 1]->candidate = 1;

		improved = 1;
	    } else
		l->candidate = 0;

	}
	idx--;
	if (idx == -1) {
	    if (!improved)
		break;

	    idx = mcG->lvl_cnt - 1;
	    improved = 0;
	}
	l = mcG->lvls[idx];

    } while (1);
    return cnt;
}

void decompose(graph_t * g, int pass);
static mcGraph *createMatrixIn(Agraph_t * g)
{
#define ARRAY_SZ 37    
    Agedge_t *e;
    Agnode_t *v;
    Agnode_t *v0;
    nodequeue *b;
    Agnode_t *head;
    mcGraph *mcG;
    int id = 0;
    char* nodes[]= 
{
    "d0",
"e1",
"e2",
"f0",
"f1",
"g0",
"a1",
"b1",
"b2",
"c2",
"c5",
"c8",
"c3",
"c4",
"c6",
"c7",
"d2",
"d4",
"d6",
"d5",
"e3",
"e4",
"e5",
"e6",
"e8",
"e7",
"f4",
"f2",
"f3",
"f5",
"a0",
"b0",
"c0",
"c1",
"d1",
"d3",
"e0" };
    GD_nlist(g)=(node_t*)malloc(sizeof(node_t)*agnnodes(g));
    GD_nlist(g)=NULL;
    for (v = agfstnode(g); v; v = agnxtnode(g, v)) 
    {
	printf ("%s\n",agnameof(v));
	fast_node(g,v);
    }
    decompose(g,1);
    for (v = GD_nlist(g); v; v = ND_next(v)) 
    {
	printf ("_%s\n",agnameof(v));
    }
    b = new_queue(agnnodes(g));
    mcG = newMcGraph(g);


    for (v = GD_nlist(g); v; v = ND_next(v)) 
    {
	if ((inEdgeCnt(v, g) == 0) && (MND_inBucket(v) != 1)) {
	    enqueue(b, v);
	    MND_inBucket(v) = 1;

	    while ((v0 = dequeue(b))) {
		if (Verbose)
		    fprintf(stderr, "installing %s\n", agnameof(v0));
		install_node(v0, mcG);
		for (e = agfstout(g, v0); e; e = agnxtout(g, e)) {
		    head = aghead(e);
		    if ((MND_inBucket(head) != 1)) {
			enqueue(b, head);
			MND_inBucket(head) = 1;
		    }
		}
	    }

	}


    }
    

/*    for (v = agfstnode(g); v; v = agnxtnode(g, v)) {
	if ((inEdgeCnt(v, g) == 0) && (MND_inBucket(v) != 1)) {
	    enqueue(b, v);
	    MND_inBucket(v) = 1;

	    while ((v0 = dequeue(b))) {
		if (Verbose)
		    fprintf(stderr, "installing %s\n", agnameof(v0));
		install_node(v0, mcG);
		for (e = agfstout(g, v0); e; e = agnxtout(g, e)) {
		    head = aghead(e);
		    if ((MND_inBucket(head) != 1)) {
			enqueue(b, head);
			MND_inBucket(head) = 1;
		    }
		}
	    }

	}
    }

*/
/*    for (id=0;id < ARRAY_SZ; id++)
    {
	install_node(nodeByName(g,nodes[id]),mcG);
    }*/
    free_queue (b); 
    for (v = agfstnode(g); v; v = agnxtnode(g, v))
	addAdjEdges(v);
    id = countAllX(mcG);
#ifdef DEBUG
    #ifdef WIN32
    dumpGraph (mcG, "c:/temp/before.gv");
    #else
    dumpGraph (mcG, "before.gv");
    #endif
#endif
    _transpose(mcG, 0);
//    transposeAllL2H(mcG);


#ifdef DEBUG
    #ifdef WIN32
    dumpGraph (mcG, "c:/temp/after_trans.gv");
    #else
    dumpGraph (mcG, "after_trans.gv");
    #endif
#endif


    id = countAllX(mcG);

    return mcG;

}
static mcGraph *createMatrixOut(Agraph_t * g)
{
    Agedge_t *e;
    Agnode_t *v;
    Agnode_t *v0;
    nodequeue *b;
    Agnode_t *tail;
    mcGraph *mcG;
    b = new_queue(agnnodes(g));
    mcG = newMcGraph(g);
    for (v = agfstnode(g); v; v = agnxtnode(g, v)) {
	if ((outEdgeCnt(v, g) == 0) && (MND_inBucket(v) != 1)) {
	    enqueue(b, v);
	    MND_inBucket(v) = 1;

	    while ((v0 = dequeue(b))) {
		install_node(v0, mcG);
		if (Verbose)
		    fprintf(stderr, "installing %s\n", agnameof(v0));

		for (e = agfstin(g, v0); e; e = agnxtin(g, e)) {
		    tail = agtail(e);
		    if ((MND_inBucket(tail) != 1)) {
			enqueue(b, tail);
			MND_inBucket(tail) = 1;
		    }
		}
	    }

	}
    }
    free_queue (b); 
    for (v = agfstnode(g); v; v = agnxtnode(g, v))
	addAdjEdges(v);
    transposeAllH2L(mcG);


    return mcG;
}

/*
    calculates new positions of nodes by using median values,
    inOrder=1 direction is from lower rank to higher rank
    inorder=0 direction is from higher rank to lower rank
    this functions returns the local crossing count (count of passed level)
*/
static int int_cmp(const void *a, const void *b)
{
    const int *ia = (const int *) a;	// casting pointer types
    const int *ib = (const int *) b;
    return *ia - *ib;
    /* integer comparison: returns negative if b > a 
       and positive if a > b */
}


static int nodeMedian(mcNode * n, int lowToHigh)
{
    int P[1024];
    int id = 0;
    int m;
    int ps;
    mcEdge **edges;
    int left;
    int right;
    mcEdge *e;
    if (lowToHigh) {
	edges = n->low_edgs;
	m = n->low_edgs_cnt;

    } else {
	edges = n->high_edgs;
	m = n->high_edgs_cnt;

    }
    ps = m;
    m = m / 2;
    for (id = 0; id < ps; id++) {
	e = edges[id];
	if (lowToHigh)
	    P[id] = (int)e->lowerN->order;
	else
	    P[id] = (int)e->higherN->order;
    };
    qsort(P, ps, sizeof(int), int_cmp);
    if (ps == 0)
	return -1;
    if ((ps % 2) == 1)
	return P[m];
    if (ps == 2) {
	return (P[0] + P[1]) / 2;
    }
    left = P[m - 1] - P[0];
    right = P[ps - 1] - P[m];
    return (P[m - 1] * right + P[m] * left) / (left + right);





/*
12. procedure median_value(v,adj_rank)
13. P = adj_position(v,adj_rank);
14. m = ú Pú /2;
15. if ú Pú = 0 then
16. return -1.0;
17. elseif ú Pú mod 2 == 1 then
18. return P[m];
19. elseif ú Pú = 2 then
20. return (P[0] + P[1])/2;
21. else
22. left = P[m-1] - P[0];
23. right = P[ú Pú -1] - P[m];
24. return (P[m-1]*right + P[m]*left)/(left+right);
25. endif
26. end
*/


}


static int nodeMedian2(mcNode * n, int lowToHigh)
{
/*    int edgeCnt;
    int median;
    int even=0;
    float newOrder;
    Dt_t* edges;
    mcEdge* e;
    if (lowToHigh)
        edges=n->low_edgs;
    else
        edges=n->high_edgs;
    edgeCnt=dtsize(edges);
    if(edgeCnt==0)
	return -1;
    if(edgeCnt % 2 == 0)
	    even=1;
    edgeCnt=edgeCnt /2;
    for(e=dtfirst(edges);edgeCnt > 0;edgeCnt--)
        e=dtnext(edges,e);
    if(lowToHigh)
        newOrder=e->lowerN->order;
    else
        newOrder=e->higherN->order;
    if(even)
    {
        e=dtprev(edges,e);
        if(lowToHigh)
	    newOrder=(newOrder+e->lowerN->order)/2;
        else
	    newOrder=(newOrder+e->higherN->order)/2;
    }
//    return newOrder;
    return nodeMedian2( n,lowToHigh);*/
    return 0;
}

static void normalizeLevel(mcLevel * l)
{
    int id = 0;
    mcNode *n;
    for (id = 0; id < l->nodecnt; id++) {
	n = l->nodes[id];
	n->order = (float)id;
    }

}
static void reSetNodePos(mcGraph * mcG, mcLevel * l, int lowToHigh)
{
    mcNode *n;
    int id = 0;
    float order;
    float order2;
    for (id = 0; id < l->nodecnt; id++) {
	n = l->nodes[id];
	order = (float)nodeMedian(n, lowToHigh);
	order2 = (float)nodeMedian(n, (lowToHigh + 1) % 2);
	if (order == -1)
	    order = n->order;
	if (order2 == -1)
	    order2 = order;
	if (MAGIC_NUMBER != 0)
	    n->order = (order + order2 / MAGIC_NUMBER);
	else
	    n->order = order;
	MND_orderr(n->node) = n->order;
    }
    //normalize

    reOrderLevel(l);
    normalizeLevel(l);
}

static void cacheOrders(mcGraph * mcg, int cacheId)
{
    mcNode *n;
    mcLevel *l;
    int id = 0;
    int idx;
    for (idx = 0; idx < mcg->lvl_cnt; idx++) {
	l = mcg->lvls[idx];
	for (id = 0; id < l->nodecnt; id++) {
	    n = l->nodes[id];
	    n->prevOrder[cacheId] = n->order;
	}
    }
}

static void loadOrders(mcGraph * mcg, int cacheId)
{
    mcNode *n;
    mcLevel *l;
    int id = 0;
    int idx;
    for (idx = 0; idx < mcg->lvl_cnt; idx++) {
	l = mcg->lvls[idx];
	for (id = 0; id < l->nodecnt; id++) {
	    n = l->nodes[id];
	    n->order = n->prevOrder[cacheId];
	}
    }
    for (idx = 0; idx < mcg->lvl_cnt; idx++) {
	l = mcg->lvls[idx];
	reOrderLevel(l);
	normalizeLevel(l);
    }
}

static void wMedianLevel(mcLevel * l, mcGraph * mg, int lowToHigh)
{
    if (lowToHigh)
	reSetNodePos(mg, l, 1);
    else
	reSetNodePos(mg, l, 0);

}
static void dumbcross(mcGraph * mcG, int pass)
{
    mcLevel *l;
    int idx;
    int it = 0;
    int lowToHigh = 0;
    int best;
    for (idx = 0; idx < mcG->lvl_cnt; idx++) {
	l = mcG->lvls[idx];
	reOrderLevel(l);
	normalizeLevel(l);
    }
    cacheOrders(mcG, 0);	/*save init pos */
    best = mcG->crossCnt = countAllX(mcG);
    fprintf(stderr, "original crossing count : %d\n", mcG->crossCnt);
    for (it = 0; it < pass; it++) {
	if (lowToHigh == 0)
	    lowToHigh = 1;
	else
	    lowToHigh = 0;
	/*low to high */
	if (lowToHigh) {
	    int cnt = 0;
	    cacheOrders(mcG, 0);
	    for (idx = 0; idx < mcG->lvl_cnt; idx++) {
		l = mcG->lvls[idx];
		wMedianLevel(l, mcG, lowToHigh);
	    }
	    cnt = countAllX(mcG);
	    if (cnt < best) {
		best = cnt;
		cacheOrders(mcG, 0);

	    } else
		loadOrders(mcG, 0);
	    fprintf(stderr, "%d -> %d\n\n", best, cnt);
	} else {
	    int cnt = 0;
	    cacheOrders(mcG, 0);
	    for (idx = mcG->lvl_cnt - 1; idx >= 0; idx--) {
		l = mcG->lvls[idx];
		wMedianLevel(l, mcG, lowToHigh);
	    }
	    cnt = countAllX(mcG);
	    if (cnt < best) {
		best = cnt;
		cacheOrders(mcG, 0);

	    } else
		loadOrders(mcG, 0);
	    fprintf(stderr, "%d -> %d\n\n", best, cnt);
	}
    }

}

void clusterEdges(mcGraph * mcG)
{
    mcLevel *l;
    mcNode *n = NULL;
    mcEdge *e = NULL;
    int id, id2;
    Agnode_t *n1;
    Agnode_t *n2;
    int idx = 0;
    for (idx = 0; idx < mcG->lvl_cnt; idx++) {
	l = mcG->lvls[idx];
	for (id = 0; id < l->nodecnt; id++) {
	    n = l->nodes[id];
	    for (id2 = 0; id2 < n->low_edgs_cnt; id2++) {
		e = n->low_edgs[id2];
		n1 = e->higherN->node;
		n2 = e->lowerN->node;
		if (MND_highCluster(n1) == MND_highCluster(n2))
		    if (MND_highCluster(n1)) {
			e->penalty = 10000;
			e->higherN->order = -1;
			e->lowerN->order = -1;

		    }

	    }
	}

    }


    for (idx = mcG->lvl_cnt - 1; idx >= 0; idx--) {
	l = mcG->lvls[idx];

	for (id = 0; id < l->nodecnt; id++) {
	    n = l->nodes[id];
	    for (id2 = 0; id2 < n->high_edgs_cnt; id2++) {
		e = n->high_edgs[id2];
		n1 = e->higherN->node;
		n2 = e->lowerN->node;
		if (MND_highCluster(n1) == MND_highCluster(n2))
		    if (MND_highCluster(n1)) {
			e->penalty = 10000;
			e->higherN->order = -1;
			e->lowerN->order = -1;

		    }

	    }
	}

    }



    for (idx = 0; idx < mcG->lvl_cnt; idx++) {

	reOrderLevel(l);
	normalizeLevel(l);
    }



}

/*temp function to compare old and new dot*/
static void readOrders(Agraph_t * g)
{
    Agnode_t *v;
    char *bf;
    for (v = agfstnode(g); v; v = agnxtnode(g, v)) {
	bf = agget(v, "order");
	if (bf) {
	    MND_order(v) = atoi(bf);
//              MND_rank(v)=atoi(bf);
	    fprintf(stderr, "retrieved order :%d\n", MND_order(v));
	}
    }
}

static void mincross(mcGraph * mcG)
{
    int id;
    //   drawMatrix(mcG,"I0-original.png","a_original");
#ifdef DEBUG
    /* dumpGraph (mcG, "before.gv"); */
#endif
    for (id = 20; id >= 0; id--) {
	MAGIC_NUMBER = id;
	dumbcross(mcG, 25);
    }
    for (id = 0; id <= 15; id++) {
	MAGIC_NUMBER = id;
	dumbcross(mcG, 10);
    }

    dumpGraph (mcG, "c:/temp/before_final_trans.gv");
    _transpose(mcG,0);
    fprintf(stderr,"after transpose final count %d...\n", countAllX(mcG));
#ifdef DEBUG_CAIRO
    drawMatrix(mcG, "after.png", "after mincross");
#endif
#ifdef DEBUG
    dumpGraph (mcG, "c:/temp/after.gv");
#endif
}

Agraph_t *dot2_mincross (Agraph_t * srcGraph)
{
    mcGraph *mcG;
    Agraph_t *g = mkMCGraph (srcGraph);
    graphGroups(g);
    if (agattr(srcGraph, AGEDGE, "label", (char *) 0)) {
	double_ranks(g);
	addNodes(g, 1);
    } else
	addNodes(g, 0);
    nodesToClusters(g);

    mcG = createMatrixIn(g);
    fprintf(stderr, "in :%d\n", countAllX(mcG));
/*    mcG=createMatrixOut(g);
    fprintf(stderr, ("out :%d\n",countAllX(mcG));*/

    createGroups(mcG->g);
    clusterEdges(mcG);
    mincross(mcG);

    /* store order info in g */
    free_mcGraph (mcG);
    return g;
}

#ifdef DEBUG
static void dumpGraph (mcGraph* mcG, char* fname)
{
    FILE* fp = fopen (fname, "w");
    int id, id2, idx;
    mcLevel *l;
    mcLevel *nextl;
    mcNode *n;
    mcNode *nn;
    mcEdge *e;
    int dely = 72;
    int delx = 100;
    char* color;
    char buf[512];
    char buf2[512];
    int width, maxl;

    if (!fp) {
	fprintf (stderr, "Could not open %s for writing\n", fname);
	return;
    }
    fputs ("digraph {\n", fp);
    fputs ("  node[shape=box fontsize=8]\n", fp);
    maxl = 0;
    for (idx = 0; idx < mcG->lvl_cnt; idx++) {
	l = mcG->lvls[idx];
	if (l->nodecnt > maxl) maxl = l->nodecnt;

    }
    width = delx*(maxl+2);
    for (idx = 0; idx < mcG->lvl_cnt; idx++) {
	l = mcG->lvls[idx];
        delx = width/(l->nodecnt + 1); 
	for (id = 0; id < l->nodecnt; id++) {
	    n = l->nodes[id];
	    sprintf(buf, "%s(%0.1f)", agnameof(n->node), n->order);
	    if (n->group)
		color = "red";
	    else
		color = "black";
	    fprintf (fp, "  \"%s\" [pos=\"%d,%d\" color=%s]\n", buf, (id+1)*delx, -dely*idx, color);
	}

    }
    for (idx = 0; idx < mcG->lvl_cnt-1; idx++) {
	l = mcG->lvls[idx];
	nextl = mcG->lvls[idx + 1];
	for (id = 0; id < l->nodecnt; id++) {
	    n = l->nodes[id];
	    sprintf(buf, "%s(%0.1f)", agnameof(n->node), n->order);
	    for (id2 = 0; id2 < n->high_edgs_cnt; id2++) {
		e = n->high_edgs[id2];
		nn = e->higherN;
		sprintf(buf2, "%s(%0.1f)", agnameof(nn->node), nn->order);
		if (e->penalty > 1)
		    color = "red";
		else
		    color = "black";
		fprintf (fp, "  \"%s\" -> \"%s\" [color=%s]\n", buf, buf2, color);
	    }
	}
    }
    fputs ("}\n", fp);
    fclose (fp);
}
#endif

#ifdef DEBUG_CAIRO
#include <cairo/cairo.h>
#include <pango/pangocairo.h>
#include <png.h>

static int
writer(void *closure, const unsigned char *data, unsigned int length)
{
    if (length == fwrite(data, 1, length, (FILE *) closure)) {
	return CAIRO_STATUS_SUCCESS;
    }

    return CAIRO_STATUS_WRITE_ERROR;
}

static void drawMatrix(mcGraph * mcG, char *fileName, char *label)
{
    int ind = 0;
    mcLevel *l;
    mcLevel *nextl;
    mcNode *n;
    mcNode *nn;
    mcEdge *e;
    int id, idx, id2;
    int x = 0;
    int y = 10;
    int ex = 0;
    int ey = 0;
    FILE *output_file;
    cairo_t *cr;
    int c = 1;
    int rectW = 30;
    int rectH = 30;
    char buf[512];

    int verDist = (DOC_HEIGHT - 150) / (mcG->lvl_cnt - 1);
    cairo_surface_t *surface =
	cairo_image_surface_create(CAIRO_FORMAT_ARGB32, DOC_WIDTH + 100,
				   DOC_HEIGHT);
    cr = cairo_create(surface);
    cairo_select_font_face(cr, "Arial", CAIRO_FONT_SLANT_NORMAL,
			   CAIRO_FONT_WEIGHT_NORMAL);

    for (idx = 0; idx < mcG->lvl_cnt; idx++) {
	int crossing = -1;
	l = mcG->lvls[idx];
	c = 1;
/*
        nextl=dtnext(mcG->levels,l);
	if(nextl)
	{
	    crossing=countx(l,nextl);
	    sprintf(buf,"cross:(%d)",crossing);
	    cairo_set_font_size(cr,(double)16);
	    cairo_move_to (cr, 0, y+rectH-3);
	    cairo_text_path(cr,buf);
	    cairo_stroke (cr);

	}
*/
	for (id = 0; id < l->nodecnt; id++) {
	    static flip = 0;
	    n = l->nodes[id];
	    sprintf(buf, "%s(%0.1f)", agnameof(n->node), n->order);
	    x = (DOC_WIDTH / (l->nodecnt + 1)) * c;
	    if (n->group)
		cairo_set_source_rgb(cr, 1, 0, 0);
	    else
		cairo_set_source_rgb(cr, 0, 0, 0);

	    cairo_rectangle(cr, x, y, rectW, rectH);
	    cairo_set_font_size(cr, (double) 18);
	    cairo_move_to(cr, x + 3, y + rectH - 3 + flip * 20);
	    cairo_text_path(cr, buf);
	    cairo_stroke(cr);
	    n->x = x;
	    n->y = y;
	    c++;
	    if (flip == 0)
		flip = 1;
	    else
		flip = 0;
	}
	y = y + verDist;

    }
    cairo_set_source_rgb(cr, 0, 0, 0);
    cairo_set_font_size(cr, (double) 16);
    cairo_move_to(cr, 0, 30);
    cairo_text_path(cr, label);
    cairo_stroke(cr);
    for (idx = 0; idx < mcG->lvl_cnt; idx++) {
	l = mcG->lvls[idx];
	if (idx < mcG->lvl_cnt - 1)
	    nextl = mcG->lvls[idx + 1];
	else
	    nextl = NULL;
	for (id = 0; id < l->nodecnt; id++) {
	    n = l->nodes[id];
	    if (nextl) {
		x = n->x;
		y = n->y;
		for (id2 = 0; id2 < n->high_edgs_cnt; id2++) {
		    e = n->high_edgs[id2];
		    nn = e->higherN;
		    ex = nn->x;
		    ey = nn->y;
		    cairo_move_to(cr, x + rectW / 2, y + rectH);
		    if (e->penalty > 1)
			cairo_set_source_rgb(cr, 1, 0, 0);
		    else
			cairo_set_source_rgb(cr, 0, 0, 0);

		    cairo_line_to(cr, ex, ey);
		    cairo_stroke(cr);
		}
	    }
	}

    }

    sprintf(buf, "c:/temp/%s.png", fileName);
    output_file = fopen(buf, "wb+");

    if (output_file) {
	cairo_surface_write_to_png_stream(surface, writer, output_file);
    }
    fclose(output_file);
}
#endif

static void exchange(mcNode* v, mcNode* w)
{
    
    int vi, wi;
    vi=(int)v->order;
    wi=(int)w->order;
    v->order=(float)wi;
    w->order=(float)vi;
//	reOrderLevel((mcLevel*)v->level);

}

static int in_cross(mcNode* v, mcNode* w)
{
    register mcEdge* e1, *e2;
    register int inv, cross = 0, t;
    int id,id2;

    for (id=0;id < w->low_edgs_cnt; id++)
    {
	register int cnt;		
	e2=w->low_edgs[id];
	cnt=(int)e2->penalty;
	inv =(int) e2->lowerN->order;
	for (id2=0;id2 < v->low_edgs_cnt; id2++)
	{
	    e1=v->low_edgs[id2];
	    t =(int)e1->lowerN->order-inv;
	    if ((t > 0)
		|| ((t == 0)
		&& ( ED_tail_port(e1->lowerN->node).p.x > ED_tail_port(e2->lowerN->node).p.x)))
		    
		    cross += (int)e1->penalty * cnt;
	}
    }
    return cross;
}
static int out_cross(mcNode* v, mcNode* w)
{
    register mcEdge* e1, *e2;
    register int inv, cross = 0, t;
    int id,id2;

    for (id=0;id < w->high_edgs_cnt; id++)
    {
	register int cnt;		
	e2=w->high_edgs[id];
	cnt=(int)e2->penalty;
	inv = (int)e2->higherN->order;
	for (id2=0;id2 < v->high_edgs_cnt; id2++)
	{
	    e1=v->high_edgs[id2];
	    t =(int)e1->higherN->order-inv;
	    if ((t > 0)
		|| ((t == 0)
		&& ( ED_head_port(e1->higherN->node).p.x > ED_head_port(e2->higherN->node).p.x)))
		    
		    cross +=(int) e1->penalty * cnt;
	}
    }
    return cross;
}



static int transpose_step(mcGraph* mcG, int r, int reverse)
{
    int i, c0, c1, rv;
    mcNode *v,*w;
    mcLevel* l= mcG->lvls[r];

    rv = 0;
    l->candidate=FALSE;

    for (i = 0; i <l->nodecnt-1;i++) {
	v=l->nodes[i];
	w=l->nodes[i+1];
	assert(v->order < w->order);
	/*if (left2right(mcG, v, w))
	    continue;*/
	c0 = c1 = 0;
	if (r > 0) {
	    c0 += in_cross(v, w);
	    c1 += in_cross(w, v);
	}
	if (mcG->lvl_cnt > (r+1)) 
	{
	    c0 += out_cross(v, w);
	    c1 += out_cross(w, v);
	}
	if ((c1 < c0) || ((c0 > 0) && reverse && (c1 == c0))) 
	{
	    exchange(v, w);
	    rv += (c0 - c1);
	    l->candidate=TRUE;
	    if (r > 0) 
		mcG->lvls[r-1]->candidate=TRUE;
	    if (r < mcG->lvl_cnt-1) 
		mcG->lvls[r+1]->candidate = TRUE;
	}
    }
    reOrderLevel( mcG->lvls[r]);
    return rv;
}


static void _transpose(mcGraph * mcG, int reverse)
{
    int r, delta;

    for (r = 0; r <= mcG->lvl_cnt-1;r++)
	mcG->lvls[r]->candidate= TRUE;
    do {
	delta = 0;
#ifdef NOTDEF
	/* don't run both the upward and downward passes- they cancel. 
	   i tried making it depend on whether an odd or even pass, 
	   but that didn't help. */
	for (r = GD_maxrank(g); r >= GD_minrank(g); r--)
	    if (GD_rank(g)[r].candidate)
		delta += transpose_step(g, r, reverse);
#endif
	for (r = 0; r <= mcG->lvl_cnt-1;r++)
        {
	    if (mcG->lvls[r]->candidate) 
	    {
		delta += transpose_step(mcG, r, reverse);
	    }
	}
	/*} while (delta > ncross(g)*(1.0 - Convergence)); */
    } while (delta >= 1);
}




static void fast_node(graph_t * g, Agnode_t * n)
{
   ND_next(n) = GD_nlist(g);
    if (ND_next(n))
	ND_prev(ND_next(n)) = n;
    GD_nlist(g) = n;
    ND_prev(n) = NULL;
    assert(n != ND_next(n));
}

