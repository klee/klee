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

#include <digcola.h>
#ifdef DIGCOLA
#include "kkutils.h"

static int *given_levels = NULL;
/*
 * This function partitions the graph nodes into levels
 * according to the minimizer of the hierarchy energy.
 * 
 * To allow more flexibility we define a new level only 
 * when the hierarchy energy shows a significant jump
 * (to compensate for noise).
 * This is controlled by two parameters: 'abs_tol' and 
 * 'relative_tol'. The smaller these two are, the more 
 * levels we'll get. 
 * More speciffically:
 * We never consider gaps smaller than 'abs_tol'
 * Additionally, we never consider gaps smaller than 'abs_tol'*<avg_gap>
 * 
 * The output is an ordering of the nodes according to 
 * their levels, as follows:
 *   First level: 
 *     ordering[0],ordering[1],...ordering[levels[0]-1]
 *   Second level: 
 *     ordering[levels[0]],ordering[levels[0]+1],...ordering[levels[1]-1]
 *   ...
 *   Last level: 
 *     ordering[levels[num_levels-1]],ordering[levels[num_levels-1]+1],...ordering[n-1]
 * 
 * Hence, the nodes were partitioned into 'num_levels'+1
 * levels.
 * 
 * Please note that 'ordering[levels[i]]' contains the first node at level i+1,
 *  and not the last node of level i.
 */
int
compute_hierarchy(vtx_data * graph, int n, double abs_tol,
		  double relative_tol, double *given_coords,
		  int **orderingp, int **levelsp, int *num_levelsp)
{
    double *y;
    int i, rv=0;
    double spread;
    int use_given_levels = FALSE;
    int *ordering;
    int *levels;
    double tol;			/* node 'i' precedes 'j' in hierachy iff y[i]-y[j]>tol */
    double hierarchy_span;
    int num_levels;

    /* compute optimizer of hierarchy energy: 'y' */
    if (given_coords) {
	y = given_coords;
    } else {
	y = N_GNEW(n, double);
	if (compute_y_coords(graph, n, y, n)) {
	    rv = 1;
	    goto finish;    
	}
    }

    /* sort nodes accoridng to their y-ordering */
    *orderingp = ordering = N_NEW(n, int);
    for (i = 0; i < n; i++) {
	ordering[i] = i;
    }
    quicksort_place(y, ordering, 0, n - 1);

    spread = y[ordering[n - 1]] - y[ordering[0]];

    /* after spread is computed, we may take the y-coords as the given levels */
    if (given_levels) {
	use_given_levels = TRUE;
	for (i = 0; i < n; i++) {
	    use_given_levels = use_given_levels && given_levels[i] >= 0;
	}
    }
    if (use_given_levels) {
	for (i = 0; i < n; i++) {
	    y[i] = given_levels[i];
	    ordering[i] = i;
	}
	quicksort_place(y, ordering, 0, n - 1);
    }

    /* compute tolerance
     * take the maximum between 'abs_tol' and a fraction of the average gap
     * controlled by 'relative_tol'
     */
    hierarchy_span = y[ordering[n - 1]] - y[ordering[0]];
    tol = MAX(abs_tol, relative_tol * hierarchy_span / (n - 1));
    /* 'hierarchy_span/(n-1)' - average gap between consecutive nodes */


    /* count how many levels the hierarchy contains (a SINGLE_LINK clustering */
    /* alternatively we could use COMPLETE_LINK clustering) */
    num_levels = 0;
    for (i = 1; i < n; i++) {
	if (y[ordering[i]] - y[ordering[i - 1]] > tol) {
	    num_levels++;
	}
    }
    *num_levelsp = num_levels;
    if (num_levels == 0) {
	*levelsp = levels = N_GNEW(1, int);
	levels[0] = n;
    } else {
	int count = 0;
	*levelsp = levels = N_GNEW(num_levels, int);
	for (i = 1; i < n; i++) {
	    if (y[ordering[i]] - y[ordering[i - 1]] > tol) {
		levels[count++] = i;
	    }
	}
    }
finish:
    if (!given_coords) {
	free(y);
    }

    return rv;
}

#endif				/* DIGCOLA */
