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

#include "dot.h"

/*
 * Author: Mohammad T. Irfan
 *   Summer, 2008
 */

/* TODO:
 *   - Support clusters
 *   - Support disconnected graphs
 *   - Provide algorithms for aspect ratios < 1
 */

#define MIN_AR 1.0
#define MAX_AR 20.0
#define DEF_PASSES 5
#define DPI 72

/*
 *       NODE GROUPS FOR SAME RANKING
 *       Node group data structure groups nodes together for
 *       MIN, MAX, SOURCE, SINK constraints.
 *       The grouping is based on the union-find data structure and
 *       provides sequential access to the nodes in the same group.
 */

/* data structure for node groups */
typedef struct nodeGroup_t {
    node_t **nodes;
    int nNodes;
    double width, height;
} nodeGroup_t;

static nodeGroup_t *nodeGroups;
static int nNodeGroups = 0;

/* computeNodeGroups:
 * computeNodeGroups function does the groupings of nodes.   
 * The grouping is based on the union-find data structure.
 */
static void computeNodeGroups(graph_t * g)
{
    node_t *n;

    nodeGroups = N_GNEW(agnnodes(g), nodeGroup_t);

    nNodeGroups = 0;

    /* initialize node ids. Id of a node is used as an index to the group. */
    for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	ND_id(n) = -1;
    }

    for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	if (ND_UF_size(n) == 0) {	/* no same ranking constraint */
	    nodeGroups[nNodeGroups].nodes = NEW(node_t *);
	    nodeGroups[nNodeGroups].nodes[0] = n;
	    nodeGroups[nNodeGroups].nNodes = 1;
	    nodeGroups[nNodeGroups].width = ND_width(n);
	    nodeGroups[nNodeGroups].height = ND_height(n);

	    ND_id(n) = nNodeGroups;
	    nNodeGroups++;
	} else			/* group same ranked nodes */
	{
	    node_t *l = UF_find(n);
	    if (ND_id(l) > -1)	/* leader is already grouped */
	    {
		int index = ND_id(l);
		nodeGroups[index].nodes[nodeGroups[index].nNodes++] = n;
		nodeGroups[index].width += ND_width(n);
		nodeGroups[index].height =
		    (nodeGroups[index].height <
		     ND_height(n)) ? ND_height(n) : nodeGroups[index].
		    height;

		ND_id(n) = index;
	    } else		/* create a new group */
	    {
		nodeGroups[nNodeGroups].nodes =
		    N_NEW(ND_UF_size(l), node_t *);

		if (l == n)	/* node n is the leader */
		{
		    nodeGroups[nNodeGroups].nodes[0] = l;
		    nodeGroups[nNodeGroups].nNodes = 1;
		    nodeGroups[nNodeGroups].width = ND_width(l);
		    nodeGroups[nNodeGroups].height = ND_height(l);
		} else {
		    nodeGroups[nNodeGroups].nodes[0] = l;
		    nodeGroups[nNodeGroups].nodes[1] = n;
		    nodeGroups[nNodeGroups].nNodes = 2;
		    nodeGroups[nNodeGroups].width =
			ND_width(l) + ND_width(n);
		    nodeGroups[nNodeGroups].height =
			(ND_height(l) <
			 ND_height(n)) ? ND_height(n) : ND_height(l);
		}

		ND_id(l) = nNodeGroups;
		ND_id(n) = nNodeGroups;
		nNodeGroups++;
	    }
	}
    }

}

/*
 *       END OF CODES FOR NODE GROUPS 
 */

/* countDummyNodes:
 *  Count the number of dummy nodes
 */
int countDummyNodes(graph_t * g)
{
    int count = 0;
    node_t *n;
    edge_t *e;

    /* Count dummy nodes */
    for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	for (e = agfstout(g, n); e; e = agnxtout(g, e)) {
#ifdef UNUSED
	    /* this loop can be avoided */
	    for (k = ND_rank(agtail(e))+1; k < ND_rank(aghead(e)); k++) {
	       count++;
	    }
#endif
		/* flat edges do not have dummy nodes */
	    if (ND_rank(aghead(e)) != ND_rank(agtail(e)))	
		count += abs(ND_rank(aghead(e)) - ND_rank(agtail(e))) - 1;
	}
    }
    return count;
}

/*
 *       FFDH PACKING ALGORITHM TO ACHIEVE TARGET ASPECT RATIO
 */

/*
 *  layerWidthInfo_t: data structure for keeping layer width information
 *  Each layer consists of a number of node groups.
 */
typedef struct layerWidthInfo_t {
    int layerNumber;
    nodeGroup_t **nodeGroupsInLayer;
    int *removed;		/* is the node group removed? */
    int nNodeGroupsInLayer;
    int nDummyNodes;
    double width;
    double height;
} layerWidthInfo_t;

static layerWidthInfo_t *layerWidthInfo = NULL;
static int *sortedLayerIndex;
static int nLayers = 0;

/* computeLayerWidths:
 */
static void computeLayerWidths(graph_t * g)
{
    int i;
    node_t *v;
    node_t *n;
    edge_t *e;

    nLayers = 0;

    /* free previously allocated memory */
    if (layerWidthInfo) {
	for (i = 0; i < nNodeGroups; i++) {
	    if (layerWidthInfo[i].nodeGroupsInLayer) {
		int j;
		for (j = 0; j < layerWidthInfo[i].nNodeGroupsInLayer; j++) {
		    //if (layerWidthInfo[i].nodeGroupsInLayer[j])
		    //free(layerWidthInfo[i].nodeGroupsInLayer[j]);
		}
		free(layerWidthInfo[i].nodeGroupsInLayer);
	    }
	    if (layerWidthInfo[i].removed)
		free(layerWidthInfo[i].removed);
	}

	free(layerWidthInfo);
    }
    /* allocate memory
     * the number of layers can go up to the number of node groups
     */
    layerWidthInfo = N_NEW(nNodeGroups, layerWidthInfo_t);

    for (i = 0; i < nNodeGroups; i++) {
	layerWidthInfo[i].nodeGroupsInLayer =
	    N_NEW(nNodeGroups, nodeGroup_t *);

	layerWidthInfo[i].removed = N_NEW(nNodeGroups, int);

	layerWidthInfo[i].layerNumber = i;
	layerWidthInfo[i].nNodeGroupsInLayer = 0;
	layerWidthInfo[i].nDummyNodes = 0;
	layerWidthInfo[i].width = 0.0;
	layerWidthInfo[i].height = 0.0;
    }



    /* Count dummy nodes in the layer */
    for (n = agfstnode(g); n; n = agnxtnode(g, n))
	for (e = agfstout(g, n); e; e = agnxtout(g, e)) {
	    int k;

	    /* FIX: This loop maybe unnecessary, but removing it and using 
             * the commented codes next, gives a segmentation fault. I 
             * forgot the reason why.
             */
	    for (k = ND_rank(agtail(e)) + 1; k < ND_rank(aghead(e)); k++) {
		layerWidthInfo[k].nDummyNodes++;
	    }

#ifdef UNUSED
	    if (ND_rank(aghead(e)) != ND_rank(agtail(e)))
		/* flat edges do not have dummy nodes  */
		layerWidthInfo[k].nDummyNodes = abs(ND_rank(aghead(e)) - ND_rank(agtail(e))) - 1;
#endif
	}

#ifdef UNUSED
  /*****************************************************************
   * This code is removed. It considers dummy nodes in layer width,
   * which does not give good results in experiments.
   *****************************************************************/

    for (i = 0; i < nNodeGroups; i++) {
       v = nodeGroups[i].nodes[0];
       layerWidthInfo[ND_rank(v)].width = (layerWidthInfo[ND_rank(v)].nDummyNodes - 1) * GD_nodesep(g); 
       }
#endif

    /* gather the layer information */
    for (i = 0; i < nNodeGroups; i++) {
	v = nodeGroups[i].nodes[0];
	if (ND_rank(v) + 1 > nLayers)	/* update the number of layers */
	    nLayers = ND_rank(v) + 1;

	layerWidthInfo[ND_rank(v)].width +=
	    nodeGroups[i].width * DPI + (layerWidthInfo[ND_rank(v)].width >
					 0) * GD_nodesep(g);
	if (layerWidthInfo[ND_rank(v)].height < nodeGroups[i].height * DPI)
	    layerWidthInfo[ND_rank(v)].height = nodeGroups[i].height * DPI;
	layerWidthInfo[ND_rank(v)].
	    nodeGroupsInLayer[layerWidthInfo[ND_rank(v)].
			      nNodeGroupsInLayer] = &nodeGroups[i];
	layerWidthInfo[ND_rank(v)].nNodeGroupsInLayer++;
    }

}

