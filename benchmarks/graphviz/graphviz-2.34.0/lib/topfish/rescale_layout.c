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

///////////////////////////////////////
//                                   // 
// This file contains the functions  //
// for distorting the layout.        //
//                                   //
// Four methods are available:       //
// 1) Uniform denisity - rectilinear //
// 2) Uniform denisity - polar       //
// 3) Fisheye - rectilinear          //
// 4) Fisheye - Ploar                //
//                                   // 
///////////////////////////////////////


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include "matrix_ops.h"
/* #include "hierarchy.h" */
#include "delaunay.h"
#include "memory.h"
#include "arith.h"

static double *compute_densities(v_data * graph, int n, double *x,
				 double *y)
{
// compute density of every node by calculating the average edge length in a 2-D layout
    int i, j, neighbor;
    double sum;
    double *densities = N_NEW(n, double);

    for (i = 0; i < n; i++) {
	sum = 0;
	for (j = 1; j < graph[i].nedges; j++) {
	    neighbor = graph[i].edges[j];
	    sum +=
		sqrt((x[i] - x[neighbor]) * (x[i] - x[neighbor]) +
		     (y[i] - y[neighbor]) * (y[i] - y[neighbor]));
	}
	densities[i] = sum / graph[i].nedges;
    }
    return densities;
}

static double *recompute_densities(v_data * graph, int n, double *x,
				   double *densities)
{
// compute density of every node by calculating the average edge length in a 1-D layout
    int i, j, neighbor;
    double sum;
    densities = RALLOC(n, densities, double);

    for (i = 0; i < n; i++) {
	sum = 0;
	for (j = 1; j < graph[i].nedges; j++) {
	    neighbor = graph[i].edges[j];
	    sum += fabs(x[i] - x[neighbor]);
	}
	densities[i] = sum / graph[i].nedges;
    }
    return densities;
}

static double *smooth_vec(double *vec, int *ordering, int n, int interval,
			  double *smoothed_vec)
{
// smooth 'vec' by setting each components to the average of is 'interval'-neighborhood in 'ordering'
    int len, i, n1;
	double sum;
    smoothed_vec = RALLOC(n, smoothed_vec, double);
    n1 = MIN(1 + interval, n);
    sum = 0;
    for (i = 0; i < n1; i++) {
	sum += vec[ordering[i]];
    }

    len = n1;
    for (i = 0; i < MIN(n, interval); i++) {
	smoothed_vec[ordering[i]] = sum / len;
	if (len < n) {
	    sum += vec[ordering[len++]];
	}
    }
    if (n <= interval) {
	return smoothed_vec;
    }

    for (i = interval; i < n - interval - 1; i++) {
	smoothed_vec[ordering[i]] = sum / len;
	sum +=
	    vec[ordering[i + interval + 1]] - vec[ordering[i - interval]];
    }
    for (i = MAX(n - interval - 1, interval); i < n; i++) {
	smoothed_vec[ordering[i]] = sum / len;
	sum -= vec[ordering[i - interval]];
	len--;
    }
    return smoothed_vec;
}

/* quicksort_place:
 * Available in lib/neatogen.
 */
static int
split_by_place(double *place, int *nodes, int first, int last)
{
    int middle;
    unsigned int splitter=((unsigned int)rand()|((unsigned int)rand())<<16)%(unsigned int)(last-first+1)+(unsigned int)first;
    int val;
    double place_val;
    int left = first + 1;
    int right = last;
    int temp;

    val = nodes[splitter];
    nodes[splitter] = nodes[first];
    nodes[first] = val;
    place_val = place[val];

    while (left < right) {
	while (left < right && place[nodes[left]] <= place_val)
	    left++;
        /* use here ">" and not ">=" to enable robustness
         * by ensuring that ALL equal values move to the same side
         */
	while (left < right && place[nodes[right]] > place_val)
	    right--;
	if (left < right) {
	    temp = nodes[left];
	    nodes[left] = nodes[right];
	    nodes[right] = temp;
	    left++;
	    right--;		/* (1) */

	}
    }
    /* at this point either, left==right (meeting), or 
     * left=right+1 (because of (1)) 
     * we have to decide to which part the meeting point (or left) belongs.
     *
     * notice that always left>first, because of its initialization
     */
    if (place[nodes[left]] > place_val)
	left = left - 1;
    middle = left;
    nodes[first] = nodes[middle];
    nodes[middle] = val;
    return middle;
}

