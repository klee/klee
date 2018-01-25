/* Id$ $Revision$ */
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

#include <time.h>
#ifndef WIN32
#include <unistd.h>
#endif
#include <ctype.h>

#include "neato.h"
#include "pack.h"
#include "stress.h"
#ifdef DIGCOLA
#include "digcola.h"
#endif
#include "kkutils.h"
#include "pointset.h"

#ifndef HAVE_SRAND48
#define srand48 srand
#endif

static attrsym_t *N_pos;
static int Pack;		/* If >= 0, layout components separately and pack together
				 * The value of Pack gives margins around graphs.
				 */
static char *cc_pfx = "_neato_cc";

void neato_init_node(node_t * n)
{
#ifdef WITH_CGRAPH
    agbindrec(n, "Agnodeinfo_t", sizeof(Agnodeinfo_t), TRUE);	//node custom data
#endif /* WITH_CGRAPH */
    common_init_node(n);
    ND_pos(n) = N_NEW(GD_ndim(agraphof(n)), double);
    gv_nodesize(n, GD_flip(agraphof(n)));
}

static void neato_init_edge(edge_t * e)
{
#ifdef WITH_CGRAPH
	agbindrec(e, "Agedgeinfo_t", sizeof(Agedgeinfo_t), TRUE);	//node custom data
#endif /* WITH_CGRAPH */
    common_init_edge(e);
#ifndef WITH_CGRAPH

#endif /* WITH_CGRAPH */
    ED_factor(e) = late_double(e, E_weight, 1.0, 1.0);
}

int user_pos(attrsym_t * posptr, attrsym_t * pinptr, node_t * np, int nG)
{
    double *pvec;
    char *p, c;
    double z;

    if (posptr == NULL)
	return FALSE;
    pvec = ND_pos(np);
#ifndef WITH_CGRAPH
    p = agxget(np, posptr->index);
#else
    p = agxget(np, posptr);
#endif
    if (p[0]) {
	c = '\0';
	if ((Ndim >= 3) && 
            (sscanf(p, "%lf,%lf,%lf%c", pvec, pvec+1, pvec+2, &c) >= 3)){
	    ND_pinned(np) = P_SET;
	    if (PSinputscale > 0.0) {
		int i;
		for (i = 0; i < Ndim; i++)
		    pvec[i] = pvec[i] / PSinputscale;
	    }
	    if (Ndim > 3)
		jitter_d(np, nG, 3);
#ifndef WITH_CGRAPH
	    if ((c == '!') || (pinptr && mapbool(agxget(np, pinptr->index))))
#else
	    if ((c == '!') || (pinptr && mapbool(agxget(np, pinptr))))
#endif
		ND_pinned(np) = P_PIN;
	    return TRUE;
	}
	else if (sscanf(p, "%lf,%lf%c", pvec, pvec + 1, &c) >= 2) {
	    ND_pinned(np) = P_SET;
	    if (PSinputscale > 0.0) {
		int i;
		for (i = 0; i < Ndim; i++)
		    pvec[i] = pvec[i] / PSinputscale;
	    }
	    if (Ndim > 2) {
#ifndef WITH_CGRAPH
		if (N_z && (p = agxget(np, N_z->index)) && (sscanf(p,"%lf",&z) == 1)) { 
#else
		if (N_z && (p = agxget(np, N_z)) && (sscanf(p,"%lf",&z) == 1)) { 
#endif
		    if (PSinputscale > 0.0) {
			pvec[2] = z / PSinputscale;
		    }
		    else
			pvec[2] = z;
		    jitter_d(np, nG, 3);
		}
		else
		    jitter3d(np, nG);
	    }
#ifndef WITH_CGRAPH
	    if ((c == '!') || (pinptr && mapbool(agxget(np, pinptr->index))))
#else
	    if ((c == '!') || (pinptr && mapbool(agxget(np, pinptr))))
#endif
		ND_pinned(np) = P_PIN;
	    return TRUE;
	} else
	    agerr(AGERR, "node %s, position %s, expected two doubles\n",
		  agnameof(np), p);
    }
    return FALSE;
}

static void neato_init_node_edge(graph_t * g)
{
    node_t *n;
    edge_t *e;
    int nG = agnnodes(g);
    attrsym_t *N_pin;

    N_pos = agfindnodeattr(g, "pos");
    N_pin = agfindnodeattr(g, "pin");

    for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	neato_init_node(n);
	user_pos(N_pos, N_pin, n, nG);	/* set user position if given */
    }
    for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	for (e = agfstout(g, n); e; e = agnxtout(g, e))
	    neato_init_edge(e);
    }
}

static void neato_cleanup_graph(graph_t * g)
{
    if (Nop || (Pack < 0))
	free_scan_graph(g);
    if (g != agroot(g))
#ifndef WITH_CGRAPH
        memset(&(g->u), 0, sizeof(Agraphinfo_t));
#else /* WITH_CGRAPH */
        agclean(g, AGRAPH , "Agraphinfo_t");
#endif /* WITH_CGRAPH */
}

void neato_cleanup(graph_t * g)
{
    node_t *n;
    edge_t *e;

    for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	for (e = agfstout(g, n); e; e = agnxtout(g, e)) {
	    gv_cleanup_edge(e);
	}
	gv_cleanup_node(n);
    }
    neato_cleanup_graph(g);
}

static int numFields(unsigned char *pos)
{
    int cnt = 0;
    unsigned char c;

    do {
	while (isspace(*pos))
	    pos++;		/* skip white space */
	if ((c = *pos)) { /* skip token */
	    cnt++;
	    while ((c = *pos) && !isspace(c) && (c != ';'))
		pos++;
	}
    } while (isspace(c));
    return cnt;
}

static void set_label(void* obj, textlabel_t * l, char *name)
{
    double x, y;
    char *lp;
    lp = agget(obj, name);
    if (lp && (sscanf(lp, "%lf,%lf", &x, &y) == 2)) {
	l->pos = pointfof(x, y);
	l->set = TRUE;
    }
}

