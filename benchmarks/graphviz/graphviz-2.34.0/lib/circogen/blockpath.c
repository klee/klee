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


#include	"blockpath.h"
#include	"edgelist.h"
#include	"nodeset.h"
#include	"deglist.h"

/* The code below lays out a single block on a circle.
 */

/* We use the unused fields order and to_orig in cloned nodes and edges */
#define ORIGE(e)  (ED_to_orig(e))

/* clone_graph:
 * Create two copies of the argument graph
 * One is a subgraph, the other is an actual copy since we will be
 * adding edges to it.
 */
static Agraph_t *clone_graph(Agraph_t * ing, Agraph_t ** xg)
{
    Agraph_t *clone;
    Agraph_t *xclone;
    Agnode_t *n;
    Agnode_t *xn;
    Agnode_t *xh;
    Agedge_t *e;
    Agedge_t *xe;
    char gname[SMALLBUF];
    static int id = 0;

    sprintf(gname, "_clone_%d", id++);
#ifndef WITH_CGRAPH
    clone = agsubg(ing, gname);
#else /* WITH_CGRAPH */
    clone = agsubg(ing, gname,1);
    agbindrec(clone, "Agraphinfo_t", sizeof(Agraphinfo_t), TRUE);	//node custom data
#endif /* WITH_CGRAPH */
    sprintf(gname, "_clone_%d", id++);
#ifndef WITH_CGRAPH
    xclone = agopen(gname, ing->kind);
    for (n = agfstnode(ing); n; n = agnxtnode(ing, n)) {
	aginsert(clone, n);
	xn = agnode(xclone, agnameof(n));
#else /* WITH_CGRAPH */
    xclone = agopen(gname, ing->desc,NIL(Agdisc_t *));
    for (n = agfstnode(ing); n; n = agnxtnode(ing, n)) {
	agsubnode(clone,n,1);
	xn = agnode(xclone, agnameof(n),1);
        agbindrec(xn, "Agnodeinfo_t", sizeof(Agnodeinfo_t), TRUE);	//node custom data
#endif /* WITH_CGRAPH */
	CLONE(n) = xn;
    }

    for (n = agfstnode(ing); n; n = agnxtnode(ing, n)) {
	xn = CLONE(n);
#ifndef WITH_CGRAPH
	for (e = agfstout(ing, n); e; e = agnxtout(ing, e)) {
	    aginsert(clone, e);
	    xh = CLONE(e->head);
	    xe = agedge(xclone, xn, xh);
#else /* WITH_CGRAPH */
	for (e = agfstout(ing, n); e; e = agnxtout(ing, e)) {
	    agsubedge(clone,e,1);
	    xh = CLONE(aghead(e));
	    xe = agedge(xclone, xn, xh, NULL, 1);
	    agbindrec(xe, "Agedgeinfo_t", sizeof(Agedgeinfo_t), TRUE);	//node custom data
#endif /* WITH_CGRAPH */
	    ORIGE(xe) = e;
	    DEGREE(xn) += 1;
	    DEGREE(xh) += 1;
	}
    }
    *xg = xclone;
    return clone;
}

/* fillList:
 * Add nodes to deg_list, which stores them by degree.
 */
static deglist_t *getList(Agraph_t * g)
{
    deglist_t *dl = mkDeglist();
    Agnode_t *n;

    for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	insertDeglist(dl, n);
    }
    return dl;
}

/* find_pair_edges:
 */
static void find_pair_edges(Agraph_t * g, Agnode_t * n, Agraph_t * outg)
{
    Agnode_t **neighbors_with;
    Agnode_t **neighbors_without;

    Agedge_t *e;
    Agedge_t *ep;
    Agedge_t *ex;
    Agnode_t *n1;
    Agnode_t *n2;
    int has_pair_edge;
    int diff;
    int has_pair_count = 0;
    int no_pair_count = 0;
    int node_degree;
    int edge_cnt = 0;

    node_degree = DEGREE(n);
    neighbors_with = N_GNEW(node_degree, Agnode_t *);
    neighbors_without = N_GNEW(node_degree, Agnode_t *);

    for (e = agfstedge(g, n); e; e = agnxtedge(g, e, n)) {
	n1 = aghead(e);
	if (n1 == n)
	    n1 = agtail(e);
	has_pair_edge = 0;
	for (ep = agfstedge(g, n); ep; ep = agnxtedge(g, ep, n)) {
	    if (ep == e)
		continue;
	    n2 = aghead(ep);
	    if (n2 == n)
		n2 = agtail(ep);
	    ex = agfindedge(g, n1, n2);
	    if (ex) {
		has_pair_edge = 1;
		if (n1 < n2) {	/* count edge only once */
		    edge_cnt++;
		    if (ORIGE(ex)) {
			agdelete(outg, ORIGE(ex));
			ORIGE(ex) = 0;	/* delete only once */
		    }
		}
	    }
	}
	if (has_pair_edge) {
	    neighbors_with[has_pair_count] = n1;
	    has_pair_count++;
	} else {
	    neighbors_without[no_pair_count] = n1;
	    no_pair_count++;
	}
    }

    diff = node_degree - 1 - edge_cnt;
    if (diff > 0) {
	int mark;
	Agnode_t *hp;
	Agnode_t *tp;

	if (diff < no_pair_count) {
	    for (mark = 0; mark < no_pair_count; mark += 2) {
		if ((mark + 1) >= no_pair_count)
		    break;
		tp = neighbors_without[mark];
		hp = neighbors_without[mark + 1];
#ifndef WITH_CGRAPH
		agedge(g, tp, hp);
#else /* WITH_CGRAPH */
		agbindrec(agedge(g, tp, hp, NULL, 1), "Agedgeinfo_t", sizeof(Agedgeinfo_t), TRUE);   // edge custom data

#endif /* WITH_CGRAPH */
		DEGREE(tp)++;
		DEGREE(hp)++;
		diff--;
	    }

	    mark = 2;
	    while (diff > 0) {
		tp = neighbors_without[0];
		hp = neighbors_without[mark];
#ifndef WITH_CGRAPH
		agedge(g, tp, hp);
#else /* WITH_CGRAPH */

		agbindrec(agedge(g, tp, hp, NULL, 1), "Agedgeinfo_t", sizeof(Agedgeinfo_t), TRUE);   // edge custom data

#endif /* WITH_CGRAPH */
		DEGREE(tp)++;
		DEGREE(hp)++;
		mark++;
		diff--;
	    }
	}

	else if (diff == no_pair_count) {
	    tp = neighbors_with[0];
	    for (mark = 0; mark < no_pair_count; mark++) {
		hp = neighbors_without[mark];
#ifndef WITH_CGRAPH
		agedge(g, tp, hp);
#else /* WITH_CGRAPH */
		agbindrec(agedge(g, tp, hp, NULL, 1), "Agedgeinfo_t", sizeof(Agedgeinfo_t), TRUE);	//node custom data

#endif /* WITH_CGRAPH */
		DEGREE(tp)++;
		DEGREE(hp)++;
	    }
	}
    }

    free(neighbors_without);
    free(neighbors_with);
}

/* remove_pair_edges:
 * Create layout skeleton of ing.
 * Why is returned graph connected?
 */
static Agraph_t *remove_pair_edges(Agraph_t * ing)
{
    int counter = 0;
    int nodeCount;
    Agraph_t *outg;
    Agraph_t *g;
    deglist_t *dl;
    Agnode_t *currnode, *adjNode;
    Agedge_t *e;

    outg = clone_graph(ing, &g);
    nodeCount = agnnodes(g);
    dl = getList(g);

    while (counter < (nodeCount - 3)) {
	currnode = firstDeglist(dl);

	/* Remove all adjacent nodes since they have to be reinserted */
	for (e = agfstedge(g, currnode); e; e = agnxtedge(g, e, currnode)) {
	    adjNode = aghead(e);
	    if (currnode == adjNode)
		adjNode = agtail(e);
	    removeDeglist(dl, adjNode);
	}

	find_pair_edges(g, currnode, outg);

	for (e = agfstedge(g, currnode); e; e = agnxtedge(g, e, currnode)) {
	    adjNode = aghead(e);
	    if (currnode == adjNode)
		adjNode = agtail(e);

	    DEGREE(adjNode)--;
	    insertDeglist(dl, adjNode);
	}

	agdelete(g, currnode);

	counter++;
    }

    agclose(g);
    freeDeglist(dl);
    return outg;
}

static void
measure_distance(Agnode_t * n, Agnode_t * ancestor, int dist,
		 Agnode_t * change)
{
    Agnode_t *parent;

    parent = TPARENT(ancestor);
    if (parent == NULL)
	return;

    dist++;

    /* check parent to see if it has other leaf paths at greater distance
       than the context node.
       set the path/distance of the leaf at this ancestor node */

    if (DISTONE(parent) == 0) {
	LEAFONE(parent) = n;
	DISTONE(parent) = dist;
    } else if (dist > DISTONE(parent)) {
	if (LEAFONE(parent) != change) {
	    if (!DISTTWO(parent) || (LEAFTWO(parent) != change))
		change = LEAFONE(parent);
	    LEAFTWO(parent) = LEAFONE(parent);
	    DISTTWO(parent) = DISTONE(parent);
	}
	LEAFONE(parent) = n;
	DISTONE(parent) = dist;
    } else if (dist > DISTTWO(parent)) {
	LEAFTWO(parent) = n;
	DISTTWO(parent) = dist;
	return;
    } else
	return;

    measure_distance(n, parent, dist, change);
}

/* find_longest_path:
 * Find and return longest path in tree.
 */
static nodelist_t *find_longest_path(Agraph_t * tree)
{
    Agnode_t *n;
    Agedge_t *e;
    Agnode_t *common = 0;
    nodelist_t *path;
    nodelist_t *endPath;
    int maxlength = 0;
    int length;

    if (agnnodes(tree) == 1) {
	path = mkNodelist();
	n = agfstnode(tree);
	appendNodelist(path, NULL, n);
	SET_ONPATH(n);
	return path;
    }

    for (n = agfstnode(tree); n; n = agnxtnode(tree, n)) {
	int count = 0;
	for (e = agfstedge(tree, n); e; e = agnxtedge(tree, e, n)) {
	    count++;
	}
	if (count == 1)
	    measure_distance(n, n, 0, NULL);
    }

    /* find the branch node rooted at the longest path */
    for (n = agfstnode(tree); n; n = agnxtnode(tree, n)) {
	length = DISTONE(n) + DISTTWO(n);
	if (length > maxlength) {
	    common = n;
	    maxlength = length;
	}
    }

    path = mkNodelist();
    for (n = LEAFONE(common); n != common; n = TPARENT(n)) {
	appendNodelist(path, NULL, n);
	SET_ONPATH(n);
    }
    appendNodelist(path, NULL, common);
    SET_ONPATH(common);

    if (DISTTWO(common)) {	/* 2nd path might be empty */
	endPath = mkNodelist();
	for (n = LEAFTWO(common); n != common; n = TPARENT(n)) {
	    appendNodelist(endPath, NULL, n);
	    SET_ONPATH(n);
	}
	reverseAppend(path, endPath);
    }

    return path;
}

/* dfs:
 * Simple depth first search, adding traversed edges to tree.
 */
static void dfs(Agraph_t * g, Agnode_t * n, Agraph_t * tree)
{
    Agedge_t *e;
    Agnode_t *neighbor;

    SET_VISITED(n);
    for (e = agfstedge(g, n); e; e = agnxtedge(g, e, n)) {
	neighbor = aghead(e);
	if (neighbor == n)
	    neighbor = agtail(e);

	if (!VISITED(neighbor)) {
	    /* add the edge to the dfs tree */
#ifndef WITH_CGRAPH
	    aginsert(tree, e);
#else /* WITH_CGRAPH */
	    agsubedge(tree,e,1);
#endif /* WITH_CGRAPH */
	    TPARENT(neighbor) = n;
	    dfs(g, neighbor, tree);
	}
    }
}

/* spanning_tree:
 * Construct spanning forest of g as subgraph
 */
static Agraph_t *spanning_tree(Agraph_t * g)
{
    Agnode_t *n;
    Agraph_t *tree;
    char gname[SMALLBUF];
    static int id = 0;

    sprintf(gname, "_span_%d", id++);
#ifndef WITH_CGRAPH
    tree = agsubg(g, gname);
#else /* WITH_CGRAPH */
    tree = agsubg(g, gname,1);
    agbindrec(tree, "Agraphinfo_t", sizeof(Agraphinfo_t), TRUE);	//node custom data
#endif /* WITH_CGRAPH */

    for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
#ifndef WITH_CGRAPH
	aginsert(tree, n);
#else /* WITH_CGRAPH */
	agsubnode(tree,n,1);
#endif /* WITH_CGRAPH */
	DISTONE(n) = 0;
	DISTTWO(n) = 0;
	UNSET_VISITED(n);
    }

    for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	if (!VISITED(n)) {
	    TPARENT(n) = NULL;
	    dfs(g, n, tree);
	}
    }

    return tree;
}