static int 
sorted_place(double * place, int * ordering, int first, int last)
{
    int i, isSorted = 1; 
    for (i=first+1; i<=last && isSorted; i++) {
        if (place[ordering[i-1]]>place[ordering[i]]) {
            isSorted = 0;
        }
    }
    return isSorted;
}

void quicksort_place(double *place, int *ordering, int first, int last)
{
    if (first < last) {
	int middle = split_by_place(place, ordering, first, last);
        /* Checking for "already sorted" dramatically improves running time 
	 * and robustness (against uneven recursion) when not all values are 
         * distinct (thefore we expect emerging chunks of equal values)
	 * it never increased running time even when values were distinct
         */
	if (!sorted_place(place,ordering,first,middle-1))
	    quicksort_place(place,ordering,first,middle-1);
	if (!sorted_place(place,ordering,middle+1,last))
	    quicksort_place(place,ordering,middle+1,last);
    }
}

static void
rescaleLayout(v_data * graph, int n, double *x_coords, double *y_coords,
	      int interval, double distortion)
{
    // Rectlinear distortion - auxilliary function
    int i;
    double *densities = NULL, *smoothed_densities = NULL;
    double *copy_coords = N_NEW(n, double);
    int *ordering = N_NEW(n, int);
    double factor;

    //compute_densities(graph, n, x_coords, y_coords, densities);

    for (i = 0; i < n; i++) {
	ordering[i] = i;
    }

    // just to make milder behavior:
    if (distortion >= 0) {
	factor = sqrt(distortion);
    } else {
	factor = -sqrt(-distortion);
    }

    quicksort_place(x_coords, ordering, 0, n - 1);
    densities = recompute_densities(graph, n, x_coords, densities);
    smoothed_densities = smooth_vec(densities, ordering, n, interval, smoothed_densities);
    cpvec(copy_coords, 0, n - 1, x_coords);
    for (i = 1; i < n; i++) {
	x_coords[ordering[i]] =
	    x_coords[ordering[i - 1]] + (copy_coords[ordering[i]] -
					 copy_coords[ordering[i - 1]]) /
	    pow(smoothed_densities[ordering[i]], factor);
    }

    quicksort_place(y_coords, ordering, 0, n - 1);
    densities = recompute_densities(graph, n, y_coords, densities);
    smoothed_densities = smooth_vec(densities, ordering, n, interval, smoothed_densities);
    cpvec(copy_coords, 0, n - 1, y_coords);
    for (i = 1; i < n; i++) {
	y_coords[ordering[i]] =
	    y_coords[ordering[i - 1]] + (copy_coords[ordering[i]] -
					 copy_coords[ordering[i - 1]]) /
	    pow(smoothed_densities[ordering[i]], factor);
    }

    free(densities);
    free(smoothed_densities);
    free(copy_coords);
    free(ordering);
}

