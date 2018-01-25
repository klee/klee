/* $Id$ $Revision$ */
/* vim:set shiftwidth=4 ts=8: */

/**
 *
 * Authors:
 *   Tim Dwyer <tgdwyer@gmail.com>
 *
 * Copyright (C) 2005 Authors
 *
 * This version is released under the CPL (Common Public License) with
 * the Graphviz distribution.
 * A version is also available under the LGPL as part of the Adaptagrams
 * project: http://sourceforge.net/projects/adaptagrams.  
 * If you make improvements or bug fixes to this code it would be much
 * appreciated if you could also contribute those changes back to the
 * Adaptagrams repository.
 */

/**********************************************************
 *
 * Solve a quadratic function f(X) = X' e->A X + b X
 * subject to a set of separation constraints e->cs
 *
 * Tim Dwyer, 2006
 **********************************************************/

#include "digcola.h"
#ifdef IPSEPCOLA
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <float.h>
#include <assert.h>
#include "matrix_ops.h"
#include "kkutils.h"
#include <csolve_VPSC.h>
#include "quad_prog_vpsc.h"
#include "quad_prog_solver.h"

/* #define CONMAJ_LOGGING 1 */
#define quad_prog_tol 1e-4

/*
 * Use gradient-projection to solve Variable Placement with Separation Constraints problem.
 */
int
constrained_majorization_vpsc(CMajEnvVPSC * e, float *b, float *place,
			      int max_iterations)
{
    int i, j, counter;
    float *g, *old_place, *d;
    /* for laplacian computation need number of real vars and those
     * dummy vars included in lap
     */
    int n = e->nv + e->nldv;
    boolean converged = FALSE;
#ifdef CONMAJ_LOGGING
    static int call_no = 0;
#endif				/* CONMAJ_LOGGING */

    if (max_iterations == 0)
	return 0;
    g = e->fArray1;
    old_place = e->fArray2;
    d = e->fArray3;
    /* fprintf(stderr,"Entered: constrained_majorization_vpsc, #constraints=%d\n",e->m); */
    if (e->m > 0) {
	for (i = 0; i < n; i++) {
	    setVariableDesiredPos(e->vs[i], place[i]);
	}
	/* fprintf(stderr,"  calling satisfyVPSC...\n"); */
	satisfyVPSC(e->vpsc);
	for (i = 0; i < n; i++) {
	    place[i] = getVariablePos(e->vs[i]);
	    /* fprintf(stderr,"vs[%d]=%f\n",i,place[i]); */
	}
	/* fprintf(stderr,"    done.\n"); */
    }
#ifdef CONMAJ_LOGGING
    float prev_stress = 0;
    for (i = 0; i < n; i++) {
	prev_stress += 2 * b[i] * place[i];
	for (j = 0; j < n; j++) {
	    prev_stress -= e->A[i][j] * place[j] * place[i];
	}
    }
    FILE *logfile = fopen("constrained_majorization_log", "a");

    /* fprintf(logfile,"grad proj %d: stress=%f\n",call_no,prev_stress); */
#endif

    for (counter = 0; counter < max_iterations && !converged; counter++) {
	float test = 0;
	float alpha, beta;
	float numerator = 0, denominator = 0, r;
	/* fprintf(stderr,"."); */
	converged = TRUE;
	/* find steepest descent direction */
	for (i = 0; i < n; i++) {
	    old_place[i] = place[i];
	    g[i] = 2 * b[i];
	    for (j = 0; j < n; j++) {
		g[i] -= 2 * e->A[i][j] * place[j];
	    }
	}
	for (i = 0; i < n; i++) {
	    numerator += g[i] * g[i];
	    r = 0;
	    for (j = 0; j < n; j++) {
		r += 2 * e->A[i][j] * g[j];
	    }
	    denominator -= r * g[i];
	}
	if (denominator != 0)
	    alpha = numerator / denominator;
	else
	    alpha = 1.0;
	for (i = 0; i < n; i++) {
	    place[i] -= alpha * g[i];
	}
	if (e->m > 0) {
	    /* project to constraint boundary */
	    for (i = 0; i < n; i++) {
		setVariableDesiredPos(e->vs[i], place[i]);
	    }
	    satisfyVPSC(e->vpsc);
	    for (i = 0; i < n; i++) {
		place[i] = getVariablePos(e->vs[i]);
	    }
	}
	/* set place to the intersection of old_place-g and boundary and 
	 * compute d, the vector from intersection pnt to projection pnt
	 */
	for (i = 0; i < n; i++) {
	    d[i] = place[i] - old_place[i];
	}
	/* now compute beta */
	numerator = 0, denominator = 0;
	for (i = 0; i < n; i++) {
	    numerator += g[i] * d[i];
	    r = 0;
	    for (j = 0; j < n; j++) {
		r += 2 * e->A[i][j] * d[j];
	    }
	    denominator += r * d[i];
	}
	if (denominator != 0.0)
	    beta = numerator / denominator;
	else
	    beta = 1.0;

	for (i = 0; i < n; i++) {
	    /* beta > 1.0 takes us back outside the feasible region
	     * beta < 0 clearly not useful and may happen due to numerical imp.
	     */
	    if (beta > 0 && beta < 1.0) {
		place[i] = old_place[i] + beta * d[i];
	    }
	    test += fabs(place[i] - old_place[i]);
	}
#ifdef CONMAJ_LOGGING
	float stress = 0;
	for (i = 0; i < n; i++) {
	    stress += 2 * b[i] * place[i];
	    for (j = 0; j < n; j++) {
		stress -= e->A[i][j] * place[j] * place[i];
	    }
	}
	fprintf(logfile, "%d: stress=%f, test=%f, %s\n", call_no, stress,
		test, (stress >= prev_stress) ? "No Improvement" : "");
	prev_stress = stress;
#endif
	if (test > quad_prog_tol) {
	    converged = FALSE;
	}
    }
#ifdef CONMAJ_LOGGING
    call_no++;
    fclose(logfile);
#endif
    return counter;
}

