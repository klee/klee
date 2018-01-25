/*************************************************************************
 * Copyright (c) 2011 AT&T Intellectual Property
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors: See CVS logs. Details at http://www.graphviz.org/
 *************************************************************************/

#ifndef NEAREST_NEIGHBOR_GRAPH_H
#define NEAREST_NEIGHBOR_GRAPH_H

SparseMatrix nearest_neighbor_graph(int nPts, int num_neigbors, int dim, double *x, double eps);

#endif /* NEAREST_NEIGHBOR_GRAPH_H */