#ifdef IPSEPCOLA
static cluster_data* cluster_map(graph_t *mastergraph, graph_t *g)
{
#ifndef WITH_CGRAPH
    /* search meta-graph to find clusters */
    graph_t *mg;
    node_t *mm, *mn;
    edge_t *me;
#endif
    graph_t *subg;
    node_t *n;
     /* array of arrays of node indices in each cluster */
    int **cs,*cn;
    int i,j,nclusters=0;
    boolean* assigned = N_NEW(agnnodes(g), boolean);
    cluster_data *cdata = GNEW(cluster_data);

    cdata->ntoplevel = agnnodes(g);
#ifndef WITH_CGRAPH
    mm = mastergraph->meta_node;
    mg = agraphof(mm); 
    for (me = agfstout(mg, mm); me; me = agnxtout(mg, me)) {
        mn = aghead(me);
        subg = agusergraph(mn);
#else
    for (subg = agfstsubg(mastergraph); subg; subg = agnxtsubg(subg)) {
#endif
        if (!strncmp(agnameof(subg), "cluster", 7)) {
            nclusters++;
        }
    }
    cdata->nvars=0;
    cdata->nclusters = nclusters;
    cs = cdata->clusters = N_GNEW(nclusters,int*);
    cn = cdata->clustersizes = N_GNEW(nclusters,int);
#ifndef WITH_CGRAPH
    /* fprintf(stderr,"search %d clusters...\n",nclusters); */
    for (me = agfstout(mg, mm); me; me = agnxtout(mg, me)) {
        mn = me->head;
        subg = agusergraph(mn);
#else
    for (subg = agfstsubg(mastergraph); subg; subg = agnxtsubg(subg)) {
#endif
        /* clusters are processed by separate calls to ordered_edges */
        if (!strncmp(agnameof(subg), "cluster", 7)) {
            int *c;

            *cn = agnnodes(subg);
            cdata->nvars += *cn;
            c = *cs++ = N_GNEW(*cn++,int);
            /* fprintf(stderr,"Cluster with %d nodes...\n",agnnodes(subg)); */
            for (n = agfstnode(subg); n; n = agnxtnode(subg, n)) {
                node_t *gn;
                int ind = 0;
                for (gn = agfstnode(g); gn; gn = agnxtnode(g, gn)) {
                    if(AGSEQ(gn)==AGSEQ(n)) break;
                    ind++;
                }
                /* fprintf(stderr,"  node=%s, id=%d, ind=%d\n",agnameof(n),n->id,ind); */
                *c++=ind;
                assigned[ind]=TRUE;
                cdata->ntoplevel--;
            }
        }
    }
    cdata->bb=N_GNEW(cdata->nclusters,boxf);
    cdata->toplevel=N_GNEW(cdata->ntoplevel,int);
    for(i=j=0;i<agnnodes(g);i++) {
        if(!assigned[i]) {
            cdata->toplevel[j++]=i;
        }
    }
    assert(cdata->ntoplevel==agnnodes(g)-cdata->nvars);
    free (assigned);
    return cdata;
}

static void freeClusterData(cluster_data *c) {
    if(c->nclusters>0) {
        free(c->clusters[0]);
        free(c->clusters);
        free(c->clustersizes);
        free(c->toplevel);
        free(c->bb);
    }
    free(c);
}
#endif

/* user_spline:
 * Attempt to use already existing pos info for spline
 * Return 1 if successful, 0 otherwise.
 * Assume E_pos != NULL and ED_spl(e) == NULL.
 */
static int user_spline(attrsym_t * E_pos, edge_t * e)
{
    char *pos;
    int i, n, npts, nc;
    pointf *ps = 0;
    pointf *pp;
    double x, y;
    int sflag = 0, eflag = 0;
    pointf sp = { 0, 0 }, ep = { 0, 0};
    bezier *newspl;
    int more = 1;
    int stype, etype;
    static boolean warned;

#ifndef WITH_CGRAPH
    pos = agxget(e, E_pos->index);
#else
    pos = agxget(e, E_pos);
#endif
    if (*pos == '\0')
	return 0;

    arrow_flags(e, &stype, &etype);
    do {
	/* check for s head */
	i = sscanf(pos, "s,%lf,%lf%n", &x, &y, &nc);
	if (i == 2) {
	    sflag = 1;
	    pos = pos + nc;
	    sp.x = x;
	    sp.y = y;
	}

	/* check for e head */
	i = sscanf(pos, " e,%lf,%lf%n", &x, &y, &nc);
	if (i == 2) {
	    eflag = 1;
	    pos = pos + nc;
	    ep.x = x;
	    ep.y = y;
	}

	npts = numFields((unsigned char *) pos);	/* count potential points */
	n = npts;
	if ((n < 4) || (n % 3 != 1)) {
	    gv_free_splines(e);
	    if (!warned) {
		warned = 1;
#ifndef WITH_CGRAPH
		agerr(AGWARN, "pos attribute for edge (%s,%s) doesn't have 3n+1 points\n", e->tail->name, e->head->name);
#else
		agerr(AGWARN, "pos attribute for edge (%s,%s) doesn't have 3n+1 points\n", agnameof(agtail(e)), agnameof(aghead(e)));
#endif
	    }
	    return 0;
	}
	ps = ALLOC(n, 0, pointf);
	pp = ps;
	while (n) {
	    i = sscanf(pos, "%lf,%lf%n", &x, &y, &nc);
	    if (i < 2) {
		if (!warned) {
		    warned = 1;
#ifndef WITH_CGRAPH
		    agerr(AGWARN, "syntax error in pos attribute for edge (%s,%s)\n", e->tail->name, e->head->name);
#else
		    agerr(AGWARN, "syntax error in pos attribute for edge (%s,%s)\n", agnameof(agtail(e)), agnameof(aghead(e)));
#endif
		}
		free(ps);
		gv_free_splines(e);
		return 0;
	    }
	    pos = pos + nc;
	    pp->x = x;
	    pp->y = y;
	    pp++;
	    n--;
	}
 	while (isspace(*pos)) pos++;
	if (*pos == '\0')
	    more = 0;
	else
	    pos++;

	/* parsed successfully; create spline */
	newspl = new_spline(e, npts);
	if (sflag) {
	    newspl->sflag = stype;
	    newspl->sp = sp;
	}
	if (eflag) {
	    newspl->eflag = etype;
	    newspl->ep = ep;
	}
	for (i = 0; i < npts; i++) {
	    newspl->list[i] = ps[i];
	}
	free(ps);
    } while (more);

    if (ED_label(e))
	set_label(e, ED_label(e), "lp");
    if (ED_xlabel(e))
	set_label(e, ED_xlabel(e), "xlp");
    if (ED_head_label(e))
	set_label(e, ED_head_label(e), "head_lp");
    if (ED_tail_label(e))
	set_label(e, ED_tail_label(e), "tail_lp");

    return 1;
}

/* Nop can be:
 *  0 - do full layout
 *  1 - assume initial node positions, do (optional) adjust and all splines
 *  2 - assume final node and edges positions, do nothing except compute
 *      missing splines
 */

 /* Indicates the amount of edges with position information */
typedef enum { NoEdges, SomeEdges, AllEdges } pos_edge;

/* nop_init_edges:
 * Check edges for position info.
 * If position info exists, check for edge label positions.
 * Return number of edges with position info.
 */
static pos_edge nop_init_edges(Agraph_t * g)
{
    node_t *n;
    edge_t *e;
    int nedges = 0;
    attrsym_t *E_pos;

    if (agnedges(g) == 0)
	return AllEdges;

    E_pos = agfindedgeattr(g, "pos");
    if (!E_pos || (Nop < 2))
	return NoEdges;

    for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	for (e = agfstout(g, n); e; e = agnxtout(g, e)) {
	    if (user_spline(E_pos, e)) {
		nedges++;
	    }
	}
    }
    if (nedges) {
	if (nedges == agnedges(g))
	    return AllEdges;
	else
	    return SomeEdges;
    } else
	return NoEdges;
}

/* chkBB:
 * Scans for a correct bb attribute. If available, sets it
 * in the graph and returns 1.
 */
#define BS "%lf,%lf,%lf,%lf"

static int chkBB(Agraph_t * g, attrsym_t * G_bb, boxf* bbp)
{
    char *s;
    boxf bb;

#ifndef WITH_CGRAPH
    s = agxget(g, G_bb->index);
#else
    s = agxget(g, G_bb);
#endif
    if (sscanf(s, BS, &bb.LL.x, &bb.LL.y, &bb.UR.x, &bb.UR.y) == 4) {
	if (bb.LL.y > bb.UR.y) {
	/* If the LL.y coordinate is bigger than the UR.y coordinate,
         * we assume the input was produced using -y, so we normalize
	 * the bb. 
	 */
	    double tmp = bb.LL.y;
	    bb.LL.y = bb.UR.y;
	    bb.UR.y = tmp;
	}
	*bbp = bb;
	return 1;
    } else
	return 0;
}

static void add_cluster(Agraph_t * g, Agraph_t * subg)
{
    int cno;
    cno = ++(GD_n_cluster(g));
    GD_clust(g) = ZALLOC(cno + 1, GD_clust(g), graph_t *, GD_n_cluster(g));
    GD_clust(g)[cno] = subg;
    do_graph_label(subg);
}


static void nop_init_graphs(Agraph_t *, attrsym_t *, attrsym_t *);

/* dfs:
 * Process subgraph subg of parent graph g
 * If subg is a cluster, add its bounding box, if any; attach to
 * cluster array of parent, and recursively initialize subg.
 * If not a cluster, recursively call this function on the subgraphs
 * of subg, using parentg as the parent graph.
 */
static void
#ifndef WITH_CGRAPH
dfs(node_t * mn, Agraph_t * parentg, attrsym_t * G_lp, attrsym_t * G_bb)
#else /* WITH_CGRAPH */
dfs(Agraph_t * subg, Agraph_t * parentg, attrsym_t * G_lp, attrsym_t * G_bb)
#endif /* WITH_CGRAPH */
{
    boxf bb;

#ifndef WITH_CGRAPH
    graph_t *subg = agusergraph(mn);
#endif
    if (!strncmp(agnameof(subg), "cluster", 7) && chkBB(subg, G_bb, &bb)) {
#ifdef WITH_CGRAPH
	agbindrec(subg, "Agraphinfo_t", sizeof(Agraphinfo_t), TRUE);
#endif
	GD_bb(subg) = bb;
	add_cluster(parentg, subg);
	nop_init_graphs(subg, G_lp, G_bb);
    } else {
#ifndef WITH_CGRAPH
	graph_t *mg = parentg->meta_node->graph;
	edge_t *me;
	for (me = agfstout(mg, mn); me; me = agnxtout(mg, me)) {
	    dfs(me->head, parentg, G_lp, G_bb);
#else
	graph_t *sg;
	for (sg = agfstsubg(subg); sg; sg = agnxtsubg(sg)) {
	    dfs(sg, parentg, G_lp, G_bb);
#endif
	}
    }
}

/* nop_init_graphs:
 * Read in clusters and graph label info.
 * A subgraph is a cluster if its name starts with "cluster" and
 * it has a valid bb.
 */
static void
nop_init_graphs(Agraph_t * g, attrsym_t * G_lp, attrsym_t * G_bb)
{
#ifndef WITH_CGRAPH
    graph_t *mg;
    edge_t *me;
#else
    graph_t *subg;
#endif
    char *s;
    double x, y;

    if (GD_label(g) && G_lp) {
#ifndef WITH_CGRAPH
	s = agxget(g, G_lp->index);
#else
	s = agxget(g, G_lp);
#endif
	if (sscanf(s, "%lf,%lf", &x, &y) == 2) {
	    GD_label(g)->pos = pointfof(x, y);
	    GD_label(g)->set = TRUE;
	}
    }

    if (!G_bb)
	return;
#ifndef WITH_CGRAPH
    mg = g->meta_node->graph;
    for (me = agfstout(mg, g->meta_node); me; me = agnxtout(mg, me)) {
	dfs(me->head, g, G_lp, G_bb);
#else /* WITH_CGRAPH */
    for (subg = agfstsubg(g); subg; subg = agnxtsubg(subg)) {
	dfs(subg, g, G_lp, G_bb);
#endif /* WITH_CGRAPH */
    }
}

/* translateE:
 * Translate edge by offset.
 * Assume ED_spl(e) != NULL
 */
static void translateE(edge_t * e, pointf offset)
{
    int i, j;
    pointf *pt;
    bezier *bez;

    bez = ED_spl(e)->list;
    for (i = 0; i < ED_spl(e)->size; i++) {
	pt = bez->list;
	for (j = 0; j < bez->size; j++) {
	    pt->x -= offset.x;
	    pt->y -= offset.y;
	    pt++;
	}
	if (bez->sflag) {
	    bez->sp.x -= offset.x;
	    bez->sp.y -= offset.y;
	}
	if (bez->eflag) {
	    bez->ep.x -= offset.x;
	    bez->ep.y -= offset.y;
	}
	bez++;
    }

    if (ED_label(e) && ED_label(e)->set) {
	ED_label(e)->pos.x -= offset.x;
	ED_label(e)->pos.y -= offset.y;
    }
    if (ED_xlabel(e) && ED_xlabel(e)->set) {
	ED_xlabel(e)->pos.x -= offset.x;
	ED_xlabel(e)->pos.y -= offset.y;
    }
    if (ED_head_label(e) && ED_head_label(e)->set) {
	ED_head_label(e)->pos.x -= offset.x;
	ED_head_label(e)->pos.y -= offset.y;
    }
    if (ED_tail_label(e) && ED_tail_label(e)->set) {
	ED_tail_label(e)->pos.x -= offset.x;
	ED_tail_label(e)->pos.y -= offset.y;
    }
}

/* translateG:
 */
static void translateG(Agraph_t * g, pointf offset)
{
    int i;

    GD_bb(g).UR.x -= offset.x;
    GD_bb(g).UR.y -= offset.y;
    GD_bb(g).LL.x -= offset.x;
    GD_bb(g).LL.y -= offset.y;

    if (GD_label(g) && GD_label(g)->set) {
	GD_label(g)->pos.x -= offset.x;
	GD_label(g)->pos.y -= offset.y;
    }

    for (i = 1; i <= GD_n_cluster(g); i++)
	translateG(GD_clust(g)[i], offset);
}

/* translate:
 */
static void translate(Agraph_t * g, pos_edge posEdges)
{
    node_t *n;
    edge_t *e;
    pointf offset;
    pointf ll;

    ll = GD_bb(g).LL;

    offset.x = PS2INCH(ll.x);
    offset.y = PS2INCH(ll.y);
    for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	ND_pos(n)[0] -= offset.x;
	ND_pos(n)[1] -= offset.y;
    }
    if (posEdges != NoEdges) {
	for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	    for (e = agfstout(g, n); e; e = agnxtout(g, e))
		if (ED_spl(e))
		    translateE(e, ll);
	}
    }
    translateG(g, ll);
}

/* init_nop:
 * This assumes all nodes have been positioned.
 * It also assumes none of the * relevant fields in A*info_t have been set.
 * The input may provide additional position information for
 * clusters, edges and labels. If certain position information
 * is missing, init_nop will use a standard neato technique to
 * supply it.
 *
 * If adjust is false, init_nop does nothing but initialize all
 * of the basic graph information. No tweaking of positions or 
 * filling in edge splines is done.
 *
 * Returns 0 on normal success, 1 if postprocess should be avoided, and -1
 * on failure.
 */
int init_nop(Agraph_t * g, int adjust)
{
    int i;
    node_t *np;
    pos_edge posEdges;		/* How many edges have spline info */
    attrsym_t *G_lp = agfindgraphattr(g, "lp");
    attrsym_t *G_bb = agfindgraphattr(g, "bb");
    int didAdjust = 0;  /* Have nodes been moved? */
    int haveBackground;

    /* If G_bb not defined, define it */
    if (!G_bb)
#ifndef WITH_CGRAPH
	G_bb = agraphattr(g, "bb", "");
#else /* WITH_CGRAPH */
	G_bb = agattr(g, AGRAPH, "bb", "");
#endif /* WITH_CGRAPH */

    scan_graph(g);		/* mainly to set up GD_neato_nlist */
    for (i = 0; (np = GD_neato_nlist(g)[i]); i++) {
	if (!hasPos(np) && strncmp(agnameof(np), "cluster", 7)) {
	    agerr(AGERR, "node %s in graph %s has no position\n",
		  agnameof(np), agnameof(g));
	    return -1;
	}
	if (ND_xlabel(np))
	    set_label(np, ND_xlabel(np), "xlp");
    }
    nop_init_graphs(g, G_lp, G_bb);
    posEdges = nop_init_edges(g);

    if (GD_drawing(g)->xdots) {
	haveBackground = 1;
	GD_drawing(g)->ratio_kind = R_NONE; /* Turn off any aspect change if background present */
    }
    else
	haveBackground = 0;

    if (adjust && (Nop == 1) && !haveBackground)
	didAdjust = adjustNodes(g);

    if (didAdjust) {
	if (GD_label(g)) GD_label(g)->set = FALSE;
/* FIX: 
 *   - if nodes are moved, clusters are no longer valid.
 */
    }

#if 0
    /* If g does not have a good "bb" attribute or we adjusted the nodes, 
     * compute it. 
     */
    if (!chkBB(g, G_bb) || didAdjust)
#endif
	compute_bb(g);

    /* Adjust bounding box for any background */
    if (haveBackground)
	GD_bb(g) = xdotBB (g);

    /* At this point, all bounding boxes should be correctly defined.
     * If necessary, we translate the graph to the origin.
     */
    if (adjust && !haveBackground && (ROUND(abs(GD_bb(g).LL.x)) || ROUND(abs(GD_bb(g).LL.y))))
	translate(g, posEdges);

    if (!adjust) {
	node_t *n;
	State = GVSPLINES;
	for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	    ND_coord(n).x = POINTS_PER_INCH * (ND_pos(n)[0]);
	    ND_coord(n).y = POINTS_PER_INCH * (ND_pos(n)[1]);
	}
    }
    else if (posEdges != AllEdges)
	spline_edges0(g);
    else {
	State = GVSPLINES;
	neato_set_aspect(g);
    }
    return haveBackground;
}

