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
#include "minc.h"

typedef struct {
    Dtlink_t link;
    Agedge_t *key;
    Agedge_t *val;
} edgepair_t;

static Agedge_t*
mapEdge (Dt_t* emap, Agedge_t* e)
{
     edgepair_t* ep = dtmatch (emap, &e);
     if (ep) return ep->val;
     else return NULL;
}

static int cmppair(Dt_t * d, Agedge_t** key1, Agedge_t** key2, Dtdisc_t * disc)
{
    if (*key1 > *key2) return 1;
    else if (*key1 < *key2) return -1;
    else return 0;
}

static Dtdisc_t edgepair = {
    offsetof(edgepair_t, key),
    sizeof(Agedge_t*),
    offsetof(edgepair_t, link),
    NIL(Dtmake_f),
    NIL(Dtfree_f),
    (Dtcompar_f) cmppair,
    NIL(Dthash_f),
    NIL(Dtmemory_f),
    NIL(Dtevent_f)
};

static void
cloneSubg (Agraph_t* parent, Agraph_t* sg, Dt_t* emap)
{
    Agraph_t* subg;
    Agnode_t* t;
    Agedge_t* e;
    Agnode_t* newt;
    Agedge_t* newe;
    Agraph_t* newg;

    if (is_a_cluster(sg)) {
	newg = agsubg (parent, agnameof(sg), 1);
	parent = newg; 

	for (t = agfstnode(sg); t; t = agnxtnode(sg, t)) {
	    newt = agnode(newg, agnameof(t), 0);
            agsubnode(newg, newt, 1);
	    /* if e is in sg, both end points are, so we can use out edges */
	    for (e = agfstout(sg, t); e; e = agnxtout(sg, e)) {
		newe = mapEdge (emap, e);
		agsubedge(newg, newe, 1);
	    }
	}
    }

    for (subg = agfstsubg(sg); subg; subg = agnxtsubg(subg)) {
	cloneSubg(parent, subg, emap);
    }
}

/* mkMCGraph:
 * Clone original graph. We only need the nodes, edges and clusters.
 * Copy
 */
Agraph_t* 
mkMCGraph (Agraph_t* g)
{
    Agnode_t* t;
    Agnode_t* newt;
    Agnode_t* newh;
    Agedge_t* e;
    Agedge_t* newe;
    Agraph_t* sg;
    edgepair_t* data;
    edgepair_t* ep;
    Agraph_t* newg = agopen (agnameof(g), g->desc, 0);
    Dt_t* emap = dtopen (&edgepair, Dtoset);;
    data = N_NEW(agnedges(g), edgepair_t);
    ep = data;

    for (t = agfstnode(g); t; t = agnxtnode(g, t)) {
	newt = mkMCNode (newg, STDNODE, agnameof(t));
	assert(newt);
        MND_orig(newt) = t;
        MND_rank(newt) = ND_rank(t);
    }

    for (t = agfstnode(g); t; t = agnxtnode(g, t)) {
        newt = agnode (newg, agnameof(t), 0);
	for (e = agfstout(g, t); e; e = agnxtout(g, e)) {
	    newh = agnode (newg, agnameof(aghead(e)), 0);
	    assert(newh);
            newe = mkMCEdge (newg, newt, newh, agnameof (e), NORMAL, e); 
	    assert(newe);
	    ep->key = e;
	    ep->val = newe;
	    dtinsert (emap, ep++);
	}
    }

    for (sg = agfstsubg(g); sg; sg = agnxtsubg(sg)) {
	cloneSubg(newg, sg, emap);
    }

    dtclose (emap);
    free (data);

    return newg;
}

/* mkMCNode:
    returns a new node
    unless a name is provided, depending on the node type either a temporary node or a temp label node is returned
    otherwise a regular node is returned.
    it is caller's responsibility to set necessary edges
*/
Agnode_t *mkMCNode (Agraph_t * g, int type, char *name)
{
    static int k = 0;
    Agnode_t *rv;
    char buf[512];
    switch (type) {
    case INTNODE:
	sprintf(buf, "____tempN%d", k++);	/*unique node name for internal nodes */
	rv = agnode(g, buf, 1);
	break;
    case LBLNODE:
	sprintf(buf, "____lblN%d", k++);	/*unique node name for labels */
	rv = agnode(g, buf, 1);
	break;
    default:
	rv = agnode(g, name, 1);
    }
    agbindrec(rv, "mcnodeinfo_t", sizeof(mcnodeinfo_t), TRUE);
    MND_type(rv) = type;
    return rv;
}

Agedge_t* mkMCEdge (Agraph_t* g, Agnode_t* tail, Agnode_t* head, char* name, int type, Agedge_t* orige)
{
    Agedge_t* tmpE = agedge(g, tail, head, name, 1);
    agbindrec(tmpE, "mcedgeinfo_t", sizeof(mcedgeinfo_t), TRUE);
    MED_type(tmpE) = type;
    MED_orig(tmpE) = orige;
    return tmpE;
}