/* compFunction:
 * Comparison function to be used in qsort.
 * For sorting the layers by nonincreasing width
 */
static int compFunction(const void *a, const void *b)
{
    int *ind1 = (int *) a;
    int *ind2 = (int *) b;

    return (layerWidthInfo[*ind2].width >
	    layerWidthInfo[*ind1].width) - (layerWidthInfo[*ind2].width <
					    layerWidthInfo[*ind1].width);
}

/* sortLayers:
 * Sort the layers by width (nonincreasing order)
 * qsort should be replaced by insertion sort for better performance.
 * (layers are "almost" sorted during iterations)
 */
static void sortLayers(graph_t * g)
{
    qsort(sortedLayerIndex, agnnodes(g), sizeof(int), compFunction);
}

#ifdef UNUSED
/* getMaxDummyNodes:
 * get the max # of dummy nodes on the incoming edges to a nodeGroup
 */
static int getMaxDummyNodes(nodeGroup_t * ng)
{
    int i, max = 0, cnt = 0;
    for (i = 0; i < ng->nNodes; i++) {
	node_t *n = ng->nodes[i];
	edge_t *e;
	graph_t *g = agraphof(n);
	for (e = agfstin(g, n); e; e = agnxtin(g, e)) {
	    cnt += ND_rank(aghead(e)) - ND_rank(agtail(e));	// it's 1 more than the original count
	    if (ND_rank(aghead(e)) - ND_rank(agtail(e)) > max)
		max = ND_rank(aghead(e)) - ND_rank(agtail(e));
	}
    }

    return max;
}
#endif

/* getOutDegree:
 * Return the sum of out degrees of the nodes in a node group.
 */
static int getOutDegree(nodeGroup_t * ng)
{
    int i, cnt = 0;
    for (i = 0; i < ng->nNodes; i++) {
	node_t *n = ng->nodes[i];
	edge_t *e;
	graph_t *g = agraphof(n);

	/* count outdegree. This loop might be unnecessary. */
	for (e = agfstout(g, n); e; e = agnxtout(g, e)) {
	    cnt++;
	}
    }

    return cnt;
}

/* compFunction2:
 * Comparison function to be used in qsort.
 * For sorting the node groups by their out degrees (nondecreasing)
 */
static int compFunction2(const void *a, const void *b)
{
    nodeGroup_t **ind1 = (nodeGroup_t **) a, **ind2 = (nodeGroup_t **) b;

    int cnt1 = getOutDegree(*ind1);
    int cnt2 = getOutDegree(*ind2);

    return (cnt2 < cnt1) - (cnt2 > cnt1);
}

#ifdef UNUSED
/* compFunction3:
 * Comparison function to be used in qsort.
 * For sorting the node groups by their height & width
 */
static int compFunction3(const void *a, const void *b)
{
    nodeGroup_t **ind1 = (nodeGroup_t **) a, **ind2 = (nodeGroup_t **) b;
    if ((*ind2)->height == (*ind1)->height)
	return ((*ind2)->width < (*ind1)->width) - ((*ind2)->width >
						    (*ind1)->width);

    return ((*ind2)->height < (*ind1)->height) - ((*ind2)->height >
						  (*ind1)->height);
}

/*****************************************************************
 * The following commented codes are no longer used
 * Originally used in the cocktail tool.
 *****************************************************************/
/* checkLayerConstraints:
 * check if there is any node group in the layer 
 * that is not constrained by MIN/MAX/SOURCE/SINK-RANK constraints.
 */
static int checkLayerConstraints(layerWidthInfo_t lwi)
{
    int i;
    for (i = 0; i < lwi.nNodeGroupsInLayer; i++) {
	if (lwi.nodeGroupsInLayer[i]->nNodes > 0) {
	    int rtype = ND_ranktype(lwi.nodeGroupsInLayer[i]->nodes[0]);
	    if (rtype != MINRANK && rtype != MAXRANK && rtype != SOURCERANK
		&& rtype != SINKRANK)
		return 1;
	}
    }

    return 0;
}

/* checkLayerConstraints:
 * check if all the node groups in the layer are 
 * constrained by MIN/MAX/SOURCE/SINK-RANK constraints
 */
static int checkLayerConstraints(layerWidthInfo_t lwi)
{
    int i;
    for (i = 0; i < lwi.nNodeGroupsInLayer; i++) {
	if (lwi.nodeGroupsInLayer[i]->nNodes > 0) {
	    int rtype = ND_ranktype(lwi.nodeGroupsInLayer[i]->nodes[0]);
	    if (rtype != MINRANK && rtype != MAXRANK && rtype != SOURCERANK
		&& rtype != SINKRANK)
		return 0;
	}
    }

    return 1;
}

/* checkNodeGroupConstraints:
 * check if the node group is not constrained by 
 * MIN/MAX/SOURCE/SINK-RANK constraints
 * Only used in the cocktail tool.
 */
static int checkNodeGroupConstraints(nodeGroup_t * ndg)
{
    int i;
    int rtype = ND_ranktype(ndg->nodes[0]);

    if (rtype != MINRANK && rtype != MAXRANK && rtype != SOURCERANK
	&& rtype != SINKRANK)
	return 1;

    return 0;
}

/* checkHorizontalEdge:
 * check if there is an edge from ng to a node in 
 * layerWidthInfo[nextLayerIndex].
 * Only used in the cocktail tool.
 */
static int
checkHorizontalEdge(graph_t * g, nodeGroup_t * ng, int nextLayerIndex)
{
    int i;
    edge_t *e;

    for (i = 0; i < ng->nNodes; i++) {
	for (e = agfstout(g, ng->nodes[i]); e; e = agnxtout(g, e)) {
	    if (layerWidthInfo[nextLayerIndex].layerNumber ==
		ND_rank(aghead(e))) {
		return 1;
	    }
	}
    }


    return 0;
}

/* hasMaxOrSinkNodes:
 * check if the the layer lwi has MAX or SINK nodes
 * Only used in the cocktail tool.
 */
static int hasMaxOrSinkNodes(layerWidthInfo_t * lwi)
{
    int i, j;

    for (i = 0; i < lwi->nNodeGroupsInLayer; i++) {
	if (lwi->removed[i])
	    continue;
	for (j = 0; j < lwi->nodeGroupsInLayer[i]->nNodes; j++) {
	    if (ND_ranktype(lwi->nodeGroupsInLayer[i]->nodes[j]) == MAXRANK
		|| ND_ranktype(lwi->nodeGroupsInLayer[i]->nodes[j]) ==
		SINKRANK)
		return 1;
	}
    }

    return 0;
}

/* reduceMaxWidth:
 * The following function is no longer used.
 * Originally used for FFDH packing heuristic
 * FFDH procedure
 */