static void neato_init_graph (Agraph_t * g)
{
    int outdim;

    setEdgeType (g, ET_LINE);
    outdim = late_int(g, agfindgraphattr(g, "dimen"), 2, 2);
    GD_ndim(agroot(g)) = late_int(g, agfindgraphattr(g, "dim"), outdim, 2);
    Ndim = GD_ndim(g->root) = MIN(GD_ndim(g->root), MAXDIM);
    GD_odim(g->root) = MIN(outdim, Ndim);
    neato_init_node_edge(g);
}

static int neatoModel(graph_t * g)
{
    char *p = agget(g, "model");
    char c;

    if (!p || (!(c = *p)))    /* if p is NULL or "" */
	return MODEL_SHORTPATH;
    if ((c == 'c') && streq(p, "circuit"))
	return MODEL_CIRCUIT;
    if (c == 's') {
	if (streq(p, "subset"))
	    return MODEL_SUBSET;
	else if (streq(p, "shortpath"))
	    return MODEL_SHORTPATH;
    }
    if ((c == 'm') && streq(p, "mds")) {
#ifndef WITH_CGRAPH
	if (agindex(g->root->proto->e, "len") >= 0)
#else /* WITH_CGRAPH */
	if (agattr(g, AGEDGE, "len", 0))
#endif /* WITH_CGRAPH */
	    return MODEL_MDS;
	else {
	    agerr(AGWARN,
	        "edges in graph %s have no len attribute. Hence, the mds model\n", agnameof(g));
	    agerr(AGPREV, "is inappropriate. Reverting to the shortest path model.\n");
	    return MODEL_SHORTPATH;
	}
    }
    agerr(AGWARN,
	  "Unknown value %s for attribute \"model\" in graph %s - ignored\n",
	  p, agnameof(g));
    return MODEL_SHORTPATH;
}

