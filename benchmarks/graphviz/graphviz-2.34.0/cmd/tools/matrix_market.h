/* $Id$Revision:  */
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
#ifndef MATRIX_MARKET_H
#define MATRIX_MARKET_H

#include "mmio.h"
#include "SparseMatrix.h"
int mm_get_type(MM_typecode typecode);
void SparseMatrix_export_matrix_market(FILE * file, SparseMatrix A,
				       char *comment);
SparseMatrix SparseMatrix_import_matrix_market(FILE * f, int format);

#endif
