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
 * this implements the resistor circuit current model for
 * computing node distance, as an alternative to shortest-path.
 * likely it could be improved by using edge weights, somehow.
 * Return 1 if successful; 0 otherwise (e.g., graph is disconnected).
 */
#include	"neato.h"

int solveCircuit(int nG, double **Gm, double **Gm_inv)
{
    double sum;
    int i, j;

    if (Verbose)
	fprintf(stderr, "Calculating circuit model");

    /* set diagonal entries to sum of conductances but ignore nth node */
    for (i = 0; i < nG; i++) {
	sum = 0.0;
	for (j = 0; j < nG; j++)
	    if (i != j)
		sum += Gm[i][j];
	Gm[i][i] = -sum;
    }
    return matinv(Gm, Gm_inv, nG - 1);
}

int circuit_model(graph_t * g, int nG)
{
    double **Gm;
    double **Gm_inv;
    int rv;
    long i, j;
    node_t *v;
    edge_t *e;

    Gm = new_array(nG, nG, 0.0);
    Gm_inv = new_array(nG, nG, 0.0);

    /* set non-diagonal entries */
    for (v = agfstnode(g); v; v = agnxtnode(g, v)) {
	for (e = agfstedge(g, v); e; e = agnxtedge(g, e, v)) {
	    i = AGSEQ(agtail(e));
	    j = AGSEQ(aghead(e));
	    if (i == j)
		continue;
	    /* conductance is 1/resistance */
	    Gm[i][j] = Gm[j][i] = -1.0 / ED_dist(e);	/* negate */
	}
    }

    rv = solveCircuit(nG, Gm, Gm_inv);

    if (rv)
	for (i = 0; i < nG; i++) {
	    for (j = 0; j < nG; j++) {
		GD_dist(g)[i][j] =
		    Gm_inv[i][i] + Gm_inv[j][j] - 2.0 * Gm_inv[i][j];
	    }
	}
    free_array(Gm);
    free_array(Gm_inv);
    return rv;
}