/* neatoMode:
 */
static int neatoMode(graph_t * g)
{
    char *str;
    int mode = MODE_MAJOR;	/* default mode */

    str = agget(g, "mode");
    if (str && *str) {
	if (streq(str, "KK"))
	    mode = MODE_KK;
	else if (streq(str, "major"))
	    mode = MODE_MAJOR;
#ifdef DIGCOLA
	else if (streq(str, "hier"))
	    mode = MODE_HIER;
#ifdef IPSEPCOLA
        else if (streq(str, "ipsep"))
            mode = MODE_IPSEP;
#endif
#endif
	else
	    agerr(AGWARN,
		  "Illegal value %s for attribute \"mode\" in graph %s - ignored\n",
		  str, agnameof(g));
    }

    return mode;
}

/* checkEdge:
 *
 */
static int checkEdge(PointMap * pm, edge_t * ep, int idx)
{
    int i = ND_id(agtail(ep));
    int j = ND_id(aghead(ep));
    int tmp;

    if (i > j) {
	tmp = i;
	i = j;
	j = tmp;
    }
    return insertPM(pm, i, j, idx);
}

#ifdef DIGCOLA
/* dfsCycle:
 * dfs for breaking cycles in vtxdata
 */
static void
dfsCycle (vtx_data* graph, int i,int mode, node_t* nodes[])
{
    node_t *np, *hp;
    int j, e, f;
    /* if mode is IPSEP make it an in-edge
     * at both ends, so that an edge constraint won't be generated!
     */
    double x = (mode==MODE_IPSEP?-1.0:1.0);

    np = nodes[i];
    ND_mark(np) = TRUE;
    ND_onstack(np) = TRUE;
    for (e = 1; e < graph[i].nedges; e++) {
	if (graph[i].edists[e] == 1.0) continue;  /* in edge */
	j = graph[i].edges[e];
	hp = nodes[j];
	if (ND_onstack(hp)) {  /* back edge: reverse it */
            graph[i].edists[e] = x;
            for (f = 1; (f < graph[j].nedges) &&(graph[j].edges[f] != i); f++) ;
            assert (f < graph[j].nedges);
            graph[j].edists[f] = -1.0;
        }
	else if (ND_mark(hp) == FALSE) dfsCycle(graph, j, mode, nodes);

    }
    ND_onstack(np) = FALSE;
}