static void reduceMaxWidth(graph_t * g)
{
    int i;
    int maxLayerIndex;		// = sortedLayerIndex[0];
    double nextMaxWidth;	// = (nLayers > 1) ? layerWidthInfo[sortedLayerIndex[1]].width : 0;
    double w = 0;
    Agnode_t *v;

    for (i = 0; i < nLayers; i++) {
	if (layerWidthInfo[sortedLayerIndex[i]].nNodeGroupsInLayer <= 1)	// || !checkLayerConstraints(layerWidthInfo[sortedLayerIndex[i]]))
	    continue;
	else {
	    maxLayerIndex = sortedLayerIndex[i];
	    nextMaxWidth =
		(nLayers >
		 i + 1) ? layerWidthInfo[sortedLayerIndex[i +
							  1]].width : 0;
	    break;
	}
    }

    if (i == nLayers)
	return;			//reduction of layerwidth is not possible.


    //sort the node groups in maxLayerIndex layer by height and then width, nonincreasing
    qsort(layerWidthInfo[maxLayerIndex].nodeGroupsInLayer,
	  layerWidthInfo[maxLayerIndex].nNodeGroupsInLayer,
	  sizeof(nodeGroup_t *), compFunction2);
    //printf("ht0 = %lf, ht1 = %lf\n", ND_height(layerWidthInfo[maxLayerIndex].nodes[0]), ND_height(layerWidthInfo[maxLayerIndex].nodes[1]));


    if (nextMaxWidth <= layerWidthInfo[maxLayerIndex].width / 2
	|| nextMaxWidth == layerWidthInfo[maxLayerIndex].width)
	nextMaxWidth = layerWidthInfo[maxLayerIndex].width / 2;

    double targetWidth = nextMaxWidth;	//layerWidthInfo[maxLayerIndex].width/2;

    //printf("max = %lf, target = %lf\n", layerWidthInfo[maxLayerIndex].width, targetWidth);//,  w + (layerWidthInfo[maxLayerIndex].nodeGroupsInLayer[i])->width );
    //getchar();


    //packing by node demotion
    int nextLayerIndex = -1;
    for (i = 0; i < nLayers; i++) {
	if (layerWidthInfo[i].layerNumber ==
	    layerWidthInfo[maxLayerIndex].layerNumber + 1)
	    nextLayerIndex = i;
    }

    if (nextLayerIndex > -1) {
	//if (layerWidthInfo[nextLayerIndex].width <= 0.5*layerWidthInfo[maxLayerIndex].width)
	//{
	int changed = 0;
	//demote nodes to the next layer
	for (i = 0; i < layerWidthInfo[maxLayerIndex].nNodeGroupsInLayer;
	     i++) {
	    if (layerWidthInfo[maxLayerIndex].removed[i])
		continue;

	    if (!checkHorizontalEdge
		(g, layerWidthInfo[maxLayerIndex].nodeGroupsInLayer[i],
		 nextLayerIndex)
		&& w + layerWidthInfo[nextLayerIndex].width +
		(layerWidthInfo[maxLayerIndex].nodeGroupsInLayer[i])->
		width <= targetWidth) {
		w += (layerWidthInfo[maxLayerIndex].nodeGroupsInLayer[i])->
		    width;
		changed++;

		int j;
		nodeGroup_t *ng =
		    layerWidthInfo[maxLayerIndex].nodeGroupsInLayer[i];

		layerWidthInfo[maxLayerIndex].removed[i] = 1;
		layerWidthInfo[maxLayerIndex].nNodeGroupsInLayer--;
		layerWidthInfo[maxLayerIndex].width -= ng->width;
		for (j = 0; j < ng->nNodes; j++)
		    ND_rank(ng->nodes[j])++;


		layerWidthInfo[nextLayerIndex].
		    nodeGroupsInLayer[layerWidthInfo[nextLayerIndex].
				      nNodeGroupsInLayer] = ng;
		layerWidthInfo[nextLayerIndex].
		    removed[layerWidthInfo[nextLayerIndex].
			    nNodeGroupsInLayer] = 0;
		layerWidthInfo[nextLayerIndex].nNodeGroupsInLayer++;
		layerWidthInfo[nextLayerIndex].width += ng->width;

		//int jj;
		//for (jj = 0; jj < layerWidthInfo[nextLayerIndex].nNodeGroupsInLayer; jj++) {
		    //Agnode_t *node = layerWidthInfo[nextLayerIndex].nodeGroupsInLayer[jj]->nodes[0];
		    //printf("%s\n", agnameof(node));
		//}
	    }

	}

	if (changed) {
	    //printf("Demoted %d nodes\n", changed);
	    return;
	}
	//}
    }
    //packing by creating new layers. Must be commented out if packing by demotion is used

    //going to create a new layer. increase the rank of all higher ranked nodes. (to be modified...)
    for (v = agfstnode(g); v; v = agnxtnode(g, v)) {
	if (ND_rank(v) > layerWidthInfo[maxLayerIndex].layerNumber)	///////////********  +1
	    ND_rank(v)++;
    }

    for (i = 0; i < nLayers; i++) {
	if (layerWidthInfo[i].layerNumber >
	    layerWidthInfo[maxLayerIndex].layerNumber)
	    layerWidthInfo[i].layerNumber++;
    }

    //now partition the current layer into two layers (to be modified to support general case of > 2 layers)
    int flag = 0;		//is a new layer created?
    int alt = 0;
    for (i = 0; i < layerWidthInfo[maxLayerIndex].nNodeGroupsInLayer; i++) {
	if (layerWidthInfo[maxLayerIndex].removed[i])
	    continue;

	//nodesep-> only if there are > 1 nodes*******************************
	if ((w +
	     (layerWidthInfo[maxLayerIndex].nodeGroupsInLayer[i])->width *
	     DPI + (w > 0) * GD_nodesep(g) <= targetWidth && alt == 0
	     && ND_ranktype(layerWidthInfo[maxLayerIndex].
			    nodeGroupsInLayer[i]->nodes[0]) != SINKRANK
	     && ND_ranktype(layerWidthInfo[maxLayerIndex].
			    nodeGroupsInLayer[i]->nodes[0]) != MAXRANK)
	    ||
	    (ND_ranktype
	     (layerWidthInfo[maxLayerIndex].nodeGroupsInLayer[i]->
	      nodes[0]) != SINKRANK
	     && ND_ranktype(layerWidthInfo[maxLayerIndex].
			    nodeGroupsInLayer[i]->nodes[0]) != MAXRANK
	     && hasMaxOrSinkNodes(&layerWidthInfo[maxLayerIndex]))
	    )
	    //&& ND_pinned(layerWidthInfo[maxLayerIndex].nodes[i]) == 0 ) 
	{
	    w += (layerWidthInfo[maxLayerIndex].nodeGroupsInLayer[i])->
		width * DPI + (w > 0) * GD_nodesep(g);
	    alt = 1;
	} else {
	    int j;
	    nodeGroup_t *ng =
		layerWidthInfo[maxLayerIndex].nodeGroupsInLayer[i];

	    flag = 1;

	    layerWidthInfo[maxLayerIndex].removed[i] = 1;
	    layerWidthInfo[maxLayerIndex].nNodeGroupsInLayer--;
	    layerWidthInfo[maxLayerIndex].nDummyNodes++;	/////************** SHOULD BE INCREASED BY THE SUM OF INDEG OF ALL NODES IN GROUP
	    layerWidthInfo[maxLayerIndex].width -=
		(ng->width * DPI + GD_nodesep(g));
	    for (j = 0; j < ng->nNodes; j++)
		ND_rank(ng->nodes[j])++;

	    //create new layer
	    layerWidthInfo[nLayers].
		nodeGroupsInLayer[layerWidthInfo[nLayers].
				  nNodeGroupsInLayer] = ng;
	    layerWidthInfo[nLayers].nNodeGroupsInLayer++;
	    layerWidthInfo[nLayers].layerNumber = ND_rank(ng->nodes[0]);

	    layerWidthInfo[nLayers].width += (ng->width * DPI + (layerWidthInfo[nLayers].nNodeGroupsInLayer > 1) * GD_nodesep(g));	// just add the node widths now. 

	    alt = 0;
	}
    }

    if (flag) {
	//calculate number of dummy nodes
	node_t *n;
	edge_t *e;
	int nDummy = 0;

	for (n = agfstnode(g); n; n = agnxtnode(g, n))
	    for (e = agfstout(g, n); e; e = agnxtout(g, e)) {
		if ((ND_rank(aghead(e)) > layerWidthInfo[nLayers].layerNumber
		     && ND_rank(agtail(e)) <
		     layerWidthInfo[nLayers].layerNumber)
		    || (ND_rank(aghead(e)) <
			layerWidthInfo[nLayers].layerNumber
			&& ND_rank(agtail(e)) >
			layerWidthInfo[nLayers].layerNumber)
		    )
		    nDummy++;
	    }

	layerWidthInfo[nLayers].nDummyNodes = nDummy;
	layerWidthInfo[nLayers].width +=
	    (layerWidthInfo[nLayers].nDummyNodes - 1) * GD_nodesep(g);
	nLayers++;
    }

    else {
	//undo increment of ranks and layerNumbers.*****************
	for (v = agfstnode(g); v; v = agnxtnode(g, v)) {
	    if (ND_rank(v) > layerWidthInfo[maxLayerIndex].layerNumber + 1)
		ND_rank(v)--;
	}

	for (i = 0; i < nLayers; i++) {
	    if (layerWidthInfo[i].layerNumber >
		layerWidthInfo[maxLayerIndex].layerNumber + 1)
		layerWidthInfo[i].layerNumber--;
	}
    }
}
#endif

