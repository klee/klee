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


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include	"neato.h"
#include	"stress.h"
#include	<time.h>
#ifndef WIN32
#include	<unistd.h>
#endif

static double Epsilon2;


double fpow32(double x)
{
    x = sqrt(x);
    return x * x * x;
}

double distvec(double *p0, double *p1, double *vec)
{
    int k;
    double dist = 0.0;

    for (k = 0; k < Ndim; k++) {
	vec[k] = p0[k] - p1[k];
	dist += (vec[k] * vec[k]);
    }
    dist = sqrt(dist);
    return dist;
}

double **new_array(int m, int n, double ival)
{
    double **rv;
    double *mem;
    int i, j;

    rv = N_NEW(m, double *);
    mem = N_NEW(m * n, double);
    for (i = 0; i < m; i++) {
	rv[i] = mem;
	mem = mem + n;
	for (j = 0; j < n; j++)
	    rv[i][j] = ival;
    }
    return rv;
}

void free_array(double **rv)
{
    if (rv) {
	free(rv[0]);
	free(rv);
    }
}


static double ***new_3array(int m, int n, int p, double ival)
{
    double ***rv;
    int i, j, k;

    rv = N_NEW(m + 1, double **);
    for (i = 0; i < m; i++) {
	rv[i] = N_NEW(n + 1, double *);
	for (j = 0; j < n; j++) {
	    rv[i][j] = N_NEW(p, double);
	    for (k = 0; k < p; k++)
		rv[i][j][k] = ival;
	}
	rv[i][j] = NULL;	/* NULL terminate so we can clean up */
    }
    rv[i] = NULL;
    return rv;
}

static void free_3array(double ***rv)
{
    int i, j;

    if (rv) {
	for (i = 0; rv[i]; i++) {
	    for (j = 0; rv[i][j]; j++)
		free(rv[i][j]);
	    free(rv[i]);
	}
	free(rv);
    }
}


/* lenattr:
 * Return 1 if attribute not defined
 * Return 2 if attribute string bad
 */
#ifdef WITH_CGRAPH
static int lenattr(edge_t* e, Agsym_t* index, double* val)
#else
static int lenattr(edge_t* e, int index, double* val)
#endif
{
    char* s;

#ifdef WITH_CGRAPH
    if (index == NULL)
#else
    if (index < 0)
#endif
	return 1;

    s = agxget(e, index);
    if (*s == '\0') return 1;

    if ((sscanf(s, "%lf", val) < 1) || (*val < 0) || ((*val == 0) && !Nop)) {
	agerr(AGWARN, "bad edge len \"%s\"", s);
	return 2;
    }
    else
	return 0;
}


/* degreeKind;
 * Returns degree of n ignoring loops and multiedges.
 * Returns 0, 1 or many (2)
 * For case of 1, returns other endpoint of edge.
 */
static int degreeKind(graph_t * g, node_t * n, node_t ** op)
{
    edge_t *ep;
    int deg = 0;
    node_t *other = NULL;

    for (ep = agfstedge(g, n); ep; ep = agnxtedge(g, ep, n)) {
	if (aghead(ep) == agtail(ep))
	    continue;		/* ignore loops */
	if (deg == 1) {
	    if (((agtail(ep) == n) && (aghead(ep) == other)) ||	/* ignore multiedge */
		((agtail(ep) == other) && (aghead(ep) == n)))
		continue;
	    return 2;
	} else {		/* deg == 0 */
	    if (agtail(ep) == n)
		other = aghead(ep);
	    else
		other = agtail(ep);
	    *op = other;
	    deg++;
	}
    }
    return deg;
}


/* prune:
 * np is at end of a chain. If its degree is 0, remove it.
 * If its degree is 1, remove it and recurse.
 * If its degree > 1, stop.
 * The node next is the next node to be visited during iteration.
 * If it is equal to a node being deleted, set it to next one.
 * Delete from root graph, since G may be a connected component subgraph.
 * Return next.
 */
static node_t *prune(graph_t * G, node_t * np, node_t * next)
{
    node_t *other;
    int deg;

    while (np) {
	deg = degreeKind(G, np, &other);
	if (deg == 0) {
	    if (next == np)
		next = agnxtnode(G, np);
	    agdelete(G->root, np);
	    np = 0;
	} else if (deg == 1) {
	    if (next == np)
		next = agnxtnode(G, np);
	    agdelete(G->root, np);
	    np = other;
	} else
	    np = 0;

    }
    return next;
}