/* acyclic:
 * Do a dfs of the vtx_data, looking for cycles, reversing edges.
 */
static void
acyclic (vtx_data* graph, int nv, int mode, node_t* nodes[])
{
    int i;
    node_t* np;

    for (i = 0; i < nv; i++) {
	np = nodes[i];
	ND_mark(np) = FALSE;
	ND_onstack(np) = FALSE;
    }
    for (i = 0; i < nv; i++) {
	if (ND_mark(nodes[i])) continue;
	dfsCycle (graph, i, mode, nodes);	
    }

}
#endif

/* makeGraphData:
 * Create sparse graph representation via arrays.
 * Each node is represented by a vtx_data.
 * The index of each neighbor is stored in the edges array;
 * the corresponding edge lengths and weights go on ewgts and eweights.
 * We do not allocate the latter 2 if the graph does not use them.
 * By convention, graph[i].edges[0] == i.
 * The values graph[i].ewgts[0] and graph[i].eweights[0] are left undefined.
 *
 * In constructing graph from g, we neglect loops. We track multiedges (ignoring
 * direction). Edge weights are additive; the final edge length is the max.
 *
 * If direction is used, we set the edists field, -1 for tail, +1 for head. 
 * graph[i].edists[0] is left undefined. If multiedges exist, the direction
 * of the first one encountered is used. Finally, a pass is made to guarantee
 * the graph is acyclic.
 *
 */
static vtx_data *makeGraphData(graph_t * g, int nv, int *nedges, int mode, int model, node_t*** nodedata)
{
    vtx_data *graph;
    node_t** nodes;
    int ne = agnedges(g);	/* upper bound */
    int *edges;
    float *ewgts = NULL;
    node_t *np;
    edge_t *ep;
    float *eweights = NULL;
#ifdef DIGCOLA
    float *edists = NULL;
#endif
#ifndef WITH_CGRAPH
    int haveLen;
#else
    attrsym_t *haveLen;
#endif
    int haveWt;
    int haveDir;
    PointMap *ps = newPM();
    int i, i_nedges, idx;

    /* lengths and weights unused in reweight model */
    if (model == MODEL_SUBSET) {
	haveLen = FALSE;
	haveWt = FALSE;
    } else {
#ifndef WITH_CGRAPH
	haveLen = (agindex(g->root->proto->e, "len") >= 0);
#else /* WITH_CGRAPH */
	haveLen = agattr(g, AGEDGE, "len", 0) ;
#endif /* WITH_CGRAPH */
	haveWt = (E_weight != 0);
    }
    if (mode == MODE_HIER || mode == MODE_IPSEP)
	haveDir = TRUE;
    else
	haveDir = FALSE;

    graph = N_GNEW(nv, vtx_data);
    nodes = N_GNEW(nv, node_t*);
    edges = N_GNEW(2 * ne + nv, int);	/* reserve space for self loops */
    if (haveLen || haveDir)
	ewgts = N_GNEW(2 * ne + nv, float);
    if (haveWt)
	eweights = N_GNEW(2 * ne + nv, float);
#ifdef DIGCOLA
    if (haveDir)
	edists = N_GNEW(2*ne+nv,float);
#endif

    i = 0;
    ne = 0;
    for (np = agfstnode(g); np; np = agnxtnode(g, np)) {
	int j = 1;		/* index of neighbors */
	clearPM(ps);
	assert(ND_id(np) == i);
	nodes[i] = np;
	graph[i].edges = edges++;	/* reserve space for the self loop */
	if (haveLen || haveDir)
	    graph[i].ewgts = ewgts++;
	else
	    graph[i].ewgts = NULL;
	if (haveWt)
	    graph[i].eweights = eweights++;
	else
	    graph[i].eweights = NULL;
#ifdef DIGCOLA
	if (haveDir) {
	    graph[i].edists = edists++;
	}
	else
	    graph[i].edists = NULL;
#endif
	i_nedges = 1;		/* one for the self */

	for (ep = agfstedge(g, np); ep; ep = agnxtedge(g, ep, np)) {
	    if (aghead(ep) == agtail(ep))
		continue;	/* ignore loops */
	    idx = checkEdge(ps, ep, j);
	    if (idx != j) {	/* seen before */
		if (haveWt)
		    graph[i].eweights[idx] += ED_factor(ep);
		if (haveLen) {
		    int curlen = graph[i].ewgts[idx];
		    graph[i].ewgts[idx] = MAX(ED_dist(ep), curlen);
		}
	    } else {
		node_t *vp = (((agtail(ep)) == np) ? aghead(ep) : agtail(ep));
		ne++;
		j++;

		*edges++ = ND_id(vp);
		if (haveWt)
		    *eweights++ = ED_factor(ep);
		if (haveLen)
		    *ewgts++ = ED_dist(ep);
		else if (haveDir)
		    *ewgts++ = 1.0;
#ifdef DIGCOLA
                if (haveDir) {
                    char *s = agget(ep,"dir");
                    if(s&&!strncmp(s,"none",4)) {
                        *edists++ = 0;
                    } else {
                        *edists++ = (np == aghead(ep) ? 1.0 : -1.0);
                    }
                }
#endif
		i_nedges++;
	    }
	}

	graph[i].nedges = i_nedges;
	graph[i].edges[0] = i;
#ifdef USE_STYLES
	graph[i].styles = NULL;
#endif
	i++;
    }
#ifdef DIGCOLA
    if (haveDir) {
    /* Make graph acyclic */
	acyclic (graph, nv, mode, nodes);
    }
#endif

    ne /= 2;			/* every edge is counted twice */

    /* If necessary, release extra memory. */
    if (ne != agnedges(g)) {
	edges = RALLOC(2 * ne + nv, graph[0].edges, int);
	if (haveLen)
	    ewgts = RALLOC(2 * ne + nv, graph[0].ewgts, float);
	if (haveWt)
	    eweights = RALLOC(2 * ne + nv, graph[0].eweights, float);

	for (i = 0; i < nv; i++) {
	    int sz = graph[i].nedges;
	    graph[i].edges = edges;
	    edges += sz;
	    if (haveLen) {
		graph[i].ewgts = ewgts;
		ewgts += sz;
	    }
	    if (haveWt) {
		graph[i].eweights = eweights;
		eweights += sz;
	    }
	}
    }

    *nedges = ne;
    if (nodedata)
	*nodedata = nodes;
    else
	free (nodes);
    freePM(ps);
    return graph;
}