/* reduceMaxWidth2:
 * This is the main heuristic for partitioning the widest layer.
 * Partitioning is based on outdegrees of nodes.
 * Replace compFunction2 by compFunction3 if you want to partition
 * by node widths and heights.
 * FFDH procedure
 */
static void reduceMaxWidth2(graph_t * g)
{
    int i;
    int maxLayerIndex;
    double nextMaxWidth;
    double w = 0;
    double targetWidth;
    int fst;
    nodeGroup_t *fstNdGrp;
    int ndem;
    int p, q;
    int limit;
    int rem;
    int rem2;


    /* Find the widest layer. it must have at least 2 nodes. */
    for (i = 0; i < nLayers; i++) {
	if (layerWidthInfo[sortedLayerIndex[i]].nNodeGroupsInLayer <= 1)
	    continue;
	else {
	    maxLayerIndex = sortedLayerIndex[i];
	    /* get the width of the next widest layer */
	    nextMaxWidth =
		(nLayers >
		 i + 1) ? layerWidthInfo[sortedLayerIndex[i +
							  1]].width : 0;
	    break;
	}
    }

    if (i == nLayers)
	return;			/* reduction of layerwidth is not possible. */

    /* sort the node groups in maxLayerIndex layer by height and 
     * then width, nonincreasing 
     */
    qsort(layerWidthInfo[maxLayerIndex].nodeGroupsInLayer,
	  layerWidthInfo[maxLayerIndex].nNodeGroupsInLayer,
	  sizeof(nodeGroup_t *), compFunction2);

#if 0
    printf("\nSorted nodes in mx layer:\n---------------------------\n");
    for (i = 0; i < layerWidthInfo[maxLayerIndex].nNodeGroupsInLayer; i++) {
	Agnode_t *node = layerWidthInfo[maxLayerIndex].nodeGroupsInLayer[i]->nodes[0];
	printf("%s. width=%lf, height=%lf\n",
	       agnameof(node), node->width, node->height);
    }
#endif

    if (nextMaxWidth <= layerWidthInfo[maxLayerIndex].width / 4
	|| nextMaxWidth >= layerWidthInfo[maxLayerIndex].width * 3 / 4)
	nextMaxWidth = layerWidthInfo[maxLayerIndex].width / 2;

    targetWidth = nextMaxWidth;	/* layerWidthInfo[maxLayerIndex].width/2; */

    /* now partition the current layer into two or more 
     * layers (determined by the ranking algorithm)
     */
    fst = 0;
    ndem = 0;
    limit = layerWidthInfo[maxLayerIndex].nNodeGroupsInLayer;
    rem = 0;
    rem2 = 0;

    /* initialize w, the width of the widest layer after partitioning */
    w = 0;

    for (i = 0; i < limit + rem; i++) {
	if (layerWidthInfo[maxLayerIndex].removed[i]) {
	    rem++;
	    continue;
	}

	if ((w +
	     layerWidthInfo[maxLayerIndex].nodeGroupsInLayer[i]->width *
	     DPI + (w > 0) * GD_nodesep(g) <= targetWidth)
	    || !fst) {
	    w += (layerWidthInfo[maxLayerIndex].nodeGroupsInLayer[i])->
		width * DPI + (w > 0) * GD_nodesep(g);
	    if (!fst) {
		fstNdGrp =
		    layerWidthInfo[maxLayerIndex].nodeGroupsInLayer[i];
		fst = 1;
	    }
	} else {
	    nodeGroup_t *ng =
		layerWidthInfo[maxLayerIndex].nodeGroupsInLayer[i];


#ifdef UNUSED
	    /* The following code corrects w by adding dummy node spacing. 
	     * It's no longer used
	     */
	    int l;
	    for (l = 0; l < ng->nNodes; l++) {
		w += (ND_in(ng->nodes[l]).size - 1) * GD_nodesep(g);
	    }
#endif

	    for (p = 0; p < fstNdGrp->nNodes; p++)
		for (q = 0; q < ng->nNodes; q++) {
		    //printf("Trying to add virtual edge: %s -> %s\n",
		    //    agnameof(fstNdGrp->nodes[p]), agnameof(ng->nodes[q]));

		    /* The following code is for deletion of long virtual edges.
		     * It's no longer used.
		     */
#ifdef UNUSED
		    for (s = ND_in(ng->nodes[q]).size - 1; s >= 0; s--) {
			ev = ND_in(ng->nodes[q]).list[s];

			edge_t *et;
			int fail = 0;
			cnt = 0;

			for (et = agfstin(g, aghead(ev)); et;
			     et = agnxtin(g, et)) {
			    if (aghead(et) == aghead(ev)
				&& agtail(et) == agtail(ev)) {
				fail = 1;
				break;
			    }
			}

			if (fail) {
			    //printf("FAIL DETECTED\n");
			    continue;
			}


			if (ED_edge_type(ev) == VIRTUAL
			    && ND_rank(aghead(ev)) > ND_rank(agtail(ev)) + 1) {
			    //printf("%d. inNode= %s.deleted: %s->%s\n",
			    //   test++, agnameof(ng->nodes[q]),
			    //   agnameof(agtail(ev)), agnameof(aghead(ev)));

			    delete_fast_edge(ev);
			    free(ev);
			}
		    }
#endif

		    /* add a new virtual edge */
		    edge_t *newVEdge =
			virtual_edge(fstNdGrp->nodes[p], ng->nodes[q],
				     NULL);
		    ED_edge_type(newVEdge) = VIRTUAL;
		    ndem++;	/* increase number of node demotions */
		}

	    /* the following code updates the layer width information. The 
	     * update is not useful in the current version of the heuristic.
	     */
	    layerWidthInfo[maxLayerIndex].removed[i] = 1;
	    rem2++;
	    layerWidthInfo[maxLayerIndex].nNodeGroupsInLayer--;
	    /* SHOULD BE INCREASED BY THE SUM OF INDEG OF ALL NODES IN GROUP */
	    layerWidthInfo[maxLayerIndex].nDummyNodes++;
	    layerWidthInfo[maxLayerIndex].width -=
		(ng->width * DPI + GD_nodesep(g));
	}
    }
}

#ifdef UNUSED
/* balanceLayers:
 * The following is the layer balancing heuristic.
 * Balance the widths of the layers as much as possible.
 * It's no longer used.
 */