/*
 * Set up environment and global constraints (dir-edge constraints, containment constraints
 * etc).
 *
 * diredges: 0=no dir edge constraints
 *           1=one separation constraint for each edge (in acyclic subgraph)
 *           2=DiG-CoLa level constraints
 */
CMajEnvVPSC *initCMajVPSC(int n, float *packedMat, vtx_data * graph,
			  ipsep_options * opt, int diredges)
{
    int i, j;
    /* nv is the number of real nodes */
    int nConCs;
    /* fprintf(stderr,"Entered initCMajVPSC\n"); */
    CMajEnvVPSC *e = GNEW(CMajEnvVPSC);
    e->A = NULL;
    e->packedMat = packedMat;
    /* if we have clusters then we'll need two constraints for each var in
     * a cluster */
    e->nldv = 2 * opt->clusters->nclusters;
    e->nv = n - e->nldv;
    e->ndv = 0;

    e->gcs = NULL;
    e->vs = N_GNEW(n, Variable *);
    for (i = 0; i < n; i++) {
	e->vs[i] = newVariable(i, 1.0, 1.0);
    }
    e->gm = 0;
    if (diredges == 1) {
	if (Verbose)
	    fprintf(stderr, "  generate edge constraints...\n");
	for (i = 0; i < e->nv; i++) {
	    for (j = 1; j < graph[i].nedges; j++) {
		/* fprintf(stderr,"edist=%f\n",graph[i].edists[j]); */
		if (graph[i].edists[j] > 0.01) {
		    e->gm++;
		}
	    }
	}
	e->gcs = newConstraints(e->gm);
	e->gm = 0;
	for (i = 0; i < e->nv; i++) {
	    for (j = 1; j < graph[i].nedges; j++) {
		int u = i, v = graph[i].edges[j];
		if (graph[i].edists[j] > 0) {
		    e->gcs[e->gm++] =
			newConstraint(e->vs[u], e->vs[v], opt->edge_gap);
		}
	    }
	}
    } else if (diredges == 2) {
	int *ordering = NULL, *ls = NULL, cvar;
	double halfgap;
	DigColaLevel *levels;
	Variable **vs = e->vs;
	/* e->ndv is the number of dummy variables required, one for each boundary */
	if (compute_hierarchy(graph, e->nv, 1e-2, 1e-1, NULL, &ordering, &ls,
			  &e->ndv)) return NULL;
	levels = assign_digcola_levels(ordering, e->nv, ls, e->ndv);
	if (Verbose)
	    fprintf(stderr, "Found %d DiG-CoLa boundaries\n", e->ndv);
	e->gm =
	    get_num_digcola_constraints(levels, e->ndv + 1) + e->ndv - 1;
	e->gcs = newConstraints(e->gm);
	e->gm = 0;
	e->vs = N_GNEW(n + e->ndv, Variable *);
	for (i = 0; i < n; i++) {
	    e->vs[i] = vs[i];
	}
	free(vs);
	/* create dummy vars */
	for (i = 0; i < e->ndv; i++) {
	    /* dummy vars should have 0 weight */
	    cvar = n + i;
	    e->vs[cvar] = newVariable(cvar, 1.0, 0.000001);
	}
	halfgap = opt->edge_gap;
	for (i = 0; i < e->ndv; i++) {
	    cvar = n + i;
	    /* outgoing constraints for each var in level below boundary */
	    for (j = 0; j < levels[i].num_nodes; j++) {
		e->gcs[e->gm++] =
		    newConstraint(e->vs[levels[i].nodes[j]], e->vs[cvar],
				  halfgap);
	    }
	    /* incoming constraints for each var in level above boundary */
	    for (j = 0; j < levels[i + 1].num_nodes; j++) {
		e->gcs[e->gm++] =
		    newConstraint(e->vs[cvar],
				  e->vs[levels[i + 1].nodes[j]], halfgap);
	    }
	}
	/* constraints between adjacent boundary dummy vars */
	for (i = 0; i < e->ndv - 1; i++) {
	    e->gcs[e->gm++] =
		newConstraint(e->vs[n + i], e->vs[n + i + 1], 0);
	}
    }
    /* fprintf(stderr,"  generate edge constraints... done: n=%d,m=%d\n",e->n,e->gm); */
    if (opt->clusters->nclusters > 0) {
	/* fprintf(stderr,"  generate cluster containment constraints...\n"); */
	Constraint **ecs = e->gcs;
	nConCs = 2 * opt->clusters->nvars;
	e->gcs = newConstraints(e->gm + nConCs);
	for (i = 0; i < e->gm; i++) {
	    e->gcs[i] = ecs[i];
	}
	if (ecs != NULL)
	    deleteConstraints(0, ecs);
	for (i = 0; i < opt->clusters->nclusters; i++) {
	    for (j = 0; j < opt->clusters->clustersizes[i]; j++) {
		Variable *v = e->vs[opt->clusters->clusters[i][j]];
		Variable *cl = e->vs[e->nv + 2 * i];
		Variable *cr = e->vs[e->nv + 2 * i + 1];
		e->gcs[e->gm++] = newConstraint(cl, v, 0);
		e->gcs[e->gm++] = newConstraint(v, cr, 0);
	    }
	}
	/* fprintf(stderr,"  containment constraints... done: \n"); */
    }

    e->m = 0;
    e->cs = NULL;
    if (e->gm > 0) {
	e->vpsc = newIncVPSC(n + e->ndv, e->vs, e->gm, e->gcs);
	e->m = e->gm;
	e->cs = e->gcs;
    }
    if (packedMat != NULL) {
	e->A = unpackMatrix(packedMat, n);
    }
#ifdef MOSEK
    e->mosekEnv = NULL;
    if (opt->mosek) {
	e->mosekEnv =
	    mosek_init_sep(e->packedMat, n, e->ndv, e->gcs, e->gm);
    }
#endif

    e->fArray1 = N_GNEW(n, float);
    e->fArray2 = N_GNEW(n, float);
    e->fArray3 = N_GNEW(n, float);
    if (Verbose)
	fprintf(stderr,
		"  initCMajVPSC done: %d global constraints generated.\n",
		e->m);
    return e;
}