static void initRegular(graph_t * G, int nG)
{
    double a, da;
    node_t *np;

    a = 0.0;
    da = (2 * M_PI) / nG;
    for (np = agfstnode(G); np; np = agnxtnode(G, np)) {
	ND_pos(np)[0] = nG * Spring_coeff * cos(a);
	ND_pos(np)[1] = nG * Spring_coeff * sin(a);
	ND_pinned(np) = P_SET;
	a = a + da;
	if (Ndim > 2)
	    jitter3d(np, nG);
    }
}

#define SLEN(s) (sizeof(s)-1)
#define SMART   "self"
#define REGULAR "regular"
#define RANDOM  "random"

/* setSeed:
 * Analyze "start" attribute. If unset, return dflt.
 * If it begins with self, regular, or random, return set init to same,
 * else set init to dflt.
 * If init is random, look for value integer suffix to use a seed; if not
 * found, use time to set seed and store seed in graph.
 * Return seed in seedp.
 * Return init.
 */
int
setSeed (graph_t * G, int dflt, long* seedp)
{
    char smallbuf[32];
    char *p = agget(G, "start");
    int init = dflt;

    if (!p || (*p == '\0')) return dflt;
    if (isalpha(*(unsigned char *)p)) {
	if (!strncmp(p, SMART, SLEN(SMART))) {
	    init = INIT_SELF;
	    p += SLEN(SMART);
	} else if (!strncmp(p, REGULAR, SLEN(REGULAR))) {
	    init = INIT_REGULAR;
	    p += SLEN(REGULAR);
	} else if (!strncmp(p, RANDOM, SLEN(RANDOM))) {
	    init = INIT_RANDOM;
	    p += SLEN(RANDOM);
	}
	else init = dflt;
    }
    else if (isdigit(*(unsigned char *)p)) {
	init = INIT_RANDOM;
    }
    
    if (init == INIT_RANDOM) {
	long seed;
	/* Check for seed value */
	if (!isdigit(*(unsigned char *)p) || sscanf(p, "%ld", &seed) < 1) {
#if defined(MSWIN32) || defined(WIN32)
	    seed = (unsigned) time(NULL);
#else
	    seed = (unsigned) getpid() ^ (unsigned) time(NULL);
#endif
	    sprintf(smallbuf, "%ld", seed);
	    agset(G, "start", smallbuf);
	}
	*seedp = seed;
    }
    return init;
}

/* checkExp:
 * Allow various weights for the scale factor in used to calculate stress.
 * At present, only 1 or 2 are allowed, with 2 the default.
 */
#define exp_name "stresswt"

static int checkExp (graph_t * G)
{
    int exp = late_int(G, agfindgraphattr(G, exp_name), 2, 0);
    if ((exp == 0) || (exp > 2)) {
	agerr (AGWARN, "%s attribute value must be 1 or 2 - ignoring\n", exp_name);
	exp = 2;
    }
    return exp;
}

/* checkStart:
 * Analyzes start attribute, setting seed.
 * If set,
 *   If start is regular, places nodes and returns INIT_REGULAR.
 *   If start is self, returns INIT_SELF.
 *   If start is random, returns INIT_RANDOM
 *   Set RNG seed
 * else return default
 * 
 */
int checkStart(graph_t * G, int nG, int dflt)
{
    long seed;
    int init;

    seed = 1;
    init = setSeed (G, dflt, &seed); 
    if (N_pos && (init != INIT_RANDOM)) {
	agerr(AGWARN, "node positions are ignored unless start=random\n");
    }
    if (init == INIT_REGULAR) initRegular(G, nG);
    srand48(seed);
    return init;
}

#ifdef DEBUG_COLA
void dumpData(graph_t * g, vtx_data * gp, int nv, int ne)
{
    node_t *v;
    int i, j, n;

    fprintf(stderr, "#nodes %d #edges %d\n", nv, ne);
    for (v = agfstnode(g); v; v = agnxtnode(g, v)) {
	fprintf(stderr, "\"%s\" %d\n", agnameof(v), ND_id(v));
    }
    for (i = 0; i < nv; i++) {
	n = gp[i].nedges;
	fprintf(stderr, "[%d] %d\n", i, n);
	for (j = 0; j < n; j++) {
	    fprintf(stderr, "  %3d", gp[i].edges[j]);
	}
	fputs("\n", stderr);
	if (gp[i].ewgts) {
	    fputs("  ewgts", stderr);
	    for (j = 0; j < n; j++) {
		fprintf(stderr, "  %3f", gp[i].ewgts[j]);
	    }
	    fputs("\n", stderr);
	}
	if (gp[i].eweights) {
	    fputs("  eweights", stderr);
	    for (j = 0; j < n; j++) {
		fprintf(stderr, "  %3f", gp[i].eweights[j]);
	    }
	    fputs("\n", stderr);
	}
	if (gp[i].edists) {
	    fputs("  edists", stderr);
	    for (j = 0; j < n; j++) {
		fprintf(stderr, "  %3f", gp[i].edists[j]);
	    }
	    fputs("\n", stderr);
	}
	fputs("\n", stderr);

    }
}
void dumpClusterData (cluster_data* dp)
{
  int i, j, sz;

  fprintf (stderr, "nvars %d nclusters %d ntoplevel %d\n", dp->nvars, dp->nclusters, dp->ntoplevel);
  fprintf (stderr, "Clusters:\n");
  for (i = 0; i < dp->nclusters; i++) {
    sz = dp->clustersizes[i];
    fprintf (stderr, "  [%d] %d vars\n", i, sz);
    for (j = 0; j < sz; j++)
      fprintf (stderr, "  %d", dp->clusters[i][j]);
    fprintf (stderr, "\n");
  }
    

  fprintf (stderr, "Toplevel:\n");
  for (i = 0; i < dp->ntoplevel; i++)
    fprintf (stderr, "  %d\n", dp->toplevel[i]);

  fprintf (stderr, "Boxes:\n");
  for (i = 0; i < dp->nclusters; i++) {
    boxf bb = dp->bb[i];
    fprintf (stderr, "  (%f,%f) (%f,%f)\n", bb.LL.x, bb.LL.y, bb.UR.x, bb.UR.y);
  }
}
void dumpOpts (ipsep_options* opp, int nv)
{
  int i;

  fprintf (stderr, "diredges %d edge_gap %f noverlap %d gap (%f,%f)\n", opp->diredges, opp->edge_gap, opp->noverlap, opp->gap.x, opp->gap.y);
  for (i = 0; i < nv; i++)
    fprintf (stderr, "  (%f,%f)\n", opp->nsize[i].x, opp->nsize[i].y);
  if (opp->clusters)
    dumpClusterData (opp->clusters);
}
#endif

