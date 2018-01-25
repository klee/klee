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
#include <math.h>
#include "solvers.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif

#ifndef HAVE_CBRT
#define cbrt(x) ((x < 0) ? (-1*pow(-x, 1.0/3.0)) : pow (x, 1.0/3.0))
#endif
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define EPS 1E-7
#define AEQ0(x) (((x) < EPS) && ((x) > -EPS))

int solve3(double *coeff, double *roots)
{
    double a, b, c, d;
    int rootn, i;
    double p, q, disc, b_over_3a, c_over_a, d_over_a;
    double r, theta, temp, alpha, beta;

    a = coeff[3], b = coeff[2], c = coeff[1], d = coeff[0];
    if (AEQ0(a))
	return solve2(coeff, roots);
    b_over_3a = b / (3 * a);
    c_over_a = c / a;
    d_over_a = d / a;

    p = b_over_3a * b_over_3a;
    q = 2 * b_over_3a * p - b_over_3a * c_over_a + d_over_a;
    p = c_over_a / 3 - p;
    disc = q * q + 4 * p * p * p;

    if (disc < 0) {
	r = .5 * sqrt(-disc + q * q);
	theta = atan2(sqrt(-disc), -q);
	temp = 2 * cbrt(r);
	roots[0] = temp * cos(theta / 3);
	roots[1] = temp * cos((theta + M_PI + M_PI) / 3);
	roots[2] = temp * cos((theta - M_PI - M_PI) / 3);
	rootn = 3;
    } else {
	alpha = .5 * (sqrt(disc) - q);
	beta = -q - alpha;
	roots[0] = cbrt(alpha) + cbrt(beta);
	if (disc > 0)
	    rootn = 1;
	else
	    roots[1] = roots[2] = -.5 * roots[0], rootn = 3;
    }

    for (i = 0; i < rootn; i++)
	roots[i] -= b_over_3a;

    return rootn;
}

int solve2(double *coeff, double *roots)
{
    double a, b, c;
    double disc, b_over_2a, c_over_a;

    a = coeff[2], b = coeff[1], c = coeff[0];
    if (AEQ0(a))
	return solve1(coeff, roots);
    b_over_2a = b / (2 * a);
    c_over_a = c / a;

    disc = b_over_2a * b_over_2a - c_over_a;
    if (disc < 0)
	return 0;
    else if (disc == 0) {
	roots[0] = -b_over_2a;
	return 1;
    } else {
	roots[0] = -b_over_2a + sqrt(disc);
	roots[1] = -2 * b_over_2a - roots[0];
	return 2;
    }
}

int solve1(double *coeff, double *roots)
{
    double a, b;

    a = coeff[1], b = coeff[0];
    if (AEQ0(a)) {
	if (AEQ0(b))
	    return 4;
	else
	    return 0;
    }
    roots[0] = -b / a;
    return 1;
}