void
rescale_layout(double *x_coords, double *y_coords,
	       int n, int interval, double width, double height,
	       double margin, double distortion)
{
    // Rectlinear distortion - main function
    int i;
    double minX, maxX, minY, maxY;
    double aspect_ratio;
    v_data *graph;
    double scaleX;
    double scale_ratio;

    width -= 2 * margin;
    height -= 2 * margin;

    // compute original aspect ratio
    minX = maxX = x_coords[0];
    minY = maxY = y_coords[0];
    for (i = 1; i < n; i++) {
	if (x_coords[i] < minX)
	    minX = x_coords[i];
	if (y_coords[i] < minY)
	    minY = y_coords[i];
	if (x_coords[i] > maxX)
	    maxX = x_coords[i];
	if (y_coords[i] > maxY)
	    maxY = y_coords[i];
    }
    aspect_ratio = (maxX - minX) / (maxY - minY);

    // construct mutual neighborhood graph
    graph = UG_graph(x_coords, y_coords, n, 0);
    rescaleLayout(graph, n, x_coords, y_coords, interval, distortion);
    free(graph[0].edges);
    free(graph);

    // compute new aspect ratio
    minX = maxX = x_coords[0];
    minY = maxY = y_coords[0];
    for (i = 1; i < n; i++) {
	if (x_coords[i] < minX)
	    minX = x_coords[i];
	if (y_coords[i] < minY)
	    minY = y_coords[i];
	if (x_coords[i] > maxX)
	    maxX = x_coords[i];
	if (y_coords[i] > maxY)
	    maxY = y_coords[i];
    }

    // shift points:
    for (i = 0; i < n; i++) {
	x_coords[i] -= minX;
	y_coords[i] -= minY;
    }

    // rescale x_coords to maintain aspect ratio:
    scaleX = aspect_ratio * (maxY - minY) / (maxX - minX);
    for (i = 0; i < n; i++) {
	x_coords[i] *= scaleX;
    }

    // scale the layout to fit full drawing area:
    scale_ratio =
	MIN((width) / (aspect_ratio * (maxY - minY)),
	    (height) / (maxY - minY));
    for (i = 0; i < n; i++) {
	x_coords[i] *= scale_ratio;
	y_coords[i] *= scale_ratio;
    }

    for (i = 0; i < n; i++) {
	x_coords[i] += margin;
	y_coords[i] += margin;
    }
}

#define DIST(x1,y1,x2,y2) (sqrt((x1-x2)*(x1-x2)+(y1-y2)*(y1-y2)))

static void
rescale_layout_polarFocus(v_data * graph, int n,
	  double *x_coords, double *y_coords,
	  double x_focus, double y_focus, int interval, double distortion)
{
    // Polar distortion - auxilliary function
    int i;
    double *densities = NULL, *smoothed_densities = NULL;
    double *distances = N_NEW(n, double);
    double *orig_distances = N_NEW(n, double);
    int *ordering;
    double ratio;

    for (i = 0; i < n; i++) 
	{
		distances[i] = DIST(x_coords[i], y_coords[i], x_focus, y_focus);
    }
    cpvec(orig_distances, 0, n - 1, distances);

    ordering = N_NEW(n, int);
    for (i = 0; i < n; i++) 
	{
		ordering[i] = i;
    }
    quicksort_place(distances, ordering, 0, n - 1);

    densities = compute_densities(graph, n, x_coords, y_coords);
    smoothed_densities = smooth_vec(densities, ordering, n, interval, smoothed_densities);

    // rescale distances
    if (distortion < 1.01 && distortion > 0.99) 
	{
		for (i = 1; i < n; i++) 
		{
			distances[ordering[i]] =	distances[ordering[i - 1]] + (orig_distances[ordering[i]] -
					      orig_distances[ordering
							     [i -
							      1]]) / smoothed_densities[ordering[i]];
		}
    } else 
	{
		double factor;
		// just to make milder behavior:
		if (distortion >= 0) 
		{
			factor = sqrt(distortion);
		} 
		else 
		{
			factor = -sqrt(-distortion);
		}
		for (i = 1; i < n; i++) 
		{
			distances[ordering[i]] =
				distances[ordering[i - 1]] + (orig_distances[ordering[i]] -
					      orig_distances[ordering
							     [i -
							      1]]) /
			pow(smoothed_densities[ordering[i]], factor);
		}
    }

    // compute new coordinate:
    for (i = 0; i < n; i++) 
	{
		if (orig_distances[i] == 0) 
		{
			ratio = 0;
		} 
		else 
		{
			ratio = distances[i] / orig_distances[i];
		}
		x_coords[i] = x_focus + (x_coords[i] - x_focus) * ratio;
		y_coords[i] = y_focus + (y_coords[i] - y_focus) * ratio;
    }

    free(densities);
    free(smoothed_densities);
    free(distances);
    free(orig_distances);
    free(ordering);
}