static void balanceLayers(graph_t * g)
{
    int maxLayerIndex, nextLayerIndex, i;
    double maxWidth, w;

    //get the max width layer number

    for (i = 0; i < nLayers; i++) {
	if (layerWidthInfo[sortedLayerIndex[i]].nNodeGroupsInLayer <= 1
	    ||
	    layerWidthInfo[sortedLayerIndex[i]].layerNumber + 1 == nLayers)
	    continue;
	else {
	    maxLayerIndex = sortedLayerIndex[i];
	    maxWidth = layerWidthInfo[maxLayerIndex].width;
	    printf("Balancing: maxLayerIndex = %d\n", maxLayerIndex);
	    break;
	}
    }

    if (i == nLayers)
	return;			//reduction of layerwidth is not possible.

    //balancing ~~ packing by node demotion
    nextLayerIndex = -1;
    for (i = 0; i < nLayers; i++) {
	if (layerWidthInfo[i].layerNumber ==
	    layerWidthInfo[maxLayerIndex].layerNumber + 1) {
	    nextLayerIndex = i;
	}
    }

    if (nextLayerIndex > -1) {
	//if (layerWidthInfo[nextLayerIndex].width <= 0.5*layerWidthInfo[maxLayerIndex].width)
	//{
	int changed = 0;
	w = 0;

	//demote nodes to the next layer
	for (i = 0; i < layerWidthInfo[maxLayerIndex].nNodeGroupsInLayer;
	     i++) {
	    if (layerWidthInfo[maxLayerIndex].removed[i])
		continue;

	    if (!checkHorizontalEdge
		(g, layerWidthInfo[maxLayerIndex].nodeGroupsInLayer[i],
		 nextLayerIndex)
		&& layerWidthInfo[nextLayerIndex].width
		/*+ (layerWidthInfo[maxLayerIndex].nodeGroupsInLayer[i])->width */
		<= layerWidthInfo[maxLayerIndex].width
		/*- (layerWidthInfo[maxLayerIndex].nodeGroupsInLayer[i])->width*/
		) {
		w += (layerWidthInfo[maxLayerIndex].nodeGroupsInLayer[i])->
		    width;
		changed++;

		int j;
		nodeGroup_t *ng =
		    layerWidthInfo[maxLayerIndex].nodeGroupsInLayer[i];

		layerWidthInfo[maxLayerIndex].removed[i] = 1;
		layerWidthInfo[maxLayerIndex].nNodeGroupsInLayer--;
		layerWidthInfo[maxLayerIndex].width -= (ng->width);
		layerWidthInfo[maxLayerIndex].nDummyNodes++;
		for (j = 0; j < ng->nNodes; j++)
		    ND_rank(ng->nodes[j])++;


		layerWidthInfo[nextLayerIndex].
		    nodeGroupsInLayer[layerWidthInfo[nextLayerIndex].
				      nNodeGroupsInLayer] = ng;
		layerWidthInfo[nextLayerIndex].
		    removed[layerWidthInfo[nextLayerIndex].
			    nNodeGroupsInLayer] = 0;
		layerWidthInfo[nextLayerIndex].nNodeGroupsInLayer++;
		layerWidthInfo[nextLayerIndex].width +=
		    (ng->width + GD_nodesep(g));
	    }

	}

	if (changed) {
	    //printf("Demoted %d nodes\n", changed);
	    return;
	}
	//}
    }
}

/* applyPacking:
 * The following is the initial packing heuristic
 * It's no longer used.
 */
static void applyPacking(graph_t * g, double targetAR)
{
    int i;

    sortedLayerIndex = N_NEW(agnnodes(g), int);

    for (i = 0; i < agnnodes(g); i++) {
	sortedLayerIndex[i] = i;
    }


    node_t *v;

    for (v = agfstnode(g); v; v = agnxtnode(g, v)) {
	//printf("%s, rank = %d, ranktype = %d\n", agnameof(v), ND_rank(v), ND_ranktype(v));
    }

    //GD_nodesep(g) = 0.25;
    //GD_ranksep(g) = 0.25;
    ////////////////////
    //printf("Nodesep = %d, Ranksep = %d\n",GD_nodesep(g), GD_ranksep(g));


    for (i = 0; i < 1; i++) {
	//printf("iteration = %d\n----------------------\n", i);
	for (v = agfstnode(g); v; v = agnxtnode(g, v)) {
	    //printf("%s rank = %d\n", agnameof(v), ND_rank(v));
	}

	computeLayerWidths(g);
	sortLayers(g);
	reduceMaxWidth(g);

	//printf("====================\n");
    }


    int k;

    for (k = 0; k < nLayers - 1; k++) {
	int cnt = 0, tg;
	if (layerWidthInfo[k].nNodeGroupsInLayer > 7) {

	    cnt = 0;
	    tg = layerWidthInfo[k].nNodeGroupsInLayer - 7;

	    for (i = layerWidthInfo[k].nNodeGroupsInLayer - 1; i >= 0; i--) {

		if (layerWidthInfo[k].removed[i])
		    continue;

		int j;
		nodeGroup_t *ng = layerWidthInfo[k].nodeGroupsInLayer[i];


		layerWidthInfo[k].removed[i] = 1;
		layerWidthInfo[k].nNodeGroupsInLayer--;
		layerWidthInfo[k].nDummyNodes++;
		layerWidthInfo[k].width -=
		    (ng->width * DPI + GD_nodesep(g));
		for (j = 0; j < ng->nNodes; j++)
		    ND_rank(ng->nodes[j])++;

		//create new layer
		layerWidthInfo[k +
			       1].nodeGroupsInLayer[layerWidthInfo[k +
								   1].
						    nNodeGroupsInLayer] =
		    ng;
		layerWidthInfo[k + 1].nNodeGroupsInLayer++;
		//layerWidthInfo[k+1].layerNumber = ND_rank(ng->nodes[0]);

		//layerWidthInfo[k+1].width += ( ng->width*DPI + (layerWidthInfo[nLayers].nNodeGroupsInLayer > 1) * GD_nodesep(g) ); // just add the node widths now. 

		cnt++;

		if (cnt == tg)
		    break;

	    }
	}
    }

    //calcualte the max width
    int maxW = 0;
    int nNodeG = 0, l, nDummy = 0;
    int index;

    for (k = 0; k < nLayers; k++) {
	//printf("Layer#=%d, #dumNd=%d, width=%0.1lf, node=%s\n", layerWidthInfo[k].layerNumber, layerWidthInfo[k].nDummyNodes, layerWidthInfo[k].width,
	// agnameof(layerWidthInfo[k].nodeGroupsInLayer[0]->nodes[0]));
	if (layerWidthInfo[k].width > maxW)	// && layerWidthInfo[k].nNodeGroupsInLayer > 0)
	{
	    maxW = layerWidthInfo[k].width;
	    nNodeG = layerWidthInfo[k].nNodeGroupsInLayer;
	    l = layerWidthInfo[k].layerNumber;
	    nDummy = layerWidthInfo[k].nDummyNodes;
	    index = k;
	}
    }
    //printf("Ht=%d, MxW=%d, #ndGr=%d, #dumNd=%d, lyr#=%d, 1stNd=%s\n", (nLayers-1)*DPI, maxW, nNodeG, nDummy, l, agnameof(layerWidthInfo[index].nodeGroupsInLayer[0]->nodes[0]));

    // printf("Finally...\n------------------\n");
    for (v = agfstnode(g); v; v = agnxtnode(g, v)) {
	//printf("%s, rank = %d, ranktype = %d\n", agnameof(v, ND_rank(v), ND_ranktype(v));
    }

}
#endif

/* applyPacking2:
 * The following is the packing heuristic for wide layout.
 */
static void applyPacking2(graph_t * g)
{
    int i;

    sortedLayerIndex = N_NEW(agnnodes(g), int);

    for (i = 0; i < agnnodes(g); i++) {
	sortedLayerIndex[i] = i;
    }

    computeLayerWidths(g);
    sortLayers(g);
    reduceMaxWidth2(g);

}

#ifdef UNUSED
/* applyPacking4:
 * The following is the packing heuristic for wide layout.
 * It's used with Nikolov-Healy healy heuristic.
 */
void applyPacking4(graph_t * g)
{
    int i;

    sortedLayerIndex = N_NEW(agnnodes(g), int);

    for (i = 0; i < agnnodes(g); i++) {
	sortedLayerIndex[i] = i;
    }


    for (i = 0; i < 1; i++) {
	/*    printf("iteration = %d\n----------------------\n", i);
	   for (v = agfstnode(g); v; v = agnxtnode(g,v))
	   {
	   printf("%s rank = %d\n", agnameof(v), ND_rank(v));
	   }
	 */


	computeLayerWidths(g);
	sortLayers(g);
	reduceMaxWidth2(g);
	//printf("====================\n");
    }
}

/*
 *       NOCOLOV & HEALY'S NODE PROMOTION HEURISTIC
 */

/****************************************************************
 * This data structure is needed for backing up node information
 * during node promotion
 ****************************************************************/
typedef struct myNodeInfo_t {
    int indegree;
    int outdegree;
    int rank;
    Agnode_t *node;
} myNodeInfo_t;

myNodeInfo_t *myNodeInfo;


/* getMaxLevelNumber:
 * return the maximum level number assigned
 */