#ifdef WITH_CGRAPH
static double setEdgeLen(graph_t * G, node_t * np, Agsym_t* lenx, double dfltlen)
#else
static double setEdgeLen(graph_t * G, node_t * np, int lenx, double dfltlen)
#endif
{
    edge_t *ep;
    double total_len = 0.0;
    double len;
    int err;

    for (ep = agfstout(G, np); ep; ep = agnxtout(G, ep)) {
	if ((err = lenattr(ep, lenx, &len))) {
	    if (err == 2) agerr(AGPREV, " in %s - setting to %.02f\n", agnameof(G), dfltlen);
	    len = dfltlen;
	}
	ED_dist(ep) = len;
	total_len += len;
    }
    return total_len;
}

/* scan_graph_mode:
 * Prepare the graph and data structures depending on the layout mode.
 * If Reduce is true, eliminate singletons and trees. Since G may be a
 * subgraph, we remove the nodes from the root graph.
 * Return the number of nodes in the reduced graph.
 */
int scan_graph_mode(graph_t * G, int mode)
{
    int i, nV, nE, deg;
    char *str;
    node_t *np, *xp, *other;
    double total_len = 0.0;
    double dfltlen = 1.0;
#ifdef WITH_CGRAPH
    Agsym_t* lenx;
#else
    int lenx;
#endif /* WITH_CGRAPH */

    if (Verbose)
	fprintf(stderr, "Scanning graph %s, %d nodes\n", agnameof(G),
		agnnodes(G));


    /* Eliminate singletons and trees */
    if (Reduce) {
	for (np = agfstnode(G); np; np = xp) {
	    xp = agnxtnode(G, np);
	    deg = degreeKind(G, np, &other);
	    if (deg == 0) {	/* singleton node */
		agdelete(G->root, np);
	    } else if (deg == 1) {
		agdelete(G->root, np);
		xp = prune(G, other, xp);
	    }
	}
    }

    nV = agnnodes(G);
    nE = agnedges(G);

#ifdef WITH_CGRAPH
    lenx = agattr(G, AGEDGE, "len", 0);
#else
    lenx = agindex(G->root->proto->e, "len");
#endif
    if (mode == MODE_KK) {
	Epsilon = .0001 * nV;
	getdouble(G, "epsilon", &Epsilon);
	if ((str = agget(G->root, "Damping")))
	    Damping = atof(str);
	else
	    Damping = .99;
	GD_neato_nlist(G) = N_NEW(nV + 1, node_t *);
	for (i = 0, np = agfstnode(G); np; np = agnxtnode(G, np)) {
	    GD_neato_nlist(G)[i] = np;
	    ND_id(np) = i++;
	    ND_heapindex(np) = -1;
	    total_len += setEdgeLen(G, np, lenx, dfltlen);
	}
    } else {
	Epsilon = DFLT_TOLERANCE;
	getdouble(G, "epsilon", &Epsilon);
	for (i = 0, np = agfstnode(G); np; np = agnxtnode(G, np)) {
	    ND_id(np) = i++;
	    total_len += setEdgeLen(G, np, lenx, dfltlen);
	}
    }

    str = agget(G, "defaultdist");
    if (str && str[0])
	Initial_dist = MAX(Epsilon, atof(str));
    else
	Initial_dist = total_len / (nE > 0 ? nE : 1) * sqrt(nV) + 1;

    if (!Nop && (mode == MODE_KK)) {
	GD_dist(G) = new_array(nV, nV, Initial_dist);
	GD_spring(G) = new_array(nV, nV, 1.0);
	GD_sum_t(G) = new_array(nV, Ndim, 1.0);
	GD_t(G) = new_3array(nV, nV, Ndim, 0.0);
    }

    return nV;
}

int scan_graph(graph_t * g)
{
    return scan_graph_mode(g, MODE_KK);
}

void free_scan_graph(graph_t * g)
{
    free(GD_neato_nlist(g));
    if (!Nop) {
	free_array(GD_dist(g));
	free_array(GD_spring(g));
	free_array(GD_sum_t(g));
	free_3array(GD_t(g));
	GD_t(g) = NULL;
    }
}

void jitter_d(node_t * np, int nG, int n)
{
    int k;
    for (k = n; k < Ndim; k++)
	ND_pos(np)[k] = nG * drand48();
}

void jitter3d(node_t * np, int nG)
{
    jitter_d(np, nG, 2);
}

void randompos(node_t * np, int nG)
{
    ND_pos(np)[0] = nG * drand48();
    ND_pos(np)[1] = nG * drand48();
    if (Ndim > 2)
	jitter3d(np, nG);
}

