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

#include "SparseMatrix.h"
#include "logic.h"
#include "memory.h"
#include "delaunay.h"

SparseMatrix call_tri(int n, int dim, real * x)
{
    real one = 1;
    int i, ii, jj;
    SparseMatrix A;
    SparseMatrix B;
    int* edgelist = NULL;
    real* xv = N_GNEW(n, real);
    real* yv = N_GNEW(n, real);
    int numberofedges;

    for (i = 0; i < n; i++) {
	xv[i] = x[i * 2];
	yv[i] = x[i * 2 + 1];
    }

    if (n > 2) {
	edgelist = delaunay_tri (xv, yv, n, &numberofedges);
    } else {
	numberofedges = 0;
    }

    A = SparseMatrix_new(n, n, 1, MATRIX_TYPE_REAL, FORMAT_COORD);
    for (i = 0; i < numberofedges; i++) {
	ii = edgelist[i * 2];
	jj = edgelist[i * 2 + 1];
	SparseMatrix_coordinate_form_add_entries(A, 1, &(ii), &(jj), &one);
    }
    if (n == 2) {		/* if two points, add edge i->j */
	ii = 0;
	jj = 1;
	SparseMatrix_coordinate_form_add_entries(A, 1, &(ii), &(jj), &one);
    }
    for (i = 0; i < n; i++) {
	SparseMatrix_coordinate_form_add_entries(A, 1, &i, &i, &one);
    }
    B = SparseMatrix_from_coordinate_format(A);
    SparseMatrix_delete(A);
    A = SparseMatrix_symmetrize(B, FALSE);
    SparseMatrix_delete(B);
    B = A;

    free (edgelist);
    free (xv);
    free (yv);
    return B;
}

SparseMatrix call_tri2(int n, int dim, real * xx)
{
    real *x, *y;
    v_data *delaunay;
    int i, j;
    SparseMatrix A;
    SparseMatrix B;
    real one = 1;
    x = N_GNEW(n, real);
    y = N_GNEW(n, real);

    for (i = 0; i < n; i++) {
	x[i] = xx[dim * i];
	y[i] = xx[dim * i + 1];
    }

    delaunay = UG_graph(x, y, n, 0);

    A = SparseMatrix_new(n, n, 1, MATRIX_TYPE_REAL, FORMAT_COORD);

    for (i = 0; i < n; i++) {
	for (j = 1; j < delaunay[i].nedges; j++) {
	    SparseMatrix_coordinate_form_add_entries(A, 1, &i,
						     &(delaunay[i].
						       edges[j]), &one);
	}
    }
    for (i = 0; i < n; i++) {
	SparseMatrix_coordinate_form_add_entries(A, 1, &i, &i, &one);
    }
    B = SparseMatrix_from_coordinate_format(A);
    B = SparseMatrix_symmetrize(B, FALSE);
    SparseMatrix_delete(A);

    free (x);
    free (y);
    freeGraph (delaunay);
    
    return B;

}
 
