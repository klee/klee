/*************************************************************************
 * Copyright (c) 2011 AT&T Intellectual Property
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors: See CVS logs. Details at http://www.graphviz.org/
 *************************************************************************/

#ifndef NEAREST_NEIGHBOR_GRAPH_ANN_H
#define NEAREST_NEIGHBOR_GRAPH_ANN_H

void nearest_neighbor_graph_ann(int nPts, int dim, int k, double eps, double *x, int *nz0, int **irn0, int **jcn0, double **val0);

#endif /* NEAREST_NEIGHBOR_GRAPH_ANN_H */
