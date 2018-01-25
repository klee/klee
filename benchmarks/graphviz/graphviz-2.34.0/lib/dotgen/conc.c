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


/*
 *	build edge_t concentrators for parallel edges with a common endpoint
 */

#include	"dot.h"
#include	<setjmp.h>

#define		UP		0
#define		DOWN	1

static jmp_buf jbuf;

static boolean samedir(edge_t * e, edge_t * f)
{
    edge_t *e0, *f0;

    for (e0 = e; ED_edge_type(e0) != NORMAL; e0 = ED_to_orig(e0));
    for (f0 = f; ED_edge_type(f0) != NORMAL; f0 = ED_to_orig(f0));
    if (ED_conc_opp_flag(e0))
	return FALSE;
    if (ED_conc_opp_flag(f0))
	return FALSE;
    return ((ND_rank(agtail(f0)) - ND_rank(aghead(f0)))
	    * (ND_rank(agtail(e0)) - ND_rank(aghead(e0))) > 0);
}

static boolean downcandidate(node_t * v)
{
    return ((ND_node_type(v) == VIRTUAL) && (ND_in(v).size == 1)
	    && (ND_out(v).size == 1) && (ND_label(v) == NULL));
}

static boolean bothdowncandidates(node_t * u, node_t * v)
{
    edge_t *e, *f;
    e = ND_in(u).list[0];
    f = ND_in(v).list[0];
    if (downcandidate(v) && (agtail(e) == agtail(f))) {
	return samedir(e, f)
	    && (portcmp(ED_tail_port(e), ED_tail_port(f)) == 0);
    }
    return FALSE;
}

static boolean upcandidate(node_t * v)
{
    return ((ND_node_type(v) == VIRTUAL) && (ND_out(v).size == 1)
	    && (ND_in(v).size == 1) && (ND_label(v) == NULL));
}

static boolean bothupcandidates(node_t * u, node_t * v)
{
    edge_t *e, *f;
    e = ND_out(u).list[0];
    f = ND_out(v).list[0];
    if (upcandidate(v) && (aghead(e) == aghead(f))) {
	return samedir(e, f)
	    && (portcmp(ED_head_port(e), ED_head_port(f)) == 0);
    }
    return FALSE;
}

static void mergevirtual(graph_t * g, int r, int lpos, int rpos, int dir)
{
    int i, k;
    node_t *left, *right;
    edge_t *e, *f, *e0;

    left = GD_rank(g)[r].v[lpos];
    /* merge all right nodes into the leftmost one */
    for (i = lpos + 1; i <= rpos; i++) {
	right = GD_rank(g)[r].v[i];
	if (dir == DOWN) {
	    while ((e = ND_out(right).list[0])) {
		for (k = 0; (f = ND_out(left).list[k]); k++)
		    if (aghead(f) == aghead(e))
			break;
		if (f == NULL)
		    f = virtual_edge(left, aghead(e), e);
		while ((e0 = ND_in(right).list[0])) {
		    merge_oneway(e0, f);
		    /*ED_weight(f) += ED_weight(e0); */
		    delete_fast_edge(e0);
		}
		delete_fast_edge(e);
	    }
	} else {
	    while ((e = ND_in(right).list[0])) {
		for (k = 0; (f = ND_in(left).list[k]); k++)
		    if (agtail(f) == agtail(e))
			break;
		if (f == NULL)
		    f = virtual_edge(agtail(e), left, e);
		while ((e0 = ND_out(right).list[0])) {
		    merge_oneway(e0, f);
		    delete_fast_edge(e0);
		}
		delete_fast_edge(e);
	    }
	}
	assert(ND_in(right).size + ND_out(right).size == 0);
	delete_fast_node(g, right);
    }
    k = lpos + 1;
    i = rpos + 1;
    while (i < GD_rank(g)[r].n) {
	node_t *n;
	n = GD_rank(g)[r].v[k] = GD_rank(g)[r].v[i];
	ND_order(n) = k;
	k++;
	i++;
    }
    GD_rank(g)[r].n = k;
    GD_rank(g)[r].v[k] = NULL;
}

static void infuse(graph_t * g, node_t * n)
{
    node_t *lead;

    lead = GD_rankleader(g)[ND_rank(n)];
    if ((lead == NULL) || (ND_order(lead) > ND_order(n)))
	GD_rankleader(g)[ND_rank(n)] = n;
}

