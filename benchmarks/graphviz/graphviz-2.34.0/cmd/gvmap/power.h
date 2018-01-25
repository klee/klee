/* $Id$Revision: */
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

#ifndef POWER_H
#define POWER_H

#include <general.h>

/* if you have a standard dense/sparse matrix, set matvec to matvec_dense/matvec_sparse*/
void power_method(void (*matvec)(void *M, int m, int n, real *u, real **v, int transposed, int *flag),
          void *A, int n, int K, int random_seed, int maxit, real tol, real **eigv, real **eigs);

void matvec_sparse(void *M, int m, int n, real *u, real **v, int transposed, int *flag);

void matvec_dense(void *M, int m, int n, real *u, real **v, int transposed, int *flag);

void mat_print_dense(real *M, int m, int n);


#endif