int getMaxLevelNumber(graph_t * g)
{
    int max;
    Agnode_t *n;

    max = ND_rank(agfstnode(g));

    for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	if (ND_rank(n) > max)
	    max = ND_rank(n);
    }

    return max;
}

/* countDummyDiff:
 * return the difference in the count of dummy nodes before 
 * and after promoting the node v
 */
static int countDummyDiff(graph_t * g, Agnode_t * v, int max)
{
    int dummydiff = 0;
    Agedge_t *e;
    Agnode_t *u;
    int maxR = 0;
    int j;

    for (j = 0; j < ND_in(v).size; j++) {
	e = ND_in(v).list[j];
	u = agtail(e);

	if (myNodeInfo[ND_id(u)].rank == myNodeInfo[ND_id(v)].rank + 1) {
	    dummydiff += countDummyDiff(g, u, max);
	}
    }

    if (myNodeInfo[ND_id(v)].rank + 1 < max
	|| (ND_in(v).size == 0 && myNodeInfo[ND_id(v)].rank + 1 <= max))
	myNodeInfo[ND_id(v)].rank += 1;

    dummydiff = dummydiff - ND_in(v).size + ND_out(v).size;


    return dummydiff;
}

/* applyPromotionHeuristic:
 * Node Promotion Heuristic
 * by Nikolov and Healy
 */
static void applyPromotionHeuristic(graph_t * g)
{
    graph_t graphBkup = *g;
    Agnode_t *v;
    int promotions;

    int max = getMaxLevelNumber(g);
    int count = 0;
    int nNodes = agnnodes(g);
    int i, j;

    myNodeInfo = N_NEW(nNodes, myNodeInfo_t);
    myNodeInfo_t *myNodeInfoBak = N_NEW(nNodes, myNodeInfo_t);

    for (v = agfstnode(g), i = 0; v; v = agnxtnode(g, v), i++) {
	myNodeInfo[i].indegree = ND_in(v).size;
	myNodeInfo[i].outdegree = ND_out(v).size;
	myNodeInfo[i].rank = ND_rank(v);
	myNodeInfo[i].node = v;
	ND_id(v) = i;

	myNodeInfoBak[i] = myNodeInfo[i];
    }

    do {
	promotions = 0;

	for (v = agfstnode(g); v; v = agnxtnode(g, v)) {
	    if (ND_in(v).size > 0) {
		if (countDummyDiff(g, v, max) <= 0) {
		    promotions++;

		    for (j = 0; j < nNodes; j++) {
			myNodeInfoBak[j] = myNodeInfo[j];
		    }

		} else {
		    for (j = 0; j < nNodes; j++) {
			myNodeInfo[j] = myNodeInfoBak[j];
		    }
		}
	    }
	}
	count++;
    } while (count < max);

    for (v = agfstnode(g); v; v = agnxtnode(g, v)) {
	ND_rank(v) = myNodeInfo[ND_id(v)].rank;
    }
}

/*
 *       LONGEST PATH ALGORITHM
 */

/* allNeighborsAreBelow:
 * Return 1 if all the neighbors of n already ranked, else 0
 */
static int allNeighborsAreBelow(Agnode_t * n)
{
    Agedge_t *e;
    graph_t *g = agraphof(n);
    int i;

    //for (e = agfstout(g,n); e; e = agnxtout(g,e))
    for (i = 0; i < ND_out(n).size; i++) {
	e = ND_out(n).list[i];
	if (ED_edge_type(e) == VIRTUAL) {
	    if (ED_to_orig(e) != NULL)
		e = ED_to_orig(e);
	    else if (ND_node_type(aghead(e)) == VIRTUAL)
		continue;
	}

	if (ND_pinned(aghead(e)) != 2)	//neighbor of n is not below
	{
	    return 0;
	}
    }

    return 1;
}

/* reverseLevelNumbers:
 * In Nikolov and Healy ranking, bottom layer ranking is  0 and
 * top layer ranking is the maximum. 
 * Graphviz does the opposite.
 * This function does the reversing from Nikolov to Graphviz.
 */
static void reverseLevelNumbers(graph_t * g)
{
    Agnode_t *n;
    int max;

    max = getMaxLevelNumber(g);

    //printf("max = %d\n", max);

    //return;
    for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	ND_rank(n) = max - ND_rank(n);
    }
}

/* doSameRank:
 * Maintain the same ranking constraint.
 * Can only be used with the Nikolov and Healy algorithm
 */
static void doSameRank(graph_t * g)
{
    int i;
    for (i = 0; i < nNodeGroups; i++) {
	int j;

	for (j = 0; j < nodeGroups[i].nNodes; j++) {
	    if (ND_ranktype(nodeGroups[i].nodes[j]) == SAMERANK)	//once we find a SAMERANK node in a group- make all the members of the group SAMERANK
	    {
		int k;
		int r = ND_rank(UF_find(nodeGroups[i].nodes[j]));
		for (k = 0; k < nodeGroups[i].nNodes; k++) {
		    ND_rank(nodeGroups[i].
			    nodes[(j + k) % nodeGroups[i].nNodes]) = r;
		}

		break;
	    }
	}
    }
}

/* doMinRank:
 * Maintain the MIN ranking constraint.
 * Can only be used with the Nikolov and Healy algorithm
 */
void doMinRank(graph_t * g)
{
    int i;
    for (i = 0; i < nNodeGroups; i++) {
	int j;

	for (j = 0; j < nodeGroups[i].nNodes; j++) {
	    if (ND_ranktype(nodeGroups[i].nodes[j]) == MINRANK)	//once we find a MINRANK node in a group- make the rank of all the members of the group 0
	    {
		int k;
		for (k = 0; k < nodeGroups[i].nNodes; k++) {
		    ND_rank(nodeGroups[i].
			    nodes[(j + k) % nodeGroups[i].nNodes]) = 0;
		    if (ND_ranktype
			(nodeGroups[i].
			 nodes[(j + k) % nodeGroups[i].nNodes]) !=
			SOURCERANK)
			ND_ranktype(nodeGroups[i].
				    nodes[(j +
					   k) % nodeGroups[i].nNodes]) =
			    MINRANK;
		}

		break;
	    }
	}
    }
}

/* getMaxRank:
 * Return the maximum rank among all nodes.
 */
static int getMaxRank(graph_t * g)
{
    int i;
    node_t *v;
    int maxR = ND_rank(agfstnode(g));
    for (v = agfstnode(g); v; v = agnxtnode(g, v)) {
	if (ND_rank(v) > maxR)
	    maxR = ND_rank(v);
    }

    return maxR;
}

/* doMaxRank:
 * Maintain the MAX ranking constraint.
 * Can only be used with the Nikolov and Healy algorithm
 */
static void doMaxRank(graph_t * g)
{
    int i;
    for (i = 0; i < nNodeGroups; i++) {
	int j;
	int maxR = getMaxRank(g);

	for (j = 0; j < nodeGroups[i].nNodes; j++) {
	    if (ND_ranktype(nodeGroups[i].nodes[j]) == MAXRANK)	//once we find a MAXRANK node in a group- make the rank of all the members of the group MAX
	    {
		int k;
		for (k = 0; k < nodeGroups[i].nNodes; k++) {
		    ND_rank(nodeGroups[i].
			    nodes[(j + k) % nodeGroups[i].nNodes]) = maxR;
		    if (ND_ranktype
			(nodeGroups[i].
			 nodes[(j + k) % nodeGroups[i].nNodes]) !=
			SINKRANK)
			ND_ranktype(nodeGroups[i].
				    nodes[(j +
					   k) % nodeGroups[i].nNodes]) =
			    MAXRANK;
		}

		break;
	    }
	}
    }
}

/* doSourceRank:
 * Maintain the SOURCE ranking constraint.
 * Can only be used with the Nikolov and Healy algorithm
 */