void initial_positions(graph_t * G, int nG)
{
    int init, i;
    node_t *np;
    static int once = 0;

    if (Verbose)
	fprintf(stderr, "Setting initial positions\n");

    init = checkStart(G, nG, INIT_RANDOM);
    if (init == INIT_REGULAR)
	return;
    if ((init == INIT_SELF) && (once == 0)) {
	agerr(AGWARN, "start=%s not supported with mode=self - ignored\n");
	once = 1;
    }

    for (i = 0; (np = GD_neato_nlist(G)[i]); i++) {
	if (hasPos(np))
	    continue;
	randompos(np, 1);
    }
}

void diffeq_model(graph_t * G, int nG)
{
    int i, j, k;
    double dist, **D, **K, del[MAXDIM], f;
    node_t *vi, *vj;
    edge_t *e;

    if (Verbose) {
	fprintf(stderr, "Setting up spring model: ");
	start_timer();
    }
    /* init springs */
    K = GD_spring(G);
    D = GD_dist(G);
    for (i = 0; i < nG; i++) {
	for (j = 0; j < i; j++) {
	    f = Spring_coeff / (D[i][j] * D[i][j]);
	    if ((e = agfindedge(G, GD_neato_nlist(G)[i], GD_neato_nlist(G)[j])))
		f = f * ED_factor(e);
	    K[i][j] = K[j][i] = f;
	}
    }

    /* init differential equation solver */
    for (i = 0; i < nG; i++)
	for (k = 0; k < Ndim; k++)
	    GD_sum_t(G)[i][k] = 0.0;

    for (i = 0; (vi = GD_neato_nlist(G)[i]); i++) {
	for (j = 0; j < nG; j++) {
	    if (i == j)
		continue;
	    vj = GD_neato_nlist(G)[j];
	    dist = distvec(ND_pos(vi), ND_pos(vj), del);
	    for (k = 0; k < Ndim; k++) {
		GD_t(G)[i][j][k] =
		    GD_spring(G)[i][j] * (del[k] -
					  GD_dist(G)[i][j] * del[k] /
					  dist);
		GD_sum_t(G)[i][k] += GD_t(G)[i][j][k];
	    }
	}
    }
    if (Verbose) {
	fprintf(stderr, "%.2f sec\n", elapsed_sec());
    }
}

/* total_e:
 * Return 2*energy of system.
 */
static double total_e(graph_t * G, int nG)
{
    int i, j, d;
    double e = 0.0;		/* 2*energy */
    double t0;			/* distance squared */
    double t1;
    node_t *ip, *jp;

    for (i = 0; i < nG - 1; i++) {
	ip = GD_neato_nlist(G)[i];
	for (j = i + 1; j < nG; j++) {
	    jp = GD_neato_nlist(G)[j];
	    for (t0 = 0.0, d = 0; d < Ndim; d++) {
		t1 = (ND_pos(ip)[d] - ND_pos(jp)[d]);
		t0 += t1 * t1;
	    }
	    e = e + GD_spring(G)[i][j] *
		(t0 + GD_dist(G)[i][j] * GD_dist(G)[i][j]
		 - 2.0 * GD_dist(G)[i][j] * sqrt(t0));
	}
    }
    return e;
}

void solve_model(graph_t * G, int nG)
{
    node_t *np;

    Epsilon2 = Epsilon * Epsilon;

    while ((np = choose_node(G, nG))) {
	move_node(G, nG, np);
    }
    if (Verbose) {
	fprintf(stderr, "\nfinal e = %f", total_e(G, nG));
	fprintf(stderr, " %d%s iterations %.2f sec\n",
		GD_move(G), (GD_move(G) == MaxIter ? "!" : ""),
		elapsed_sec());
    }
    if (GD_move(G) == MaxIter)
	agerr(AGWARN, "Max. iterations (%d) reached on graph %s\n",
	      MaxIter, agnameof(G));
}

void update_arrays(graph_t * G, int nG, int i)
{
    int j, k;
    double del[MAXDIM], dist, old;
    node_t *vi, *vj;

    vi = GD_neato_nlist(G)[i];
    for (k = 0; k < Ndim; k++)
	GD_sum_t(G)[i][k] = 0.0;
    for (j = 0; j < nG; j++) {
	if (i == j)
	    continue;
	vj = GD_neato_nlist(G)[j];
	dist = distvec(ND_pos(vi), ND_pos(vj), del);
	for (k = 0; k < Ndim; k++) {
	    old = GD_t(G)[i][j][k];
	    GD_t(G)[i][j][k] =
		GD_spring(G)[i][j] * (del[k] -
				      GD_dist(G)[i][j] * del[k] / dist);
	    GD_sum_t(G)[i][k] += GD_t(G)[i][j][k];
	    old = GD_t(G)[j][i][k];
	    GD_t(G)[j][i][k] = -GD_t(G)[i][j][k];
	    GD_sum_t(G)[j][k] += (GD_t(G)[j][i][k] - old);
	}
    }
}