/* block_graph:
 * Add induced edges.
 */
static void block_graph(Agraph_t * g, block_t * sn)
{
    Agnode_t *n;
    Agedge_t *e;
    Agraph_t *subg = sn->sub_graph;

    for (n = agfstnode(subg); n; n = agnxtnode(subg, n)) {
	for (e = agfstout(g, n); e; e = agnxtout(g, e)) {
	    if (BLOCK(aghead(e)) == sn)
#ifndef WITH_CGRAPH
		aginsert(subg, e);
#else /* WITH_CGRAPH */
		agsubedge(subg,e,1);
#endif /* WITH_CGRAPH */
	}
    }
}

static int count_all_crossings(nodelist_t * list, Agraph_t * subg)
{
    nodelistitem_t *item;
    edgelist *openEdgeList = init_edgelist();
    Agnode_t *n;
    Agedge_t *e;
    int crossings = 0;
    int order = 1;

    for (n = agfstnode(subg); n; n = agnxtnode(subg, n)) {
	for (e = agfstout(subg, n); e; e = agnxtout(subg, e)) {
	    EDGEORDER(e) = 0;
	}
    }

    for (item = list->first; item; item = item->next) {
	n = item->curr;

	for (e = agfstedge(subg, n); e; e = agnxtedge(subg, e, n)) {
	    if (EDGEORDER(e) > 0) {
		edgelistitem *eitem;
		Agedge_t *ep;

		for (eitem = (edgelistitem *) dtfirst(openEdgeList); eitem;
		     eitem =
		     (edgelistitem *) dtnext(openEdgeList, eitem)) {
		    ep = eitem->edge;
		    if (EDGEORDER(ep) > EDGEORDER(e)) {
			if ((aghead(ep) != n) && (agtail(ep) != n))
			    crossings++;
		    }
		}
		remove_edge(openEdgeList, e);
	    }
	}

	for (e = agfstedge(subg, n); e; e = agnxtedge(subg, e, n)) {
	    if (EDGEORDER(e) == 0) {
		EDGEORDER(e) = order;
		add_edge(openEdgeList, e);
	    }
	}
	order++;
    }

    free_edgelist(openEdgeList);
    return crossings;
}

