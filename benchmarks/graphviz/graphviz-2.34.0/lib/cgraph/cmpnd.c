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

#include <cghdr.h>

/*
 * provides "compound nodes" on top of base Libgraph.
 * a compound node behaves as both an ordinary node and a subgraph.
 * there are additional primitives to "hide" and "expose" its contents.
 *
 * you could think of these as hypergraphs, but there is an assymetry
 * in the operations we have chosen.  i.e. nodes "own" their edges,
 * but nodes and interior edges are "owned by" the hyperedges.
 * also the subgraphs are nested, etc.   the bottom line is that graphs
 * and hypergraphs are just sets, everything else here is convenience
 * and maintaining consistency.
 *
 * this package adds a primitive "agsplice" to move endpoints of edges.
 * this could be useful in other situations.
 */

static char Descriptor_id[] = "AG_cmpnd";

typedef struct Agcmpnode_s {
    Agrec_t hdr;
    Agraph_t *subg;
    int collapsed;
} Agcmpnode_t;

typedef struct Agcmpgraph_s {
    Agrec_t hdr;
    Agnode_t *node;		/* its associated node */
    Dict_t *hidden_node_set;
} Agcmpgraph_t;

typedef struct save_e_s {
    Agnode_t *from, *to;
} save_e_t;

typedef struct save_stack_s {
    save_e_t *mem;
    int stacksize;
} save_stack_t;

typedef struct Agcmpedge_s {
    Agrec_t hdr;
    save_stack_t stack[2];	/* IN and OUT save stacks */
} Agcmpedge_t;
#define IN_STACK 0
#define OUT_STACK 1

static save_stack_t *save_stack_of(Agedge_t * e,
				   Agnode_t * node_being_saved)
{
    int i;
    Agcmpedge_t *edgerec;
    edgerec =
	(Agcmpedge_t *) agbindrec(e, Descriptor_id, sizeof(*edgerec),
				  FALSE);
    if (node_being_saved == AGHEAD(e))
	i = IN_STACK;
    else
	i = OUT_STACK;
    return &(edgerec->stack[i]);
}

static void stackpush(save_stack_t * stk, Agnode_t * from, Agnode_t * to)
{
    int i, osize, nsize;

    osize = (stk->stacksize) * sizeof(stk->mem);
    i = stk->stacksize++;
    nsize = (stk->stacksize) * sizeof(stk->mem);
    stk->mem = agrealloc(agraphof(from), stk->mem, osize, nsize);
    stk->mem[i].from = from;
    stk->mem[i].to = to;
}

static save_e_t stacktop(save_stack_t * stk)
{
    save_e_t rv;
    if (stk->stacksize > 0)
	rv = stk->mem[stk->stacksize - 1];
    else
	rv.from = rv.to = NILnode;
    return rv;
}

/* note: doesn't give back mem, but stackpush() eventually does */
static save_e_t stackpop(save_stack_t * stk)
{
    save_e_t rv;
    rv = stacktop(stk);
    if (stk->stacksize > 0)
	stk->stacksize--;
    return rv;
}

typedef struct Agsplice_arg_s {
    int head_side;
    Agnode_t *target;
} Agsplice_arg_t;

/* perform a splice operation on an individual edge */
static void splice(Agobj_t * obj, void *arg)
{
    Agraph_t *g;
    Agedge_t *e, *opp;
    Agnode_t *target, *t, *h;
    Agedge_t **dict_of_del, **dict_of_ins, **dict_of_relabel;
    Agsplice_arg_t *spl;

    e = (Agedge_t *) obj;
    g = agraphof(e);
    t = AGTAIL(e);
    h = AGHEAD(e);
    opp = AGOPP(e);
    spl = arg;
    target = spl->target;

    /* set e to variant side, opp to invariant */
    if ((e->node == h) != spl->head_side) {
	Agedge_t *t = e;
	e = opp;
	opp = t;
    }

    if (spl->head_side) {
	dict_of_relabel = &(t->out);
	dict_of_del = &(h->in);
	dict_of_ins = &(target->in);
    } else {
	dict_of_relabel = &(h->in);
	dict_of_del = &(t->out);
	dict_of_ins = &(target->out);
    }

    agdeledgeimage(g, dict_of_del, opp);
    agdeledgeimage(g, dict_of_relabel, e);
    e->node = target;
    aginsedgeimage(g, dict_of_ins, opp);
    aginsedgeimage(g, dict_of_relabel, e);
}