/* majorization:
 * Solve stress using majorization.
 * Old neato attributes to incorporate:
 *  weight
 * mode will be MODE_MAJOR, MODE_HIER or MODE_IPSEP
 */
static void
majorization(graph_t *mg, graph_t * g, int nv, int mode, int model, int dim, int steps, adjust_data* am)
{
    double **coords;
    int ne;
    int i, rv = 0;
    node_t *v;
    vtx_data *gp;
    node_t** nodes;
#ifdef DIGCOLA
#ifdef IPSEPCOLA
    expand_t margin;
#endif
#endif
    int init = checkStart(g, nv, (mode == MODE_HIER ? INIT_SELF : INIT_RANDOM));
    int opts = checkExp (g);
	
    if (init == INIT_SELF)
	opts |= opt_smart_init;

    coords = N_GNEW(dim, double *);
    coords[0] = N_GNEW(nv * dim, double);
    for (i = 1; i < Ndim; i++) {
	coords[i] = coords[0] + i * nv;
    }
    if (Verbose) {
	fprintf(stderr, "model %d smart_init %d stresswt %d iterations %d tol %f\n",
		model, (init == INIT_SELF), opts & opt_exp_flag, MaxIter, Epsilon);
	fprintf(stderr, "convert graph: ");
	start_timer();
#ifdef WITH_CGRAPH
        fprintf(stderr, "majorization\n");
//	fprintf(stderr, "%i\n", count_nodes(g));

#endif /* WITH_CGRAPH */
    }
    gp = makeGraphData(g, nv, &ne, mode, model, &nodes);

    if (Verbose) {
	fprintf(stderr, "%d nodes %.2f sec\n", nv, elapsed_sec());
    }

#ifdef DIGCOLA
    if (mode != MODE_MAJOR) {
        double lgap = late_double(g, agfindgraphattr(g, "levelsgap"), 0.0, -MAXDOUBLE);
        if (mode == MODE_HIER) {        
            rv = stress_majorization_with_hierarchy(gp, nv, ne, coords, nodes, Ndim,
                       opts, model, MaxIter, lgap);
        } 
#ifdef IPSEPCOLA
	else {
            char* str;
            ipsep_options opt;
            pointf* nsize;
	    cluster_data *cs = (cluster_data*)cluster_map(mg,g);
	    nsize = N_GNEW(nv, pointf);
            opt.edge_gap = lgap;
#ifdef MOSEK
            opt.mosek = 0;
#endif /* MOSEK */
            opt.nsize = nsize;
            opt.clusters = cs;
            str = agget(g, "diredgeconstraints");
            if (mapbool(str)) {
                opt.diredges = 1;
                if(Verbose)
                    fprintf(stderr,"Generating Edge Constraints...\n");
	    } else if (str && !strncasecmp(str,"hier",4)) {
                opt.diredges = 2;
                if(Verbose)
                    fprintf(stderr,"Generating DiG-CoLa Edge Constraints...\n");
            }
	    else opt.diredges = 0;
	    if (am->mode == AM_IPSEP) {
                opt.noverlap = 1;
                if(Verbose)
                    fprintf(stderr,"Generating Non-overlap Constraints...\n");
            } else if (am->mode == AM_VPSC) {
                opt.noverlap = 2;
                if(Verbose)
                    fprintf(stderr,"Removing overlaps as postprocess...\n");
            }  
            else opt.noverlap = 0;
#ifdef MOSEK
            str = agget(g, "mosek");
            if(str && !strncmp(str,"true",4)) {
                opt.mosek = 1;
                if(Verbose)
                    fprintf(stderr,"Using Mosek for constraint optimization...\n");
            }
#endif /* MOSEK */
	    margin = sepFactor (g);
 	    /* Multiply by 2 since opt.gap is the gap size, not the margin */
	    if (margin.doAdd) {
		opt.gap.x = 2.0*PS2INCH(margin.x);
		opt.gap.y = 2.0*PS2INCH(margin.y);
	    }
            else opt.gap.x = opt.gap.y = 2.0*PS2INCH(DFLT_MARGIN);
	    if(Verbose)
		fprintf(stderr,"gap=%f,%f\n",opt.gap.x,opt.gap.y);
            for (i=0, v = agfstnode(g); v; v = agnxtnode(g, v),i++) {
                nsize[i].x = ND_width(v);
                nsize[i].y = ND_height(v);
            }

#ifdef DEBUG_COLA
	    fprintf (stderr, "nv %d ne %d Ndim %d model %d MaxIter %d\n", nv, ne, Ndim, model, MaxIter);
	    fprintf (stderr, "Nodes:\n");
	    for (i = 0; i < nv; i++) {
		fprintf (stderr, "  %s (%f,%f)\n", nodes[i]->name, coords[0][i],  coords[1][i]);
	    }
	    fprintf (stderr, "\n");
	    dumpData(g, gp, nv, ne);
	    fprintf (stderr, "\n");
	    dumpOpts (&opt, nv);
#endif
            rv = stress_majorization_cola(gp, nv, ne, coords, nodes, Ndim, model, MaxIter, &opt);
	    freeClusterData(cs);
	    free (nsize);
        }
#endif
    }
    else
#endif
	rv = stress_majorization_kD_mkernel(gp, nv, ne, coords, nodes, Ndim, opts, model, MaxIter);

    if (rv < 0) {
	agerr(AGPREV, "layout aborted\n");
    }
    else for (v = agfstnode(g); v; v = agnxtnode(g, v)) { /* store positions back in nodes */
	int idx = ND_id(v);
	int i;
	for (i = 0; i < Ndim; i++) {
	    ND_pos(v)[i] = coords[i][idx];
	}
    }
    freeGraphData(gp);
    free(coords[0]);
    free(coords);
    free(nodes);
}

static void subset_model(Agraph_t * G, int nG)
{
    int i, j, ne;
    DistType **Dij;
    vtx_data *gp;

    gp = makeGraphData(G, nG, &ne, MODE_KK, MODEL_SUBSET, NULL);
    Dij = compute_apsp_artifical_weights(gp, nG);
    for (i = 0; i < nG; i++) {
	for (j = 0; j < nG; j++) {
	    GD_dist(G)[i][j] = Dij[i][j];
	}
    }
    free(Dij[0]);
    free(Dij);
    freeGraphData(gp);
}

/* mds_model:
 * Assume the matrix already contains shortest path values.
 * Use the actual lengths provided the input for edges.
 */