void deleteCMajEnvVPSC(CMajEnvVPSC * e)
{
    int i;
    if (e->A != NULL) {
	free(e->A[0]);
	free(e->A);
    }
    if (e->m > 0) {
	deleteVPSC(e->vpsc);
	if (e->cs != e->gcs && e->gcs != NULL)
	    deleteConstraints(0, e->gcs);
	deleteConstraints(e->m, e->cs);
	for (i = 0; i < e->nv + e->nldv + e->ndv; i++) {
	    deleteVariable(e->vs[i]);
	}
	free(e->vs);
    }
    free(e->fArray1);
    free(e->fArray2);
    free(e->fArray3);
#ifdef MOSEK
    if (e->mosekEnv) {
	mosek_delete(e->mosekEnv);
    }
#endif				/* MOSEK */
    free(e);
}

/* generate non-overlap constraints inside each cluster, including dummy
 * nodes at bounds of cluster
 * generate constraints again for top level nodes and clusters treating
 * clusters as rectangles of dim (l,r,b,t)
 * for each cluster map in-constraints to l out-constraints to r 
 *
 * For now, we'll keep the global containment constraints already
 * generated for each cluster, and simply generate non-overlap constraints
 * for all nodes and then an additional set of non-overlap constraints for
 * clusters that we'll map back to the dummy vars as above.
 */
