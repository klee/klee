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

#include <stdio.h>
#include <stdlib.h>
#include <patchwork.h>
#include <tree_map.h>
#include "render.h"

typedef struct treenode_t treenode_t;
struct treenode_t {
    double area;
    double child_area;
    rectangle r;
    treenode_t *leftchild, *rightsib;
    union {
	Agraph_t *subg;
	Agnode_t *n;
    } u;
    int kind;
    int n_children;
};

#define DFLT_SZ 1.0
#define SCALE 1000.0      /* scale up so that 1 is a reasonable default size */

#ifdef DEBUG
void dumpTree (treenode_t* r, int ind)
{
    int i;
    treenode_t* cp;

    for (i=0; i < ind; i++) fputs("  ", stderr);
    fprintf (stderr, "%s (%f)\n", (r->kind == AGNODE?agnameof(r->u.n):agnameof(r->u.subg)), r->area);
    for (cp = r->leftchild; cp; cp = cp->rightsib)
	dumpTree (cp, ind+1);
}
#endif

/* fullArea:
 * Allow extra area for specified inset. Assume p->kind = AGRAPH
 * and p->child_area is set. At present, inset is additive; we
 * may want to allow a multiplicative inset as well.
 */
static double fullArea (treenode_t* p, attrsym_t* mp)
{
    double m = late_double (p->u.subg, mp, 0, 0);
    if (m == 0) return p->child_area;
    else {
	double wid = (2.0*m + sqrt(p->child_area));
	return wid*wid;
    }
}

static double getArea (void* obj, attrsym_t* ap)
{
    double area = late_double (obj, ap, DFLT_SZ, 0);
    if (area == 0) area = DFLT_SZ;
    area *= SCALE;
    return area;
}

/* mkTreeNode:
 */
static treenode_t* mkTreeNode (Agnode_t* n, attrsym_t* ap)
{
    treenode_t *p = NEW(treenode_t);

    p->area = getArea (n, ap);
    p->kind = AGNODE;
    p->u.n = n;

    return p;
}

#define INSERT(cp) if(!first) first=cp; if(prev) prev->rightsib=cp; prev=cp;

/* mkTree:
 * Recursively build tree from graph
 * Pre-condition: agnnodes(g) != 0
 */
static treenode_t *mkTree (Agraph_t * g, attrsym_t* gp, attrsym_t* ap, attrsym_t* mp)
{
    treenode_t *p = NEW(treenode_t);
    Agraph_t *subg;
    Agnode_t *n;
    treenode_t *cp;
    treenode_t *first = 0;
    treenode_t *prev = 0;
    int i, n_children = 0;
    double area = 0;

    p->kind = AGRAPH;
    p->u.subg = g;

    for (i = 1; i <= GD_n_cluster(g); i++) {
	subg = GD_clust(g)[i];
	cp = mkTree (subg, gp, ap, mp);
	n_children++;
	area += cp->area;
	INSERT(cp);
    }

    for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	if (SPARENT(n))
	    continue;
	cp = mkTreeNode (n, ap);
	n_children++;
	area += cp->area;
	INSERT(cp);
	SPARENT(n) = g;
    }

    p->n_children = n_children;
    if (n_children) {
	p->child_area = area;
	p->area = fullArea (p, mp);
    }
    else {
	p->area = getArea (g, gp);
    }
    p->leftchild = first;

    return p;
}

static int nodecmp (treenode_t** p0, treenode_t** p1)
{
    double diff = (*p0)->area - (*p1)->area;

    if (diff < 0) return 1;
    else if (diff > 0) return -1;
    else return 0;
}