static void rebuild_vlists(graph_t * g)
{
    int c, i, r, maxi;
    node_t *n, *lead;
    edge_t *e, *rep;

    for (r = GD_minrank(g); r <= GD_maxrank(g); r++)
	GD_rankleader(g)[r] = NULL;
    dot_scan_ranks(g);
    for (n = agfstnode(g); n; n = agnxtnode(g, n)) {
	infuse(g, n);
	for (e = agfstout(g, n); e; e = agnxtout(g, e)) {
	    for (rep = e; ED_to_virt(rep); rep = ED_to_virt(rep));
	    while (ND_rank(aghead(rep)) < ND_rank(aghead(e))) {
		infuse(g, aghead(rep));
		rep = ND_out(aghead(rep)).list[0];
	    }
	}
    }

    for (r = GD_minrank(g); r <= GD_maxrank(g); r++) {
	lead = GD_rankleader(g)[r];
	if (GD_rank(agroot(g))[r].v[ND_order(lead)] != lead) {
	    agerr(AGERR, "rebuiltd_vlists: rank lead %s not in order %d of rank %d\n", 
		agnameof(lead), ND_order(lead), r);
	    longjmp(jbuf, 1);
	}
	GD_rank(g)[r].v =
	    GD_rank(agroot(g))[r].v + ND_order((GD_rankleader(g)[r]));
	maxi = -1;
	for (i = 0; i < GD_rank(g)[r].n; i++) {
	    if ((n = GD_rank(g)[r].v[i]) == NULL)
		break;
	    if (ND_node_type(n) == NORMAL) {
		if (agcontains(g, n))
		    maxi = i;
		else
		    break;
	    } else {
		edge_t *e;
		for (e = ND_in(n).list[0]; e && ED_to_orig(e);
		     e = ED_to_orig(e));
		if (e && (agcontains(g, agtail(e)))
		    && agcontains(g, aghead(e)))
		    maxi = i;
	    }
	}
	if (maxi == -1)
	    agerr(AGWARN, "degenerate concentrated rank %s,%d\n", agnameof(g),
		  r);
	GD_rank(g)[r].n = maxi + 1;
    }

    for (c = 1; c <= GD_n_cluster(g); c++)
	rebuild_vlists(GD_clust(g)[c]);
}

void dot_concentrate(graph_t * g)
{
    int c, r, leftpos, rightpos;
    node_t *left, *right;

    if (GD_maxrank(g) - GD_minrank(g) <= 1)
	return;
    /* this is the downward looking pass. r is a candidate rank. */
    for (r = 1; GD_rank(g)[r + 1].n; r++) {
	for (leftpos = 0; leftpos < GD_rank(g)[r].n; leftpos++) {
	    left = GD_rank(g)[r].v[leftpos];
	    if (downcandidate(left) == FALSE)
		continue;
	    for (rightpos = leftpos + 1; rightpos < GD_rank(g)[r].n;
		 rightpos++) {
		right = GD_rank(g)[r].v[rightpos];
		if (bothdowncandidates(left, right) == FALSE)
		    break;
	    }
	    if (rightpos - leftpos > 1)
		mergevirtual(g, r, leftpos, rightpos - 1, DOWN);
	}
    }
    /* this is the corresponding upward pass */
    while (r > 0) {
	for (leftpos = 0; leftpos < GD_rank(g)[r].n; leftpos++) {
	    left = GD_rank(g)[r].v[leftpos];
	    if (upcandidate(left) == FALSE)
		continue;
	    for (rightpos = leftpos + 1; rightpos < GD_rank(g)[r].n;
		 rightpos++) {
		right = GD_rank(g)[r].v[rightpos];
		if (bothupcandidates(left, right) == FALSE)
		    break;
	    }
	    if (rightpos - leftpos > 1)
		mergevirtual(g, r, leftpos, rightpos - 1, UP);
	}
	r--;
    }
    if (setjmp(jbuf)) {
	agerr(AGPREV, "concentrate=true may not work correctly.\n");
	return;
    }
    for (c = 1; c <= GD_n_cluster(g); c++)
	rebuild_vlists(GD_clust(g)[c]);
}