static void doSourceRank(graph_t * g)
{
    int i;
    int flag = 0;

    for (i = 0; i < nNodeGroups; i++) {
	int j;

	for (j = 0; j < nodeGroups[i].nNodes; j++) {
	    //once we find a SOURCERANK node in a group- make the rank of all the members of the group 0
	    if (ND_ranktype(nodeGroups[i].nodes[j]) == SOURCERANK) {
		int k;
		for (k = 0; k < nodeGroups[i].nNodes; k++) {
		    ND_rank(nodeGroups[i].
			    nodes[(j + k) % nodeGroups[i].nNodes]) = 0;
		    ND_ranktype(nodeGroups[i].
				nodes[(j + k) % nodeGroups[i].nNodes]) =
			SOURCERANK;
		}

		flag = 1;
		break;
	    }
	}
    }

    if (!flag)
	return;

    flag = 0;

    //The SourceRank group might be the only group having rank 0. Check if increment of ranking of other nodes is necessary at all.
    for (i = 0; i < nNodeGroups; i++) {
	if (nodeGroups[i].nNodes > 0
	    && ND_ranktype(nodeGroups[i].nodes[0]) != SOURCERANK
	    && ND_rank(nodeGroups[i].nodes[0]) == 0) {
	    flag = 1;
	    break;
	}
    }


    if (!flag)
	return;

    //Now make all NON-SourceRank nodes' ranking nonzero (increment)
    for (i = 0; i < nNodeGroups; i++) {
	if (nodeGroups[i].nNodes > 0
	    && ND_ranktype(nodeGroups[i].nodes[0]) != SOURCERANK) {
	    int j;

	    for (j = 0; j < nodeGroups[i].nNodes; j++) {
		ND_rank(nodeGroups[i].nodes[j])++;
	    }
	}
    }
}

/* doSinkRank:
 * Maintain the SINK ranking constraint.
 * Can only be used with the Nikolov and Healy algorithm
 */
static void doSinkRank(graph_t * g)
{
    int i, max;
    int flag = 0;

    max = getMaxRank(g);


    //Check if any non-sink node has rank = max
    for (i = 0; i < nNodeGroups; i++) {
	if (nodeGroups[i].nNodes > 0
	    && ND_ranktype(nodeGroups[i].nodes[0]) != SINKRANK
	    && ND_rank(nodeGroups[i].nodes[0]) == max) {
	    flag = 1;
	    break;
	}
    }

    if (!flag)
	return;

    for (i = 0; i < nNodeGroups; i++) {
	int j;

	for (j = 0; j < nodeGroups[i].nNodes; j++) {
	    if (ND_ranktype(nodeGroups[i].nodes[j]) == SINKRANK)	//once we find a SINKRANK node in a group- make the rank of all the members of the group: max+1
	    {
		int k;
		for (k = 0; k < nodeGroups[i].nNodes; k++) {
		    ND_rank(nodeGroups[i].
			    nodes[(j + k) % nodeGroups[i].nNodes]) =
			max + 1;
		    ND_ranktype(nodeGroups[i].
				nodes[(j + k) % nodeGroups[i].nNodes]) =
			SINKRANK;
		}

		break;
	    }
	}
    }
}

/* rank2: 
 * Initial codes for ranking (Nikolov-Healy).
 * It's no longer used.
 */
void rank2(graph_t * g)
{
    int currentLayer = 1;
    int nNodes = agnnodes(g);
    int nEdges = agnedges(g);
    int nPinnedNodes = 0, nSelected = 0;
    Agnode_t *n, **UArray;
    int USize = 0;
    int i, prevSize = 0;

    UArray = N_NEW(nEdges * 2, Agnode_t *);

    /* make all pinning values 0 */
    for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	ND_pinned(n) = 0;
    }

    while (nPinnedNodes != nNodes) {
	for (nSelected = 0, n = agfstnode(g); n; n = agnxtnode(g, n)) {
	    if (ND_pinned(n) == 0) {
		if (allNeighborsAreBelow(n)) {
		    ND_pinned(n) = 1;

		    UArray[USize] = n;
		    USize++;

		    ND_rank(n) = currentLayer;
		    nPinnedNodes++;
		    nSelected++;
		}
	    }
	}

	if (nSelected == 0)	//no node has been selected
	{
	    currentLayer++;
	    for (i = prevSize; i < USize; i++) {
		ND_pinned(UArray[i]) = 2;	//pinning value of 2 indicates this node is below the current node under consideration
	    }

	    prevSize = USize;
	}
    }

    //Apply Nikolov's node promotion heuristic
    applyPromotionHeuristic(g);

    //this is for the sake of graphViz layer numbering scheme
    reverseLevelNumbers(g);

    computeNodeGroups(g);	//groups of UF DS nodes

    //modify the ranking to respect the same ranking constraint
    doSameRank(g);

    //satisfy min ranking constraints
    doMinRank(g);
    doMaxRank(g);

    //satisfy source ranking constraints
    doSourceRank(g);
    doSinkRank(g);

    //Apply the FFDH algorithm to achieve better aspect ratio;
    applyPacking(g, 1);		//achieve an aspect ratio of 1
}
#endif

/****************************************************************
 * Initialize all the edge types to NORMAL 
 ****************************************************************/
void initEdgeTypes(graph_t * g)
{
    edge_t *e;
    node_t *n;
    int lc;

    for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	for (lc = 0; lc < ND_in(n).size; lc++) {
	    e = ND_in(n).list[lc];
	    ED_edge_type(e) = NORMAL;
	}
    }
}

/* computeCombiAR:
 * Compute and return combinatorial aspect ratio 
 * =
 * Width of the widest layer / Height 
 * (in ranking phase)
 */
static double computeCombiAR(graph_t * g)
{
    int i, maxLayerIndex;
    double maxW = 0;
    double maxH;
    double ratio;

    computeLayerWidths(g);
    maxH = (nLayers - 1) * GD_ranksep(g);

    for (i = 0; i < nLayers; i++) {
	if (maxW <
	    layerWidthInfo[i].width +
	    layerWidthInfo[i].nDummyNodes * GD_nodesep(g)) {
	    maxW =
		layerWidthInfo[i].width +
		layerWidthInfo[i].nDummyNodes * GD_nodesep(g);
	    maxLayerIndex = i;
	}
	maxH += layerWidthInfo[i].height;
    }

    ratio = maxW / maxH;

    return ratio;
}

#ifdef UNUSED
/* applyExpansion:
 * Heuristic for expanding very narrow graphs by edge reversal.
 * Needs improvement.
 */
void applyExpansion(graph_t * g)
{
    node_t *sink = NULL;
    int i, k;
    edge_t *e;

    computeLayerWidths(g);

    for (i = 0; i < nLayers; i++) {
	if (layerWidthInfo[i].layerNumber == nLayers / 2) {
	    k = i;
	    break;
	}
    }

    //now reverse the edges, from the k-th layer nodes to their parents
    for (i = 0; i < layerWidthInfo[k].nNodeGroupsInLayer; i++) {
	int p;
	nodeGroup_t *ng = layerWidthInfo[k].nodeGroupsInLayer[i];
	for (p = 0; p < ng->nNodes; p++) {
	    node_t *nd = ng->nodes[p];

	    while (e = ND_in(nd).list[0]) {
		printf("Reversing edge: %s->%s\n", agnemeof(agtail(e)),
		       agnameof(aghead(e)));
		reverse_edge(e);
	    }

	    int j, l;
	    node_t *v3;
	    edge_t *e3;
	    for (v3 = agfstnode(g); v3; v3 = agnxtnode(g, v3)) {
		for (e3 = agfstout(g, v3); e3; e3 = agnxtout(g, e3)) {
		    if (ND_rank(aghead(e3)) > k && ND_rank(agtail(e3)) < k) {
			printf("Reversing long edge: %s->%s\n",
			       agnameof(agtail(e3)), agnameof(aghead(e3)));
			reverse_edge(e3);
		    }

		}
	    }

	    /*for (l = 0; l < nLayers; l++) {
	        if (layerWidthInfo[l].layerNumber <= k)
	            continue;

	        for (j = 0; j < layerWidthInfo[l].nNodeGroupsInLayer; j++) {
	            int q;
	            nodeGroup_t *ng2 = layerWidthInfo[l].nodeGroupsInLayer[j];
	            for (q = 0; q < ng2->nNodes; q++) {
	                node_t *nd2 = ng2->nodes[q];
	                edge_t *e2;
	                int s = 0;
	                while (e2 = ND_in(nd2).list[s]) {
	                    if (ND_rank(agtail(e2)) < k) {
	                        printf("Reversing edge: %s->%s\n",
				    agnameof(agtail(e2)), agnameof(aghead(e2)));
	                        getchar();
	                        //reverse_edge(e2);
	                    }
	                    else s++;
	                }
	            }
	        }
	    } */

	    if (sink == NULL) {
		int brFlag = 1;
		for (l = 0; l < nLayers && brFlag; l++) {
		    for (j = 0;
			 j < layerWidthInfo[l].nNodeGroupsInLayer
			 && brFlag; j++) {
			int q;
			nodeGroup_t *ng2 =
			    layerWidthInfo[l].nodeGroupsInLayer[j];
			for (q = 0; q < ng2->nNodes && brFlag; q++) {
			    node_t *nd2 = ng2->nodes[q];
			    if (ND_in(nd2).size == 0) {
				sink = nd2;
				brFlag = 0;
			    }
			}
		    }
		}

	    }

	    virtual_edge(nd,	/*sink */
			 layerWidthInfo[0].nodeGroupsInLayer[0]->nodes[0],
			 NULL);
	}
    }

    //collapse_sets(g);
}
#endif