#define CROSS_ITER 10

/* reduce:
 * Attempt to reduce edge crossings by moving nodes.
 * Original crossing count is in cnt; final count is returned there.
 * list is the original list; return the best list found.
 */
static nodelist_t *reduce(nodelist_t * list, Agraph_t * subg, int *cnt)
{
    Agnode_t *curnode;
    Agedge_t *e;
    Agnode_t *neighbor;
    nodelist_t *listCopy;
    int crossings, j, newCrossings;

    crossings = *cnt;
    for (curnode = agfstnode(subg); curnode;
	 curnode = agnxtnode(subg, curnode)) {
	/*  move curnode next to its neighbors */
	for (e = agfstedge(subg, curnode); e;
	     e = agnxtedge(subg, e, curnode)) {
	    neighbor = agtail(e);
	    if (neighbor == curnode)
		neighbor = aghead(e);

	    for (j = 0; j < 2; j++) {
		listCopy = cloneNodelist(list);
		insertNodelist(list, curnode, neighbor, j);
		newCrossings = count_all_crossings(list, subg);
		if (newCrossings < crossings) {
		    crossings = newCrossings;
		    freeNodelist(listCopy);
		    if (crossings == 0) {
			*cnt = 0;
			return list;
		    }
		} else {
		    freeNodelist(list);
		    list = listCopy;
		}
	    }
	}
    }
    *cnt = crossings;
    return list;
}