#define Msub(i,j)  M[(i)*Ndim+(j)]
void D2E(graph_t * G, int nG, int n, double *M)
{
    int i, l, k;
    node_t *vi, *vn;
    double scale, sq, t[MAXDIM];
    double **K = GD_spring(G);
    double **D = GD_dist(G);

    vn = GD_neato_nlist(G)[n];
    for (l = 0; l < Ndim; l++)
	for (k = 0; k < Ndim; k++)
	    Msub(l, k) = 0.0;
    for (i = 0; i < nG; i++) {
	if (n == i)
	    continue;
	vi = GD_neato_nlist(G)[i];
	sq = 0.0;
	for (k = 0; k < Ndim; k++) {
	    t[k] = ND_pos(vn)[k] - ND_pos(vi)[k];
	    sq += (t[k] * t[k]);
	}
	scale = 1 / fpow32(sq);
	for (k = 0; k < Ndim; k++) {
	    for (l = 0; l < k; l++)
		Msub(l, k) += K[n][i] * D[n][i] * t[k] * t[l] * scale;
	    Msub(k, k) +=
		K[n][i] * (1.0 - D[n][i] * (sq - (t[k] * t[k])) * scale);
	}
    }
    for (k = 1; k < Ndim; k++)
	for (l = 0; l < k; l++)
	    Msub(k, l) = Msub(l, k);
}

void final_energy(graph_t * G, int nG)
{
    fprintf(stderr, "iterations = %d final e = %f\n", GD_move(G),
	    total_e(G, nG));
}

node_t *choose_node(graph_t * G, int nG)
{
    int i, k;
    double m, max;
    node_t *choice, *np;
    static int cnt = 0;
#if 0
    double e;
    static double save_e = MAXDOUBLE;
#endif

    cnt++;
    if (GD_move(G) >= MaxIter)
	return NULL;
#if 0
    if ((cnt % 100) == 0) {
	e = total_e(G, nG);
	if (e - save_e > 0)
	    return NULL;
	save_e = e;
    }
#endif
    max = 0.0;
    choice = NULL;
    for (i = 0; i < nG; i++) {
	np = GD_neato_nlist(G)[i];
	if (ND_pinned(np) > P_SET)
	    continue;
	for (m = 0.0, k = 0; k < Ndim; k++)
	    m += (GD_sum_t(G)[i][k] * GD_sum_t(G)[i][k]);
	/* could set the color=energy of the node here */
	if (m > max) {
	    choice = np;
	    max = m;
	}
    }
    if (max < Epsilon2)
	choice = NULL;
    else {
	if (Verbose && (cnt % 100 == 0)) {
	    fprintf(stderr, "%.3f ", sqrt(max));
	    if (cnt % 1000 == 0)
		fprintf(stderr, "\n");
	}
#if 0
	e = total_e(G, nG);
	if (fabs((e - save_e) / save_e) < 1e-5) {
	    choice = NULL;
	}
#endif
    }
    return choice;
}

void move_node(graph_t * G, int nG, node_t * n)
{
    int i, m;
    static double *a, b[MAXDIM], c[MAXDIM];

    m = ND_id(n);
    a = ALLOC(Ndim * Ndim, a, double);
    D2E(G, nG, m, a);
    for (i = 0; i < Ndim; i++)
	c[i] = -GD_sum_t(G)[m][i];
    solve(a, b, c, Ndim);
    for (i = 0; i < Ndim; i++) {
	b[i] = (Damping + 2 * (1 - Damping) * drand48()) * b[i];
	ND_pos(n)[i] += b[i];
    }
    GD_move(G)++;
    update_arrays(G, nG, m);
    if (test_toggle()) {
	double sum = 0;
	for (i = 0; i < Ndim; i++) {
	    sum += fabs(b[i]);
	}			/* Why not squared? */
	sum = sqrt(sum);
	fprintf(stderr, "%s %.3f\n", agnameof(n), sum);
    }
}

static node_t **Heap;
static int Heapsize;
static node_t *Src;