int agsplice(Agedge_t * e, Agnode_t * target)
{
    Agnode_t *t, *h;
    Agraph_t *g, *root;
    Agsplice_arg_t splice_arg;


    if ((e == NILedge) || (e->node == target))
	return FAILURE;
    g = agraphof(e);
    t = AGTAIL(e);
    h = AGHEAD(e);
    splice_arg.head_side = (e->node == h);
    splice_arg.target = target;
    root = agroot(g);
    agapply(root, (Agobj_t *) e, splice, &splice_arg, TRUE);
    return SUCCESS;
}

Agnode_t *agcmpnode(Agraph_t * g, char *name)
{
    Agnode_t *n;
    Agraph_t *subg;
    n = agnode(g, name, TRUE);
    subg = agsubg(g, name);
    if (n && g && agassociate(n, subg))
	return n;
    else
	return NILnode;
}

int agassociate(Agnode_t * n, Agraph_t * sub)
{
    Agcmpnode_t *noderec;
    Agcmpgraph_t *graphrec;

    if (agsubnode(sub, n, FALSE))
	return FAILURE;		/* avoid cycles */
    noderec = agbindrec(n, Descriptor_id, sizeof(*noderec), FALSE);
    graphrec = agbindrec(sub, Descriptor_id, sizeof(*graphrec), FALSE);
    if (noderec->subg || graphrec->node)
	return FAILURE;
    noderec->subg = sub;
    graphrec->node = n;
    return SUCCESS;
}

/* a utility function for aghide */
static void delete_outside_subg(Agraph_t * g, Agnode_t * node,
				Agraph_t * subg)
{
    Agraph_t *s, **subglist;
    Agnode_t *n;
    Agcmpgraph_t *graphrec;
    Dict_t *d;

    if ((g != subg) && (n = agsubnode(g, (Agnode_t *) node, FALSE))) {
	dtdelete(g->n_dict, n);

	graphrec = agbindrec(g, Descriptor_id, sizeof(*graphrec), FALSE);
	if ((d = graphrec->hidden_node_set) == NIL(Dict_t *)) {
	    /* use name disc. to permit search for hidden node by name */
	    d = graphrec->hidden_node_set
		= agdtopen(g, &Ag_node_name_disc, Dttree);
	}
	dtinsert(d, n);

	subglist = agsubglist(g);
	while ((s = *subglist++))
	    delete_outside_subg(s, node, subg);
    }
}

/*
 *  when we hide a compound node (make it opaque)
 *	1. hide its nodes (option)
 *	2. hide the associated subgraph (option)
 *	3. remap edges to internal nodes (option)
 */
int aghide(Agnode_t * cmpnode)
{
    Agcmpnode_t *noderec;
    Agraph_t *g, *subg, *root;
    Agnode_t *n, *nn, *rootn;
    Agedge_t *e, *opp, *f;

    g = agraphof(cmpnode);
    /* skip operation if node is not compound, or hidden */
    if (agcmpgraph_of(cmpnode) == NILgraph)
	return FAILURE;
    noderec = (Agcmpnode_t *) aggetrec(cmpnode, Descriptor_id, FALSE);

    subg = noderec->subg;
    root = agroot(g);

    /* make sure caller hasn't put a node "inside itself" */
    if ((n = agsubnode(subg, cmpnode, FALSE)))
	agdelnode(n);

    /* remap edges by splicing and saving previous endpt */
    for (n = agfstnode(subg); n; n = agnxtnode(n)) {
	rootn = agsubnode(root, n, FALSE);
	for (e = agfstedge(rootn); e; e = f) {
	    f = agnxtedge(e, rootn);
	    if (agsubedge(subg, e, FALSE))
		continue;	/* an internal edge */
	    opp = AGOPP(e);
	    stackpush(save_stack_of(e, rootn), rootn, cmpnode);
	    agsplice(opp, cmpnode);
	}
    }

    /* hide nodes by deleting from the parent set.  what if they also
       belong to a sibling subg?  weird. possible bug. */
    for (n = agfstnode(subg); n; n = nn) {
	nn = agnxtnode(n);
	delete_outside_subg(root, n, subg);
    }

    /* hide subgraph is easy */
    agdelsubg(agparent(subg), subg);

    noderec->collapsed = TRUE;
    g->desc.has_cmpnd = TRUE;
    return SUCCESS;
}