static nodelist_t *reduce_edge_crossings(nodelist_t * list,
					 Agraph_t * subg)
{
    int i, crossings, origCrossings;

    crossings = count_all_crossings(list, subg);
    if (crossings == 0)
	return list;

    for (i = 0; i < CROSS_ITER; i++) {
	origCrossings = crossings;
	list = reduce(list, subg, &crossings);
	/* return if no crossings or no improvement */
	if ((origCrossings == crossings) || (crossings == 0))
	    return list;
    }
    return list;
}

/* largest_nodesize:
 * Return max dimension of nodes on list
 */
static double largest_nodesize(nodelist_t * list)
{
    Agnode_t *n;
    nodelistitem_t *item;
    double size = 0;

    for (item = list->first; item; item = item->next) {
	n = ORIGN(item->curr);
	if (ND_width(n) > size)
	    size = ND_width(n);
	if (ND_height(n) > size)
	    size = ND_height(n);
    }
    return size;
}

/* place_node:
 * Add n to list. By construction, n is not in list at start.
 */
static void place_node(Agraph_t * g, Agnode_t * n, nodelist_t * list)
{
    Agedge_t *e;
    int placed = 0;
    nodelist_t *neighbors = mkNodelist();
    nodelistitem_t *one, *two;

    for (e = agfstout(g, n); e; e = agnxtout(g, e)) {
	appendNodelist(neighbors, NULL, aghead(e));
	SET_NEIGHBOR(aghead(e));
    }
    for (e = agfstin(g, n); e; e = agnxtin(g, e)) {
	appendNodelist(neighbors, NULL, agtail(e));
	SET_NEIGHBOR(agtail(e));
    }

    /* Look for 2 neighbors consecutive on list */
    if (sizeNodelist(neighbors) >= 2) {
	for (one = list->first; one; one = one->next) {
	    if (one == list->last)
		two = list->first;
	    else
		two = one->next;

	    if (NEIGHBOR(one->curr) && NEIGHBOR(two->curr)) {
		appendNodelist(list, one, n);
		placed = 1;
		break;
	    }
	}
    }

    /* Find any neighbor on list */
    if (!placed && sizeNodelist(neighbors) > 0) {
	for (one = list->first; one; one = one->next) {
	    if (NEIGHBOR(one->curr)) {
		appendNodelist(list, one, n);
		placed = 1;
		break;
	    }
	}
    }

    if (!placed)
	appendNodelist(list, NULL, n);

    for (one = neighbors->first; one; one = one->next)
	UNSET_NEIGHBOR(one->curr);
    freeNodelist(neighbors);
}