/* zapLayers:
 * After applying the expansion heuristic, some layers are
 * found to be empty.
 * This function removes the empty layers.
 */
static void zapLayers(graph_t * g)
{
    int i, j;
    int start = 0;
    int count = 0;

    /* the layers are sorted by the layer number.  now zap the empty layers */

    for (i = 0; i < nLayers; i++) {
	if (layerWidthInfo[i].nNodeGroupsInLayer == 0) {
	    if (count == 0)
		start = layerWidthInfo[i].layerNumber;
	    count++;
	} else if (count && layerWidthInfo[i].layerNumber > start) {
	    for (j = 0; j < layerWidthInfo[i].nNodeGroupsInLayer; j++) {
		int q;
		nodeGroup_t *ng = layerWidthInfo[i].nodeGroupsInLayer[j];
		for (q = 0; q < ng->nNodes; q++) {
		    node_t *nd = ng->nodes[q];
		    ND_rank(nd) -= count;
		}
	    }
	}
    }
}

/* rank3: 
 * ranking function for dealing with wide/narrow graphs,
 * or graphs with varying node widths and heights.
 * This function iteratively calls dot's rank1() function and 
 * applies packing (by calling the applyPacking2 function. 
 * applyPacking2 function calls the reduceMaxWidth2 function
 * for partitioning the widest layer).
 * Initially the iterations argument is -1, for which rank3
 * callse applyPacking2 function until the combinatorial aspect
 * ratio is <= the desired aspect ratio.
 */
void rank3(graph_t * g, aspect_t * asp)
{
    Agnode_t *n;
    int i;
    int iterations = asp->nextIter;
    double lastAR = MAXDOUBLE;

    computeNodeGroups(g);	/* groups of UF DS nodes */

    for (i = 0; (i < iterations) || (iterations == -1); i++) {
	/* initialize all ranks to be 0 */
	for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	    ND_rank(n) = 0;
	}

	/* need to compute ranking first--- by Network flow */

	rank1(g);

	asp->combiAR = computeCombiAR(g);
	if (Verbose)
	    fprintf(stderr, "combiAR = %lf\n", asp->combiAR);


	/* Uncomment the following codes, for working with narrow graphs */
#ifdef UNUSED
	if (combiAR < 0.8 * targetAR) {
	    char str[20];
	    printf("Apply expansion? (y/n):");
	    scanf("%s", str);
	    if (strcmp(str, "y") == 0)
		applyExpansion(g);
	    break;
	} else
#endif
        /* Success or if no improvement */
	if ((asp->combiAR <= asp->targetAR) || ((iterations == -1) && (lastAR <= asp->combiAR))) {
	    asp->prevIterations = asp->curIterations;
	    asp->curIterations = i;

	    break;
	}
	lastAR = asp->combiAR;
	/* Apply the FFDH algorithm to reduce the aspect ratio; */
	applyPacking2(g);
    }

    /* do network flow once again... incorporating the added edges */
    rank1(g);

    computeLayerWidths(g);
    zapLayers(g);
    asp->combiAR = computeCombiAR(g);
}

#ifdef UNUSED
/* NikolovHealy:
 * Nikolov-Healy approach to ranking.
 * First, use longest path algorithm.
 * Then use node promotion heuristic.
 * This function is called by rank4 function.
 */
static void NikolovHealy(graph_t * g)
{
    int currentLayer = 1;
    int nNodes = agnnodes(g);
    int nEdges = agnedges(g);
    int nPinnedNodes = 0, nSelected = 0;
    Agnode_t *n, **UArray;
    int USize = 0;
    int i, prevSize = 0;

    /************************************************************************
     * longest path algorithm
     ************************************************************************/
    UArray = N_NEW(nEdges * 2, Agnode_t *);

    /* make all pinning values 0 */
    for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	ND_pinned(n) = 0;
    }

    while (nPinnedNodes != nNodes) {
	for (nSelected = 0, n = agfstnode(g); n; n = agnxtnode(g, n)) {
	    if (ND_pinned(n) == 0) {
		if (allNeighborsAreBelow(n)) {
		    ND_pinned(n) = 1;

		    UArray[USize] = n;
		    USize++;

		    ND_rank(n) = currentLayer;
		    nPinnedNodes++;
		    nSelected++;
		}
	    }
	}

	if (nSelected == 0)	//no node has been selected
	{
	    currentLayer++;
	    for (i = prevSize; i < USize; i++) {
		ND_pinned(UArray[i]) = 2;	//pinning value of 2 indicates this node is below the current node under consideration
	    }

	    prevSize = USize;
	}

    }

    /************************************************************************
     * end of longest path algorithm
     ************************************************************************/

    //Apply node promotion heuristic
    applyPromotionHeuristic(g);

    //this is for the sake of graphViz layer numbering scheme
    reverseLevelNumbers(g);

}


/* rank4:
 * This function is calls the NikolovHealy function
 * for ranking in the Nikolov-Healy approach.
 */
void rank4(graph_t * g, int iterations)
{
    int currentLayer = 1;
    int nNodes = agnnodes(g);
    int nEdges = agnedges(g);
    int nPinnedNodes = 0, nSelected = 0;
    Agnode_t *n, **UArray;
    int USize = 0;
    int i, prevSize = 0;

    int it;
    printf("# of interations of packing: ");
    scanf("%d", &it);
    printf("it=%d\n", it);

    computeNodeGroups(g);	//groups of UF DS nodes

    for (i = 0; i < it; i++) {
	for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	    ND_rank(n) = 0;
	}

	NikolovHealy(g);

	edge_t *e;
	int cnt = 0;
	int lc;


	combiAR = computeCombiAR(g);
	printf("%d. combiAR = %lf\n", i, combiAR);

	/*
	   //modify the ranking to respect the same ranking constraint
	   doSameRank(g);

	   //satisfy min ranking constraints
	   doMinRank(g);
	   doMaxRank(g);

	   //satisfy source ranking constraints
	   doSourceRank(g);
	   doSinkRank(g);
	 */

	//Apply the FFDH algorithm to achieve better aspect ratio;
	applyPacking4(g);

    }

}
#endif

/* init_UF_size:
 * Initialize the Union Find data structure
 */
void init_UF_size(graph_t * g)
{
    node_t *n;

    for (n = agfstnode(g); n; n = agnxtnode(g, n))
	ND_UF_size(n) = 0;
}

aspect_t*
setAspect (Agraph_t * g, aspect_t* adata)
{
    double rv;
    char *p;
    int r, passes = DEF_PASSES;

    p = agget (g, "aspect");

    if (!p || ((r = sscanf (p, "%lf,%d", &rv, &passes)) <= 0)) {
	adata->nextIter = 0;
	adata->badGraph = 0;
	return NULL;
    }
    
    if (rv < MIN_AR) rv = MIN_AR;
    else if (rv > MAX_AR) rv = MAX_AR;
    adata->targetAR = rv;
    adata->nextIter = -1;
    adata->nPasses = passes;
    adata->badGraph = 0;

    if (Verbose) 
        fprintf(stderr, "Target AR = %g\n", adata->targetAR);

    return adata;
}