static void layoutTree(treenode_t * tree)
{
    rectangle *recs;
    treenode_t** nodes;
    double* areas_sorted;
    int i, nc;
    treenode_t* cp;

    /* if (tree->kind == AGNODE) return; */
    if (tree->n_children == 0) return;

    nc = tree->n_children;
    nodes = N_NEW(nc, treenode_t*);
    cp = tree->leftchild;
    for (i = 0; i < nc; i++) {
	nodes[i] = cp;
	cp = cp->rightsib;
    }

    qsort (nodes, nc, sizeof(treenode_t*), (qsort_cmpf)nodecmp);
    areas_sorted = N_NEW(nc,double);
    for (i = 0; i < nc; i++) {
	areas_sorted[i] = nodes[i]->area;
    }
    if (tree->area == tree->child_area)
	recs = tree_map(nc, areas_sorted, tree->r);
    else {
	rectangle crec;
	double disc, delta, m, h = tree->r.size[1], w = tree->r.size[0];
	crec.x[0] = tree->r.x[0];
	crec.x[1] = tree->r.x[1];
	delta = h - w;
	disc = sqrt(delta*delta + 4.0*tree->child_area);
	m = (h + w - disc)/2.0;
	crec.size[0] = w - m;
	crec.size[1] = h - m;
	recs = tree_map(nc, areas_sorted, crec);
    }
    if (Verbose)
	fprintf (stderr, "rec %f %f %f %f\n", tree->r.x[0], tree->r.x[1], tree->r.size[0], tree->r.size[1]);
    for (i = 0; i < nc; i++) {
	nodes[i]->r = recs[i];
	if (Verbose)
	    fprintf (stderr, "%f - %f %f %f %f = %f (%f %f %f %f)\n", areas_sorted[i],
        	recs[i].x[0]-recs[i].size[0]*0.5, recs[i].x[1]-recs[i].size[1]*0.5,
        	recs[i].x[0]+recs[i].size[0]*0.5, recs[i].x[1]+recs[i].size[1]*0.5, recs[i].size[0]*recs[i].size[1],
        	recs[i].x[0], recs[i].x[1],  recs[i].size[0], recs[i].size[1]);

    }
    free (nodes);
    free (areas_sorted);
    free (recs);

    cp = tree->leftchild;
    for (i = 0; i < nc; i++) {
	if (cp->kind == AGRAPH)
	    layoutTree (cp);
	cp = cp->rightsib;
    }
}

static void finishNode(node_t * n)
{
    char buf [40];
    if (N_fontsize) {
#ifndef WITH_CGRAPH
	char* str = agxget(n, N_fontsize->index);
#else
	char* str = agxget(n, N_fontsize);
#endif
	if (*str == '\0') {
	    sprintf (buf, "%.03f", ND_ht(n)*0.7); 
#ifndef WITH_CGRAPH
	    agxset(n, N_fontsize->index, buf);
#else /* WITH_CGRAPH */
	    agxset(n, N_fontsize, buf);
#endif /* WITH_CGRAPH */
	}
    }
    common_init_node (n);
}

static void walkTree(treenode_t * tree)
{
    treenode_t *p;
    Agnode_t *n;
    pointf center;
    rectangle rr;
    boxf r;
    double x0,  y0, wd, ht;

    if (tree->kind == AGRAPH) {
	for (p = tree->leftchild; p; p = p->rightsib)
	    walkTree (p);
	x0 = tree->r.x[0];
	y0 = tree->r.x[1];
	wd = tree->r.size[0];
	ht = tree->r.size[1];
	r.LL.x = x0 - wd/2.0;
	r.LL.y = y0 - ht/2.0;
	r.UR.x = r.LL.x + wd;
	r.UR.y = r.LL.y + ht;
	GD_bb(tree->u.subg) = r;
    }
    else {
	rr = tree->r;
	center.x = rr.x[0];
	center.y = rr.x[1];

	n = tree->u.n;
	ND_coord(n) = center;
	ND_width(n) = PS2INCH(rr.size[0]);
	ND_height(n) = PS2INCH(rr.size[1]);
	gv_nodesize(n, GD_flip(agraphof(n)));
	finishNode(n);
	if (Verbose)
	    fprintf(stderr,"%s coord %.5g %.5g ht %f width %f\n",
		agnameof(n), ND_coord(n).x, ND_coord(n).y, ND_ht(n), ND_xsize(n));
    }
}

/* freeTree:
 */
static void freeTree (treenode_t* tp)
{
    treenode_t* cp = tp->leftchild;
    int i, nc = tp->n_children;

    for (i = 0; i < nc; i++) {
	freeTree (cp);
	cp = cp->rightsib;
    }
    free (tp);
}

/* patchworkLayout:
 */
void patchworkLayout(Agraph_t * g)
{
    treenode_t* root;
    attrsym_t * ap = agfindnodeattr(g, "area");
    attrsym_t * gp = agfindgraphattr(g, "area");
    attrsym_t * mp = agfindgraphattr(g, "inset");
    double total;

    root = mkTree (g,gp,ap,mp);
    total = root->area;
    root->r = rectangle_new(0, 0, sqrt(total + 0.1), sqrt(total + 0.1));
    layoutTree(root);
    walkTree(root);
    freeTree (root);
}