/* utility function for agexpose */
static void insert_outside_subg(Agraph_t * g, Agnode_t * node,
				Agraph_t * subg)
{
    Agraph_t *s, **subglist;
    Agnode_t *n;
    Agcmpgraph_t *graphrec;

    if ((g != subg)
	&& ((n = agsubnode(g, (Agnode_t *) node, FALSE)) == NILnode)) {
	graphrec = (Agcmpgraph_t *) aggetrec(g, Descriptor_id, FALSE);
	if (graphrec
	    &&
	    ((n = (Agnode_t *) dtsearch(graphrec->hidden_node_set, node))))
	    dtinsert(g->n_dict, n);

	subglist = agsubglist(g);
	while ((s = *subglist++))
	    delete_outside_subg(s, node, subg);
    }
}

int agexpose(Agnode_t * cmpnode)
{
    Agcmpnode_t *noderec;
    Agcmpedge_t *edgerec;
    Agraph_t *g, *subg, *root;
    Agnode_t *n, *rootcmp;
    Agedge_t *e, *f;
    save_stack_t *stk;
    save_e_t sav;
    int i;

    g = agraphof(cmpnode);

    /* skip if this is not a collapsed subgraph */
    noderec = (Agcmpnode_t *) aggetrec(cmpnode, Descriptor_id, FALSE);
    if ((noderec == NIL(Agcmpnode_t *) || NOT(noderec->collapsed)))
	return FAILURE;

    /* undo aghide (above) in reverse order.  first, expose subgraph */
    subg = noderec->subg;
    agsubgrec_insert(agsubgrec(agparent(subg)), subg);

    /* re-insert nodes */
    for (n = agfstnode(subg); n; n = agnxtnode(n))
	insert_outside_subg(g, n, subg);

    /* re-splice the affected edges */
    root = agroot(g);
    rootcmp = agsubnode(root, cmpnode, FALSE);
    for (e = agfstedge(rootcmp); e; e = f) {
	f = agnxtedge(e, rootcmp);
	if ((edgerec = (Agcmpedge_t *) aggetrec(e, Descriptor_id, FALSE))) {
	    /* anything interesting on either stack? */
	    for (i = 0; i < 2; i++) {
		stk = &(edgerec->stack[i]);
		sav = stacktop(stk);
		if (sav.to && (AGID(sav.to) == AGID(cmpnode))) {
		    if (e->node != sav.to)
			e = AGOPP(e);
		    agsplice(e, sav.from);
		    stackpop(stk);
		    continue;
		}
	    }
	}
    }
    noderec->collapsed = FALSE;
    return SUCCESS;
}

Agraph_t *agcmpgraph_of(Agnode_t * n)
{
    Agcmpnode_t *noderec;
    noderec = (Agcmpnode_t *) aggetrec(n, Descriptor_id, FALSE);
    if (noderec && NOT(noderec->collapsed))
	return noderec->subg;
    else
	return NILgraph;
}

Agnode_t *agcmpnode_of(Agraph_t * g)
{
    Agcmpgraph_t *graphrec;
    graphrec = (Agcmpgraph_t *) aggetrec(g, Descriptor_id, FALSE);
    if (graphrec)
	return graphrec->node;
    else
	return NILnode;
}

Agnode_t *agfindhidden(Agraph_t * g, char *name)
{
    Agcmpgraph_t *graphrec;
    Agnode_t key;

    graphrec = (Agcmpgraph_t *) aggetrec(g, Descriptor_id, FALSE);
    if (graphrec) {
	key.name = name;
	return (Agnode_t *) dtsearch(graphrec->hidden_node_set, &key);
    } else
	return NILnode;
}