static void mds_model(graph_t * g, int nG)
{
    long i, j;
    node_t *v;
    edge_t *e;

    for (v = agfstnode(g); v; v = agnxtnode(g, v)) {
	for (e = agfstout(g, v); e; e = agnxtout(g, e)) {
	    i = AGSEQ(agtail(e));
	    j = AGSEQ(aghead(e));
	    if (i == j)
		continue;
	    GD_dist(g)[i][j] = GD_dist(g)[j][i] = ED_dist(e);
	}
    }
}

/* kkNeato:
 * Solve using gradient descent a la Kamada-Kawai.
 */
static void kkNeato(Agraph_t * g, int nG, int model)
{
    if (model == MODEL_SUBSET) {
	subset_model(g, nG);
    } else if (model == MODEL_CIRCUIT) {
	if (!circuit_model(g, nG)) {
	    agerr(AGWARN,
		  "graph %s is disconnected. Hence, the circuit model\n",
		  agnameof(g));
	    agerr(AGPREV,
		  "is undefined. Reverting to the shortest path model.\n");
	    agerr(AGPREV,
		  "Alternatively, consider running neato using -Gpack=true or decomposing\n");
	    agerr(AGPREV, "the graph into connected components.\n");
	    shortest_path(g, nG);
	}
    } else if (model == MODEL_MDS) {
	shortest_path(g, nG);
	mds_model(g, nG);
    } else
	shortest_path(g, nG);
    initial_positions(g, nG);
    diffeq_model(g, nG);
    if (Verbose) {
	fprintf(stderr, "Solving model %d iterations %d tol %f\n",
		model, MaxIter, Epsilon);
	start_timer();
    }
    solve_model(g, nG);
}

/* neatoLayout:
 * Use stress optimization to layout a single component
 */
static void 
neatoLayout(Agraph_t * mg, Agraph_t * g, int layoutMode, int layoutModel,
  adjust_data* am)
{
    int nG;
    char *str;

    if ((str = agget(g, "maxiter")))
	MaxIter = atoi(str);
    else if (layoutMode == MODE_MAJOR)
	MaxIter = DFLT_ITERATIONS;
    else
	MaxIter = 100 * agnnodes(g);

    nG = scan_graph_mode(g, layoutMode);
    if ((nG < 2) || (MaxIter < 0))
	return;
    if (layoutMode)
	majorization(mg, g, nG, layoutMode, layoutModel, Ndim, MaxIter, am);
    else
	kkNeato(g, nG, layoutModel);
}

/* addZ;
 * If dimension == 3 and z attribute is declared, 
 * attach z value to nodes if not defined.
 */
static void addZ (Agraph_t* g)
{
    node_t* n;
    char    buf[BUFSIZ];

    if ((Ndim >= 3) && N_z) { 
	for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	    sprintf(buf, "%lf", POINTS_PER_INCH * (ND_pos(n)[2]));
#ifndef WITH_CGRAPH
	    agxset(n, N_z->index, buf);
#else /* WITH_CGRAPH */
	    agxset(n, N_z, buf);
#endif /* WITH_CGRAPH */
	}
    }
}

#ifdef IPSEPCOLA
static void
addCluster (graph_t* g)
{
    graph_t *subg;
#ifndef WITH_CGRAPH
    graph_t *mg;
    node_t *mm, *mn;
    edge_t *me;
    mm = g->meta_node;
    mg = agraphof(mm);
    for (me = agfstout(mg, mm); me; me = agnxtout(mg, me)) {
	mn = aghead(me);
	subg = agusergraph(mn);
#else
    for (subg = agfstsubg(agroot(g)); subg; subg = agnxtsubg(subg)) {
#endif
	if (!strncmp(agnameof(subg), "cluster", 7)) {
#ifdef WITH_CGRAPH
	    agbindrec(subg, "Agraphinfo_t", sizeof(Agraphinfo_t), TRUE);
#endif
	    add_cluster(g, subg);
	    compute_bb(subg);
	}
   }
}
#endif

/* neato_layout:
 */
void neato_layout(Agraph_t * g)
{
    int layoutMode;
    int model;
    pack_mode mode;
    pack_info pinfo;
    adjust_data am;

    if (Nop) {
	int save = PSinputscale;
	int ret;
	PSinputscale = POINTS_PER_INCH;
	neato_init_graph(g);
	addZ (g);
	ret = init_nop(g, 1);
	PSinputscale = save;
	if (ret < 0) {
	    agerr(AGPREV, "as required by the -n flag\n");
	    return;
	}
	else gv_postprocess(g, !ret);
    } else {
	neato_init_graph(g);
	layoutMode = neatoMode(g);
	graphAdjustMode (g, &am, 0);
	model = neatoModel(g);
	mode = getPackModeInfo (g, l_undef, &pinfo);
	Pack = getPack(g, -1, CL_OFFSET);
	/* pack if just packmode defined. */
	if (mode == l_undef) {
	    /* If the user has not indicated packing but we are
	     * using the new neato, turn packing on.
	     */
	    if ((Pack < 0) && layoutMode)
		Pack = CL_OFFSET;
	    pinfo.mode = l_node;
	} else if (Pack < 0)
	    Pack = CL_OFFSET;
	if (Pack >= 0) {
	    graph_t *gc;
	    graph_t **cc;
	    int n_cc;
	    int i;
	    boolean pin;

	    cc = pccomps(g, &n_cc, cc_pfx, &pin);

	    if (n_cc > 1) {
		boolean *bp;
		for (i = 0; i < n_cc; i++) {
		    gc = cc[i];
		    nodeInduce(gc);
		    neatoLayout(g, gc, layoutMode, model, &am);
		    removeOverlapWith(gc, &am);
		    setEdgeType (gc, ET_LINE);
		    spline_edges(gc);
		}
		if (pin) {
		    bp = N_NEW(n_cc, boolean);
		    bp[0] = TRUE;
		} else
		    bp = 0;
		pinfo.margin = Pack;
		pinfo.fixed = bp;
		pinfo.doSplines = 1;
		packGraphs(n_cc, cc, g, &pinfo);
		if (bp)
		    free(bp);
	    }
	    else {
		neatoLayout(g, g, layoutMode, model, &am);
		removeOverlapWith(g, &am);
		spline_edges(g);
	    }
	    compute_bb(g);
	    addZ (g);

	    /* cleanup and remove component subgraphs */
	    for (i = 0; i < n_cc; i++) {
		gc = cc[i];
		free_scan_graph(gc);
#ifdef WITH_CGRAPH
		agdelrec (gc, "Agraphinfo_t"); 
#endif
		agdelete(g, gc);
	    }
	    free (cc);
#ifdef IPSEPCOLA
	    addCluster (g);
#endif
	} else {
	    neatoLayout(g, g, layoutMode, model, &am);
	    removeOverlapWith(g, &am);
	    addZ (g);
	    spline_edges(g);
	}
	dotneato_postprocess(g);
    }
}
