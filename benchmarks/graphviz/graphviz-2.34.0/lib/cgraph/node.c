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

Agnode_t *agfindnode_by_id(Agraph_t * g, unsigned long id)
{
    Agsubnode_t *sn;
    static Agsubnode_t template;
    static Agnode_t dummy;

    dummy.base.tag.id = id;
    template.node = &dummy;
    sn = (Agsubnode_t *) dtsearch(g->n_id, &template);
    return sn ? sn->node : NILnode;
}

Agnode_t *agfindnode_by_name(Agraph_t * g, char *name)
{
    unsigned long id;

    if (agmapnametoid(g, AGNODE, name, &id, FALSE))
	return agfindnode_by_id(g, id);
    else
	return NILnode;
}

Agnode_t *agfstnode(Agraph_t * g)
{
    Agsubnode_t *sn;
    sn = (Agsubnode_t *) dtfirst(g->n_seq);
    return sn ? sn->node : NILnode;
}

Agnode_t *agnxtnode(Agraph_t * g, Agnode_t * n)
{
    Agsubnode_t *sn;
    sn = agsubrep(g, n);
    if (sn) sn = ((Agsubnode_t *) dtnext(g->n_seq, sn));
    return sn ? sn->node : NILnode;
}

Agnode_t *aglstnode(Agraph_t * g)
{
    Agsubnode_t *sn;
    sn = (Agsubnode_t *) dtlast(g->n_seq);
    return sn ? sn->node : NILnode;
}

Agnode_t *agprvnode(Agraph_t * g, Agnode_t * n)
{
    Agsubnode_t *sn;
    sn = agsubrep(g, n);
    if (sn) sn = ((Agsubnode_t *) dtprev(g->n_seq, sn));
    return sn ? sn->node : NILnode;
}


/* internal node constructor */
static Agnode_t *newnode(Agraph_t * g, unsigned long id, unsigned long seq)
{
    Agnode_t *n;

    n = agalloc(g, sizeof(Agnode_t));
    AGTYPE(n) = AGNODE;
    AGID(n) = id;
    AGSEQ(n) = seq;
    n->root = agroot(g);
    if (agroot(g)->desc.has_attrs)
	(void) agbindrec(n, AgDataRecName, sizeof(Agattr_t), FALSE);
    /* nodeattr_init and method_init will be called later, from the
     * subgraph where the node was actually created, but first it has
     * to be installed in all the (sub)graphs up to root. */
    return n;
}

static void installnode(Agraph_t * g, Agnode_t * n)
{
    Agsubnode_t *sn;
    int osize;

    assert(dtsize(g->n_id) == dtsize(g->n_seq));
    osize = dtsize(g->n_id);
    if (g == agroot(g)) sn = &(n->mainsub);
    else sn = agalloc(g, sizeof(Agsubnode_t));
    sn->node = n;
    dtinsert(g->n_id, sn);
    dtinsert(g->n_seq, sn);
    assert(dtsize(g->n_id) == dtsize(g->n_seq));
    assert(dtsize(g->n_id) == osize + 1);
}

static void installnodetoroot(Agraph_t * g, Agnode_t * n)
{
    Agraph_t *par;
    installnode(g, n);
    if ((par = agparent(g)))
	installnodetoroot(par, n);
}

static void initnode(Agraph_t * g, Agnode_t * n)
{
    if (agroot(g)->desc.has_attrs)
	agnodeattr_init(g,n);
    agmethod_init(g, n);
}

/* external node constructor - create by id */
Agnode_t *agidnode(Agraph_t * g, unsigned long id, int cflag)
{
    Agraph_t *root;
    Agnode_t *n;

    n = agfindnode_by_id(g, id);
    if ((n == NILnode) && cflag) {
	root = agroot(g);
	if ((g != root) && ((n = agfindnode_by_id(root, id))))	/*old */
	    agsubnode(g, n, TRUE);	/* insert locally */
	else {
	    if (agallocid(g, AGNODE, id)) {	/* new */
		n = newnode(g, id, agnextseq(g, AGNODE));
		installnodetoroot(g, n);
		initnode(g, n);
	    } else
		n = NILnode;	/* allocid for new node failed */
	}
    }
    /* else return probe result */
    return n;
}

Agnode_t *agnode(Agraph_t * g, char *name, int cflag)
{
    Agraph_t *root;
    Agnode_t *n;
    unsigned long id;

    root = agroot(g);
    /* probe for existing node */
    if (agmapnametoid(g, AGNODE, name, &id, FALSE)) {
	if ((n = agfindnode_by_id(g, id)))
	    return n;

	/* might already exist globally, but need to insert locally */
	if (cflag && (g != root) && ((n = agfindnode_by_id(root, id)))) {
	    return agsubnode(g, n, TRUE);
	}
    }

    if (cflag && agmapnametoid(g, AGNODE, name, &id, TRUE)) {	/* reserve id */
	n = newnode(g, id, agnextseq(g, AGNODE));
	installnodetoroot(g, n);
	initnode(g, n);
	assert(agsubrep(g,n));
	agregister(g, AGNODE, n); /* register in external namespace */
	return n;
    }

    return NILnode;
}

/* removes image of node and its edges from graph.
   caller must ensure n belongs to g. */
