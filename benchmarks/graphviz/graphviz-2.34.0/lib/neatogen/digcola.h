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

#ifdef __cplusplus
extern "C" {
#endif

#ifndef DIGCOLA_H
#define DIGCOLA_H

#include <defs.h>
#ifdef DIGCOLA
extern int compute_y_coords(vtx_data*, int, double*, int);
extern int compute_hierarchy(vtx_data*, int, double, double, 
                                double*, int**, int**, int*); 
extern int IMDS_given_dim(vtx_data*, int, double*, double*, double);
extern int stress_majorization_with_hierarchy(vtx_data*, int, int, double**, 
                                              node_t**, int, int, int, int, double);
#ifdef IPSEPCOLA
typedef struct ipsep_options {
    int diredges;       /* 1=generate directed edge constraints */
                        /* 2=generate directed hierarchy level constraints (DiG-CoLa) */
    double edge_gap;    /* amount to force vertical separation of */
                        /* start/end nodes */
    int noverlap;       /* 1=generate non-overlap constraints */
                        /* 2=remove overlaps after layout */
    pointf gap;         /* hor and vert gap to enforce when removing overlap*/
    pointf* nsize;      /* node widths and heights */
    cluster_data* clusters;
                        /* list of node indices for each cluster */
#ifdef MOSEK
    int mosek;          /* use Mosek as constraint optimization engine */
#endif /* MOSEK */
} ipsep_options;

 /* stress majorization, for Constraint Layout */
extern int stress_majorization_cola(vtx_data*, int, int, double**, node_t**, int, int, int, ipsep_options*);
#endif
#endif
#endif

#ifdef __cplusplus
}
#endif