void
rescale_layout_polar(double *x_coords, double *y_coords,
		     double *x_foci, double *y_foci, int num_foci,
		     int n, int interval, double width,
		     double height, double margin, double distortion)
{
    // Polar distortion - main function
    int i;
    double minX, maxX, minY, maxY;
    double aspect_ratio;
    v_data *graph;
    double scaleX;
    double scale_ratio;

    width -= 2 * margin;
    height -= 2 * margin;

    // compute original aspect ratio
    minX = maxX = x_coords[0];
    minY = maxY = y_coords[0];
    for (i = 1; i < n; i++) 
	{
		if (x_coords[i] < minX)
		    minX = x_coords[i];
		if (y_coords[i] < minY)
			minY = y_coords[i];
		if (x_coords[i] > maxX)
			maxX = x_coords[i];
		if (y_coords[i] > maxY)
			maxY = y_coords[i];
    }
    aspect_ratio = (maxX - minX) / (maxY - minY);

    // construct mutual neighborhood graph
    graph = UG_graph(x_coords, y_coords, n, 0);

    if (num_foci == 1) 
	{	// accelerate execution of most common case
		rescale_layout_polarFocus(graph, n, x_coords, y_coords, x_foci[0],
				  y_foci[0], interval, distortion);
    } else
	{
	// average-based rescale
	double *final_x_coords = N_NEW(n, double);
	double *final_y_coords = N_NEW(n, double);
	double *cp_x_coords = N_NEW(n, double);
	double *cp_y_coords = N_NEW(n, double);
	for (i = 0; i < n; i++) {
	    final_x_coords[i] = final_y_coords[i] = 0;
	}
	for (i = 0; i < num_foci; i++) {
	    cpvec(cp_x_coords, 0, n - 1, x_coords);
	    cpvec(cp_y_coords, 0, n - 1, y_coords);
	    rescale_layout_polarFocus(graph, n, cp_x_coords, cp_y_coords,
				      x_foci[i], y_foci[i], interval, distortion);
	    scadd(final_x_coords, 0, n - 1, 1.0 / num_foci, cp_x_coords);
	    scadd(final_y_coords, 0, n - 1, 1.0 / num_foci, cp_y_coords);
	}
	cpvec(x_coords, 0, n - 1, final_x_coords);
	cpvec(y_coords, 0, n - 1, final_y_coords);
	free(final_x_coords);
	free(final_y_coords);
	free(cp_x_coords);
	free(cp_y_coords);
    }
    free(graph[0].edges);
    free(graph);

    minX = maxX = x_coords[0];
    minY = maxY = y_coords[0];
    for (i = 1; i < n; i++) {
	if (x_coords[i] < minX)
	    minX = x_coords[i];
	if (y_coords[i] < minY)
	    minY = y_coords[i];
	if (x_coords[i] > maxX)
	    maxX = x_coords[i];
	if (y_coords[i] > maxY)
	    maxY = y_coords[i];
    }

    // shift points:
    for (i = 0; i < n; i++) {
	x_coords[i] -= minX;
	y_coords[i] -= minY;
    }

    // rescale x_coords to maintain aspect ratio:
    scaleX = aspect_ratio * (maxY - minY) / (maxX - minX);
    for (i = 0; i < n; i++) {
	x_coords[i] *= scaleX;
    }


    // scale the layout to fit full drawing area:
    scale_ratio =
	MIN((width) / (aspect_ratio * (maxY - minY)),
	    (height) / (maxY - minY));
    for (i = 0; i < n; i++) {
	x_coords[i] *= scale_ratio;
	y_coords[i] *= scale_ratio;
    }

    for (i = 0; i < n; i++) {
	x_coords[i] += margin;
	y_coords[i] += margin;
    }
}
