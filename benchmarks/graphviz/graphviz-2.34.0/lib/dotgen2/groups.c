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
#include "groups.h"
#include "utils.h"

static int comparePos(Dt_t* dt, float* key1, float* key2, Dtdisc_t* disc)
{
    float a,b;
    a = *key1;
    b = *key2;
    if(a < b) return -1;
    else return 1;
}

/*group discipline */
Dtdisc_t discGroup = {
    0,
    0,
    -1,
    NIL(Dtmake_f),
    NIL(Dtfree_f),
    NIL(Dtcompar_f),
    NIL(Dthash_f),
    NIL(Dtmemory_f),
    NIL(Dtevent_f)
};


/*node discipline , sorted set*/
Dtdisc_t discNode = {
    offsetof(mcNode,order),
    sizeof(float),
    -1,
    NIL(Dtmake_f),
    NIL(Dtfree_f),
    (Dtcompar_f)comparePos,
    NIL(Dthash_f),
    NIL(Dtmemory_f),
    NIL(Dtevent_f)
};

static mcGroup* initMcGroup(mcGroup_t type)
{
    mcGroup* rv = NEW(mcGroup);
    rv->alignType=mcBoth;
    rv->cnt=0;
    rv->nodes=dtopen(&discNode,Dtset);
#if 0
    rv->children=dtopen(&discGroup,Dtset);
#endif
    rv->type=type;
    return rv;
}

/* createGroups:
 * run after temp nodes are created
*/
void createGroups(Agraph_t* sg)
{
    mcGroup* gr;
    Agraph_t* subg;
    Agnode_t* v;
    mcNode* leader;
    mcNode* mcn;
    for (subg = agfstsubg(sg); subg; subg = agnxtsubg(subg)) 
    {
	if(is_a_cluster(subg)) {
	    int isLeader=1;
	    gr=initMcGroup(mcGrStrict);
	    for (v = agfstnode(subg); v; v = agnxtnode(subg, v)) {
		mcn = MND_mcn(v);
		dtappend(gr->nodes,mcn);
		mcn->group=gr;
		mcn->groupOrder=0;/*this will be removed eventually */
		mcn->isLeader=isLeader;

		if(isLeader==1) {
		    leader = mcn;
		    isLeader = 0;
		}
		mcn->leader = leader;
	    }
	}
	else
	    createGroups (subg);
    }
}

/* nodesToClusters:
 * If dummy nodes belong to a cluster, add them.
 */
void nodesToClusters(Agraph_t* g)
{
    Agnode_t* v;
    Agraph_t* c;
    for (v = agfstnode(g); v; v = agnxtnode(g, v)) 
    {
	if ((c = MND_highCluster(v)))
	    agsubnode(c,v,1); /*i hope this routine is optimized!*/
    }
}

/* graphGroups:
 * The highCluster attribute of a node is set to the highest containing
 * cluster, or NULL if not in a cluster.
 * Run this before creating mincross temp nodes
 */

void graphGroups(Agraph_t* sg)
{
    Agraph_t* subg;
    Agnode_t* v;
    for (subg = agfstsubg(sg); subg; subg = agnxtsubg(subg)) 
    {
	if(is_a_cluster(subg))
	    for (v = agfstnode(subg); v; v = agnxtnode(subg, v)) 
	    	MND_highCluster(v)=subg;
	else
	    graphGroups (subg);

    }
}

