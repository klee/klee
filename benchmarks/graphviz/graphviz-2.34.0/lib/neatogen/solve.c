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


/* solves the system ab=c using gauss reduction */
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include "render.h"
#define asub(i,j) a[(i)*n + (j)]


void solve(double *a, double *b, double *c, int n)
{				/*a[n][n],b[n],c[n] */
    double *asave, *csave;
    double amax, dum, pivot;
    register int i, ii, j;
    register int k, m, mp;
    register int istar, ip;
    register int nm, nsq, t;

    istar = 0;			/* quiet warnings */
    nsq = n * n;
    asave = N_GNEW(nsq, double);
    csave = N_GNEW(n, double);

    for (i = 0; i < n; i++)
	csave[i] = c[i];
    for (i = 0; i < nsq; i++)
	asave[i] = a[i];
    /* eliminate ith unknown */
    nm = n - 1;
    for (i = 0; i < nm; i++) {
	/* find largest pivot */
	amax = 0.;
	for (ii = i; ii < n; ii++) {
	    dum = fabs(asub(ii, i));
	    if (dum < amax)
		continue;
	    istar = ii;
	    amax = dum;
	}
	/* return if pivot is too small */
	if (amax < 1.e-10)
	    goto bad;
	/* switch rows */
	for (j = i; j < n; j++) {
	    t = istar * n + j;
	    dum = a[t];
	    a[t] = a[i * n + j];
	    a[i * n + j] = dum;
	}
	dum = c[istar];
	c[istar] = c[i];
	c[i] = dum;
	/*pivot */
	ip = i + 1;
	for (ii = ip; ii < n; ii++) {
	    pivot = a[ii * n + i] / a[i * n + i];
	    c[ii] = c[ii] - pivot * c[i];
	    for (j = 0; j < n; j++)
		a[ii * n + j] = a[ii * n + j] - pivot * a[i * n + j];
	}
    }
    /* return if last pivot is too small */
    if (fabs(a[n * n - 1]) < 1.e-10)
	goto bad;
    b[n - 1] = c[n - 1] / a[n * n - 1];
    /* back substitute */
    for (k = 0; k < nm; k++) {
	m = n - k - 2;
	b[m] = c[m];
	mp = m + 1;
	for (j = mp; j < n; j++)
	    b[m] = b[m] - a[m * n + j] * b[j];
	b[m] = b[m] / a[m * n + m];
    }
    /* restore original a,c */
    for (i = 0; i < n; i++)
	c[i] = csave[i];
    for (i = 0; i < nsq; i++)
	a[i] = asave[i];
    free(asave);
    free(csave);
    return;
  bad:
    printf("ill-conditioned\n");
    free(asave);
    free(csave);
    return;
}