/* place_residual_nodes:
 * Add nodes not in list to list.
 */
static void place_residual_nodes(Agraph_t * g, nodelist_t * list)
{
    Agnode_t *n;

    for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	if (!ONPATH(n))
	    place_node(g, n, list);
    }
}

nodelist_t *layout_block(Agraph_t * g, block_t * sn, double min_dist)
{
    Agnode_t *n;
    Agraph_t *copyG, *tree, *subg;
    nodelist_t *longest_path;
    nodelistitem_t *item;
    int N, k;
    double theta, radius, largest_node;
    largest_node = 0;

    subg = sn->sub_graph;
    block_graph(g, sn);		/* add induced edges */

    copyG = remove_pair_edges(subg);

    tree = spanning_tree(copyG);
    longest_path = find_longest_path(tree);
    place_residual_nodes(subg, longest_path);
    /* at this point, longest_path is a list of all nodes in the block */

    /* apply crossing reduction algorithms here */
    longest_path = reduce_edge_crossings(longest_path, subg);

    N = sizeNodelist(longest_path);
    largest_node = largest_nodesize(longest_path);
    /* N*(min_dist+largest_node) is roughly circumference of required circle */
    if (N == 1)
	radius = 0;
    else
	radius = (N * (min_dist + largest_node)) / (2 * M_PI);

    for (item = longest_path->first; item; item = item->next) {
	n = item->curr;
	if (ISPARENT(n)) {
	    /* QUESTION: Why is only one parent realigned? */
	    realignNodelist(longest_path, item);
	    break;
	}
    }

    k = 0;
    for (item = longest_path->first; item; item = item->next) {
	n = item->curr;
	POSITION(n) = k;
	PSI(n) = 0.0;
	theta = k * ((2.0 * M_PI) / N);

	ND_pos(n)[0] = radius * cos(theta);
	ND_pos(n)[1] = radius * sin(theta);

	k++;
    }

    if (N == 1)
	sn->radius = largest_node / 2;
    else
	sn->radius = radius;
    sn->rad0 = sn->radius;

    /* initialize parent pos */
    sn->parent_pos = -1;

    agclose(copyG);
    return longest_path;
}

#ifdef DEBUG
void prTree(Agraph_t * g)
{
    Agnode_t *n;

    for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	if (TPARENT(n)) {
			fprintf(stderr, "%s ", agnameof(n));
			fprintf(stderr, "-> %s\n", agnameof(TPARENT(n)));
		}
    }
}
#endif