void agdelnodeimage(Agraph_t * g, Agnode_t * n, void *ignored)
{
    Agedge_t *e, *f;
    static Agsubnode_t template;
    template.node = n;

    NOTUSED(ignored);
    for (e = agfstedge(g, n); e; e = f) {
	f = agnxtedge(g, e, n);
	agdeledgeimage(g, e, 0);
    }
    /* If the following lines are switched, switch the discpline using
     * free_subnode below.
     */ 
    dtdelete(g->n_id, &template);
    dtdelete(g->n_seq, &template);
}

int agdelnode(Agraph_t * g, Agnode_t * n)
{
    Agedge_t *e, *f;

    if (!agfindnode_by_id(g, AGID(n)))
	return FAILURE;		/* bad arg */
    if (g == agroot(g)) {
	for (e = agfstedge(g, n); e; e = f) {
	    f = agnxtedge(g, e, n);
	    agdeledge(g, e);
	}
	if (g->desc.has_attrs)
	    agnodeattr_delete(n);
	agmethod_delete(g, n);
	agrecclose((Agobj_t *) n);
	agfreeid(g, AGNODE, AGID(n));
    }
    if (agapply (g, (Agobj_t *) n, (agobjfn_t) agdelnodeimage, NILnode, FALSE) == SUCCESS) {
	if (g == agroot(g))
	    agfree(g, n);
	return SUCCESS;
    } else
	return FAILURE;
}

static void dict_relabel(Agnode_t * n, void *arg)
{
    Agraph_t *g;
    unsigned long new_id;

    g = agraphof(n);
    new_id = *(unsigned long *) arg;
    dtdelete(g->n_id, n);	/* wrong, should be subrep */
    AGID(n) = new_id;
    dtinsert(g->n_id, n);	/* also wrong */
    /* because all the subgraphs share the same node now, this
       now requires a separate deletion and insertion phase */
}

int agrelabel_node(Agnode_t * n, char *newname)
{
    Agraph_t *g;
    unsigned long new_id;

    g = agroot(agraphof(n));
    if (agfindnode_by_name(g, newname))
	return FAILURE;
    if (agmapnametoid(g, AGNODE, newname, &new_id, TRUE)) {
	if (agfindnode_by_id(agroot(g), new_id) == NILnode) {
	    agfreeid(g, AGNODE, AGID(n));
	    agapply(g, (Agobj_t *) n, (agobjfn_t) dict_relabel,
		    (void *) &new_id, FALSE);
	    return SUCCESS;
	} else {
	    agfreeid(g, AGNODE, new_id);	/* couldn't use it after all */
	}
        /* obj* is unchanged, so no need to re agregister() */
    }
    return FAILURE;
}

/* lookup or insert <n> in <g> */
Agnode_t *agsubnode(Agraph_t * g, Agnode_t * n0, int cflag)
{
    Agraph_t *par;
    Agnode_t *n;

    if (agroot(g) != n0->root)
	return NILnode;
    n = agfindnode_by_id(g, AGID(n0));
    if ((n == NILnode) && cflag) {
	if ((par = agparent(g))) {
	    n = agsubnode(par, n0, cflag);
	    installnode(g, n);
	    /* no callback for existing node insertion in subgraph (?) */
	}
	/* else impossible that <n> doesn't belong to <g> */
    }
    /* else lookup succeeded */
    return n;
}

int agsubnodeidcmpf(Dict_t * d, void *arg0, void *arg1, Dtdisc_t * disc)
{
    long	v;
    Agsubnode_t *sn0, *sn1;
    sn0 = (Agsubnode_t *) arg0;
    sn1 = (Agsubnode_t *) arg1;
    v = (AGID(sn0->node) - AGID(sn1->node));
    return ((v==0)?0:(v<0?-1:1));
}

int agsubnodeseqcmpf(Dict_t * d, void *arg0, void *arg1, Dtdisc_t * disc)
{
    Agsubnode_t *sn0, *sn1;
    long	v;
    sn0 = (Agsubnode_t *) arg0;
    sn1 = (Agsubnode_t *) arg1;
    v = (AGSEQ(sn0->node) - AGSEQ(sn1->node));
    return ((v==0)?0:(v<0?-1:1));
}

/* free_subnode:
 * Free Agsubnode_t allocated in installnode. This should
 * only be done for subgraphs, as the root graph uses the
 * subnode structure built into the node. This explains the
 * AGSNMAIN test. Also, note that both the id and the seq
 * dictionaries use the same subnode object, so only one
 * should do the deletion.
 */
static void
free_subnode (Dt_t* d, Agsubnode_t* sn, Dtdisc_t * disc)
{

   if (!AGSNMAIN(sn)) 
	agfree (sn->node->root, sn);
}

Dtdisc_t Ag_subnode_id_disc = {
    0,				/* pass object ptr  */
    0,				/* size (ignored)   */
    offsetof(Agsubnode_t, id_link),	/* link offset */
    NIL(Dtmake_f),
    NIL(Dtfree_f),
    agsubnodeidcmpf,
    NIL(Dthash_f),
    agdictobjmem,
    NIL(Dtevent_f)
};

Dtdisc_t Ag_subnode_seq_disc = {
    0,				/* pass object ptr  */
    0,				/* size (ignored)   */
    offsetof(Agsubnode_t, seq_link),	/* link offset */
    NIL(Dtmake_f),
    (Dtfree_f)free_subnode,
    agsubnodeseqcmpf,
    NIL(Dthash_f),
    agdictobjmem,
    NIL(Dtevent_f)
};
