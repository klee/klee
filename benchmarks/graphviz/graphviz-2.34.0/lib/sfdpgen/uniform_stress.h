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

#ifndef UNIFORMSTRESS_H
#define UNIFORMSTRESS_H

#include <post_process.h> 

typedef StressMajorizationSmoother UniformStressSmoother;

#define UniformStressSmoother_struct StressMajorizationSmoother_struct

void UniformStressSmoother_delete(UniformStressSmoother sm);

UniformStressSmoother UniformStressSmoother_new(int dim, SparseMatrix A, real *x, real alpha, real M, int *flag);

void uniform_stress(int dim, SparseMatrix A, real *x, int *flag);

#endif