void heapup(node_t * v)
{
    int i, par;
    node_t *u;

    for (i = ND_heapindex(v); i > 0; i = par) {
	par = (i - 1) / 2;
	u = Heap[par];
	if (ND_dist(u) <= ND_dist(v))
	    break;
	Heap[par] = v;
	ND_heapindex(v) = par;
	Heap[i] = u;
	ND_heapindex(u) = i;
    }
}

void heapdown(node_t * v)
{
    int i, left, right, c;
    node_t *u;

    i = ND_heapindex(v);
    while ((left = 2 * i + 1) < Heapsize) {
	right = left + 1;
	if ((right < Heapsize)
	    && (ND_dist(Heap[right]) < ND_dist(Heap[left])))
	    c = right;
	else
	    c = left;
	u = Heap[c];
	if (ND_dist(v) <= ND_dist(u))
	    break;
	Heap[c] = v;
	ND_heapindex(v) = c;
	Heap[i] = u;
	ND_heapindex(u) = i;
	i = c;
    }
}

void neato_enqueue(node_t * v)
{
    int i;

    assert(ND_heapindex(v) < 0);
    i = Heapsize++;
    ND_heapindex(v) = i;
    Heap[i] = v;
    if (i > 0)
	heapup(v);
}

node_t *neato_dequeue(void)
{
    int i;
    node_t *rv, *v;

    if (Heapsize == 0)
	return NULL;
    rv = Heap[0];
    i = --Heapsize;
    v = Heap[i];
    Heap[0] = v;
    ND_heapindex(v) = 0;
    if (i > 1)
	heapdown(v);
    ND_heapindex(rv) = -1;
    return rv;
}

void shortest_path(graph_t * G, int nG)
{
    node_t *v;

    Heap = N_NEW(nG + 1, node_t *);
    if (Verbose) {
	fprintf(stderr, "Calculating shortest paths: ");
	start_timer();
    }
    for (v = agfstnode(G); v; v = agnxtnode(G, v))
	s1(G, v);
    if (Verbose) {
	fprintf(stderr, "%.2f sec\n", elapsed_sec());
    }
    free(Heap);
}

void s1(graph_t * G, node_t * node)
{
    node_t *v, *u;
    edge_t *e;
    int t;
    double f;

    for (t = 0; (v = GD_neato_nlist(G)[t]); t++)
	ND_dist(v) = Initial_dist;
    Src = node;
    ND_dist(Src) = 0;
    ND_hops(Src) = 0;
    neato_enqueue(Src);

    while ((v = neato_dequeue())) {
	if (v != Src)
	    make_spring(G, Src, v, ND_dist(v));
	for (e = agfstedge(G, v); e; e = agnxtedge(G, e, v)) {
	    if ((u = agtail(e)) == v)
		u = aghead(e);
	    f = ND_dist(v) + ED_dist(e);
	    if (ND_dist(u) > f) {
		ND_dist(u) = f;
		if (ND_heapindex(u) >= 0)
		    heapup(u);
		else {
		    ND_hops(u) = ND_hops(v) + 1;
		    neato_enqueue(u);
		}
	    }
	}
    }
}

void make_spring(graph_t * G, node_t * u, node_t * v, double f)
{
    int i, j;

    i = ND_id(u);
    j = ND_id(v);
    GD_dist(G)[i][j] = GD_dist(G)[j][i] = f;
}

int allow_edits(int nsec)
{
#ifdef INTERACTIVE
    static int onetime = TRUE;
    static FILE *fp;
    static fd_set fd;
    static struct timeval tv;

    char buf[256], name[256];
    double x, y;
    node_t *np;

    if (onetime) {
	fp = fopen("/dev/tty", "r");
	if (fp == NULL)
	    exit(1);
	setbuf(fp, NULL);
	tv.tv_usec = 0;
	onetime = FALSE;
    }
    tv.tv_sec = nsec;
    FD_ZERO(&fd);
    FD_SET(fileno(fp), &fd);
    if (select(32, &fd, (fd_set *) 0, (fd_set *) 0, &tv) > 0) {
	fgets(buf, sizeof(buf), fp);
	switch (buf[0]) {
	case 'm':		/* move node */
	    if (sscanf(buf + 1, "%s %lf%lf", name, &x, &y) == 3) {
		np = getnode(G, name);
		if (np) {
		    NP_pos(np)[0] = x;
		    NP_pos(np)[1] = y;
		    diffeq_model();
		}
	    }
	    break;
	case 'q':
	    return FALSE;
	default:
	    agerr(AGERR, "unknown command '%s', ignored\n", buf);
	}
	return TRUE;
    }
#endif
    return FALSE;
}
