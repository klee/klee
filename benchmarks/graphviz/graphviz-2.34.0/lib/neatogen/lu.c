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
 * This code was (mostly) written by Ken Turkowski, who said:
 *
 * Oh, that. I wrote it in college the first time. It's open source - I think I
 * posted it after seeing so many people solve equations by inverting matrices
 * by computing minors naïvely.
 * -Ken
 *
 * The views represented here are mine and are not necessarily shared by
 * my employer.
   	Ken Turkowski			turk@apple.com
	Immersive Media Technologist 	http://www.worldserver.com/turk/
	Apple Computer, Inc.
	1 Infinite Loop, MS 302-3VR
	Cupertino, CA 95014
 */



/* This module solves linear equations in several variables (Ax = b) using
 * LU decomposition with partial pivoting and row equilibration.  Although
 * slightly more work than Gaussian elimination, it is faster for solving
 * several equations using the same coefficient matrix.  It is
 * particularly useful for matrix inversion, by sequentially solving the
 * equations with the columns of the unit matrix.
 *
 * lu_decompose() decomposes the coefficient matrix into the LU matrix,
 * and lu_solve() solves the series of matrix equations using the
 * previous LU decomposition.
 *
 *	Ken Turkowski (apple!turk)
 *	written 3/2/79, revised and enhanced 8/9/83.
 */

#include <math.h>
#include <neato.h>

static double *scales;
static double **lu;
static int *ps;

/* lu_decompose() decomposes the coefficient matrix A into upper and lower
 * triangular matrices, the composite being the LU matrix.
 *
 * The arguments are:
 *
 *	a - the (n x n) coefficient matrix
 *	n - the order of the matrix
 *
 *  1 is returned if the decomposition was successful,
 *  and 0 is returned if the coefficient matrix is singular.
 */

int lu_decompose(double **a, int n)
{
    register int i, j, k;
    int pivotindex = 0;
    double pivot, biggest, mult, tempf;

    if (lu)
	free_array(lu);
    lu = new_array(n, n, 0.0);
    if (ps)
	free(ps);
    ps = N_NEW(n, int);
    if (scales)
	free(scales);
    scales = N_NEW(n, double);

    for (i = 0; i < n; i++) {	/* For each row */
	/* Find the largest element in each row for row equilibration */
	biggest = 0.0;
	for (j = 0; j < n; j++)
	    if (biggest < (tempf = fabs(lu[i][j] = a[i][j])))
		biggest = tempf;
	if (biggest != 0.0)
	    scales[i] = 1.0 / biggest;
	else {
	    scales[i] = 0.0;
	    return (0);		/* Zero row: singular matrix */
	}
	ps[i] = i;		/* Initialize pivot sequence */
    }

    for (k = 0; k < n - 1; k++) {	/* For each column */
	/* Find the largest element in each column to pivot around */
	biggest = 0.0;
	for (i = k; i < n; i++) {
	    if (biggest < (tempf = fabs(lu[ps[i]][k]) * scales[ps[i]])) {
		biggest = tempf;
		pivotindex = i;
	    }
	}
	if (biggest == 0.0)
	    return (0);		/* Zero column: singular matrix */
	if (pivotindex != k) {	/* Update pivot sequence */
	    j = ps[k];
	    ps[k] = ps[pivotindex];
	    ps[pivotindex] = j;
	}

	/* Pivot, eliminating an extra variable  each time */
	pivot = lu[ps[k]][k];
	for (i = k + 1; i < n; i++) {
	    lu[ps[i]][k] = mult = lu[ps[i]][k] / pivot;
	    if (mult != 0.0) {
		for (j = k + 1; j < n; j++)
		    lu[ps[i]][j] -= mult * lu[ps[k]][j];
	    }
	}
    }

    if (lu[ps[n - 1]][n - 1] == 0.0)
	return (0);		/* Singular matrix */
    return (1);
}

/* lu_solve() solves the linear equation (Ax = b) after the matrix A has
 * been decomposed with lu_decompose() into the lower and upper triangular
 * matrices L and U.
 *
 * The arguments are:
 *
 *	x - the solution vector
 *	b - the constant vector
 *	n - the order of the equation
*/

void lu_solve(double *x, double *b, int n)
{
    register int i, j;
    double dot;

    /* Vector reduction using U triangular matrix */
    for (i = 0; i < n; i++) {
	dot = 0.0;
	for (j = 0; j < i; j++)
	    dot += lu[ps[i]][j] * x[j];
	x[i] = b[ps[i]] - dot;
    }

    /* Back substitution, in L triangular matrix */
    for (i = n - 1; i >= 0; i--) {
	dot = 0.0;
	for (j = i + 1; j < n; j++)
	    dot += lu[ps[i]][j] * x[j];
	x[i] = (x[i] - dot) / lu[ps[i]][i];
    }
}