void generateNonoverlapConstraints(CMajEnvVPSC * e,
				   float nsizeScale,
				   float **coords,
				   int k,
				   boolean transitiveClosure,
				   ipsep_options * opt)
{
    Constraint **csol, **csolptr;
    int i, j, mol = 0;
    int n = e->nv + e->nldv;
#ifdef WIN32
    boxf* bb = N_GNEW (n, boxf);
#else
    boxf bb[n];
#endif
    boolean genclusters = opt->clusters->nclusters > 0;
    if (genclusters) {
	/* n is the number of real variables, not dummy cluster vars */
	n -= 2 * opt->clusters->nclusters;
    }
    if (k == 0) {
	/* grow a bit in the x dimension, so that if overlap is resolved
	 * horizontally then it won't be considered overlapping vertically
	 */
	nsizeScale *= 1.0001;
    }
    for (i = 0; i < n; i++) {
	bb[i].LL.x =
	    coords[0][i] - nsizeScale * opt->nsize[i].x / 2.0 -
	    opt->gap.x / 2.0;
	bb[i].UR.x =
	    coords[0][i] + nsizeScale * opt->nsize[i].x / 2.0 +
	    opt->gap.x / 2.0;
	bb[i].LL.y =
	    coords[1][i] - nsizeScale * opt->nsize[i].y / 2.0 -
	    opt->gap.y / 2.0;
	bb[i].UR.y =
	    coords[1][i] + nsizeScale * opt->nsize[i].y / 2.0 +
	    opt->gap.y / 2.0;
    }
    if (genclusters) {
#ifdef WIN32
	Constraint ***cscl = N_GNEW(opt->clusters->nclusters + 1, Constraint**);
	int* cm = N_GNEW(opt->clusters->nclusters + 1, int);
#else
	Constraint **cscl[opt->clusters->nclusters + 1];
	int cm[opt->clusters->nclusters + 1];
#endif
	for (i = 0; i < opt->clusters->nclusters; i++) {
	    int cn = opt->clusters->clustersizes[i];
#ifdef WIN32
	    Variable** cvs = N_GNEW(cn + 2, Variable*);
	    boxf* cbb = N_GNEW(cn + 2, boxf);
#else
	    Variable *cvs[cn + 2];
	    boxf cbb[cn + 2];
#endif
	    /* compute cluster bounding bb */
	    boxf container;
	    container.LL.x = container.LL.y = DBL_MAX;
	    container.UR.x = container.UR.y = -DBL_MAX;
	    for (j = 0; j < cn; j++) {
		int iv = opt->clusters->clusters[i][j];
		cvs[j] = e->vs[iv];
		B2BF(bb[iv], cbb[j]);
		EXPANDBB(container, bb[iv]);
	    }
	    B2BF(container, opt->clusters->bb[i]);
	    cvs[cn] = e->vs[n + 2 * i];
	    cvs[cn + 1] = e->vs[n + 2 * i + 1];
	    B2BF(container, cbb[cn]);
	    B2BF(container, cbb[cn + 1]);
	    if (k == 0) {
		cbb[cn].UR.x = container.LL.x + 0.0001;
		cbb[cn + 1].LL.x = container.UR.x - 0.0001;
		cm[i] =
		    genXConstraints(cn + 2, cbb, cvs, &cscl[i],
				    transitiveClosure);
	    } else {
		cbb[cn].UR.y = container.LL.y + 0.0001;
		cbb[cn + 1].LL.y = container.UR.y - 0.0001;
		cm[i] = genYConstraints(cn + 2, cbb, cvs, &cscl[i]);
	    }
	    mol += cm[i];
#ifdef WIN32
	    free (cvs);
	    free (cbb);
#endif
	}
	/* generate top level constraints */
	{
	    int cn = opt->clusters->ntoplevel + opt->clusters->nclusters;
#ifdef WIN32
	    Variable** cvs = N_GNEW(cn,Variable*);
	    boxf* cbb = N_GNEW(cn, boxf);
#else
	    Variable *cvs[cn];
	    boxf cbb[cn];
#endif
	    for (i = 0; i < opt->clusters->ntoplevel; i++) {
		int iv = opt->clusters->toplevel[i];
		cvs[i] = e->vs[iv];
		B2BF(bb[iv], cbb[i]);
	    }
	    /* make dummy variables for clusters */
	    for (i = opt->clusters->ntoplevel; i < cn; i++) {
		cvs[i] = newVariable(123 + i, 1, 1);
		j = i - opt->clusters->ntoplevel;
		B2BF(opt->clusters->bb[j], cbb[i]);
	    }
	    i = opt->clusters->nclusters;
	    if (k == 0) {
		cm[i] =
		    genXConstraints(cn, cbb, cvs, &cscl[i],
				    transitiveClosure);
	    } else {
		cm[i] = genYConstraints(cn, cbb, cvs, &cscl[i]);
	    }
	    /* remap constraints from tmp dummy vars to cluster l and r vars */
	    for (i = opt->clusters->ntoplevel; i < cn; i++) {
		double dgap;
		j = i - opt->clusters->ntoplevel;
		/* dgap is the change in required constraint gap.
		 * since we are going from a source rectangle the size
		 * of the cluster bounding box to a zero width (in x dim,
		 * zero height in y dim) rectangle, the change will be
		 * half the bb width.
		 */
		if (k == 0) {
		    dgap = -(cbb[i].UR.x - cbb[i].LL.x) / 2.0;
		} else {
		    dgap = -(cbb[i].UR.y - cbb[i].LL.y) / 2.0;
		}
		remapInConstraints(cvs[i], e->vs[n + 2 * j], dgap);
		remapOutConstraints(cvs[i], e->vs[n + 2 * j + 1], dgap);
		/* there may be problems with cycles between
		 * cluster non-overlap and diredge constraints,
		 * to resolve:
		 * 
		 * for each constraint c:v->cvs[i]:
		 *   if exists diredge constraint u->v where u in c:
		 *     remap v->cl to cr->v (gap = height(v)/2)
		 *
		 * in = getInConstraints(cvs[i])
		 * for(c : in) {
		 *   assert(c.right==cvs[i]);
		 *   vin = getOutConstraints(v=c.left)
		 *   for(d : vin) {
		 *     if(d.left.cluster==i):
		 *       tmp = d.left
		 *       d.left = d.right
		 *       d.right = tmp
		 *       d.gap = height(d.right)/2
		 *   }
		 * }
		 *       
		 */
		deleteVariable(cvs[i]);
	    }
	    mol += cm[opt->clusters->nclusters];
#ifdef WIN32
	    free (cvs);
	    free (cbb);
#endif
	}
	csolptr = csol = newConstraints(mol);
	for (i = 0; i < opt->clusters->nclusters + 1; i++) {
	    /* copy constraints into csol */
	    for (j = 0; j < cm[i]; j++) {
		*csolptr++ = cscl[i][j];
	    }
	    deleteConstraints(0, cscl[i]);
	}
#ifdef WIN32
	free (cscl);
	free (cm);
#endif
    } else {
	if (k == 0) {
	    mol = genXConstraints(n, bb, e->vs, &csol, transitiveClosure);
	} else {
	    mol = genYConstraints(n, bb, e->vs, &csol);
	}
    }
    /* remove constraints from previous iteration */
    if (e->m > 0) {
	/* can't reuse instance of VPSC when constraints change! */
	deleteVPSC(e->vpsc);
	for (i = e->gm == 0 ? 0 : e->gm; i < e->m; i++) {
	    /* delete previous overlap constraints */
	    deleteConstraint(e->cs[i]);
	}
	/* just delete the array, not the elements */
	if (e->cs != e->gcs)
	    deleteConstraints(0, e->cs);
    }
    /* if we have no global constraints then the overlap constraints
     * are all we have to worry about.
     * Otherwise, we have to copy the global and overlap constraints 
     * into the one array
     */
    if (e->gm == 0) {
	e->m = mol;
	e->cs = csol;
    } else {
	e->m = mol + e->gm;
	e->cs = newConstraints(e->m);
	for (i = 0; i < e->m; i++) {
	    if (i < e->gm) {
		e->cs[i] = e->gcs[i];
	    } else {
		e->cs[i] = csol[i - e->gm];
	    }
	}
	/* just delete the array, not the elements */
	deleteConstraints(0, csol);
    }
    if (Verbose)
	fprintf(stderr, "  generated %d constraints\n", e->m);
    e->vpsc = newIncVPSC(e->nv + e->nldv + e->ndv, e->vs, e->m, e->cs);
#ifdef MOSEK
    if (opt->mosek) {
	if (e->mosekEnv != NULL) {
	    mosek_delete(e->mosekEnv);
	}
	e->mosekEnv =
	    mosek_init_sep(e->packedMat, e->nv + e->nldv, e->ndv, e->cs,
			   e->m);
    }
#endif
#ifdef WIN32
    free (bb);
#endif
}

/*
 * Statically remove overlaps, that is remove all overlaps by moving each node as
 * little as possible.
 */
void removeoverlaps(int n, float **coords, ipsep_options * opt)
{
    int i;
    CMajEnvVPSC *e = initCMajVPSC(n, NULL, NULL, opt, 0);
    generateNonoverlapConstraints(e, 1.0, coords, 0, TRUE, opt);
    solveVPSC(e->vpsc);
    for (i = 0; i < n; i++) {
	coords[0][i] = getVariablePos(e->vs[i]);
    }
    generateNonoverlapConstraints(e, 1.0, coords, 1, FALSE, opt);
    solveVPSC(e->vpsc);
    for (i = 0; i < n; i++) {
	coords[1][i] = getVariablePos(e->vs[i]);
    }
    deleteCMajEnvVPSC(e);
}

/*
 unpack the "ordering" array into an array of DigColaLevel
*/
DigColaLevel *assign_digcola_levels(int *ordering, int n, int *level_inds,
				    int num_divisions)
{
    int i, j;
    DigColaLevel *l = N_GNEW(num_divisions + 1, DigColaLevel);
    /* first level */
    l[0].num_nodes = level_inds[0];
    l[0].nodes = N_GNEW(l[0].num_nodes, int);
    for (i = 0; i < l[0].num_nodes; i++) {
	l[0].nodes[i] = ordering[i];
    }
    /* second through second last level */
    for (i = 1; i < num_divisions; i++) {
	l[i].num_nodes = level_inds[i] - level_inds[i - 1];
	l[i].nodes = N_GNEW(l[i].num_nodes, int);
	for (j = 0; j < l[i].num_nodes; j++) {
	    l[i].nodes[j] = ordering[level_inds[i - 1] + j];
	}
    }
    /* last level */
    if (num_divisions > 0) {
	l[num_divisions].num_nodes = n - level_inds[num_divisions - 1];
	l[num_divisions].nodes = N_GNEW(l[num_divisions].num_nodes, int);
	for (i = 0; i < l[num_divisions].num_nodes; i++) {
	    l[num_divisions].nodes[i] =
		ordering[level_inds[num_divisions - 1] + i];
	}
    }
    return l;
}
void delete_digcola_levels(DigColaLevel * l, int num_levels)
{
    int i;
    for (i = 0; i < num_levels; i++) {
	free(l[i].nodes);
    }
    free(l);
}
void print_digcola_levels(FILE * logfile, DigColaLevel * levels,
			  int num_levels)
{
    int i, j;
    fprintf(logfile, "levels:\n");
    for (i = 0; i < num_levels; i++) {
	fprintf(logfile, "  l[%d]:", i);
	for (j = 0; j < levels[i].num_nodes; j++) {
	    fprintf(logfile, "%d ", levels[i].nodes[j]);
	}
	fprintf(logfile, "\n");
    }
}

/*********************
get number of separation constraints based on the number of nodes in each level
ie, num_sep_constraints = sum_i^{num_levels-1} (|L[i]|+|L[i+1]|)
**********************/
int get_num_digcola_constraints(DigColaLevel * levels, int num_levels)
{
    int i, nc = 0;
    for (i = 1; i < num_levels; i++) {
	nc += levels[i].num_nodes + levels[i - 1].num_nodes;
    }
    nc += levels[0].num_nodes + levels[num_levels - 1].num_nodes;
    return nc;
}

#endif				/* IPSEPCOLA */
