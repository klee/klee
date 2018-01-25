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



#ifndef _CG_H_
#define _CG_H_

#include "defs.h"

/*************************
 * C.G. method - SPARSE  *
 ************************/

    extern int conjugate_gradient(vtx_data *, double *, double *, int,
				   double, int);

/*************************
 * C.G. method - DENSE   *
 ************************/

    extern int conjugate_gradient_f(float **, double *, double *, int,
				     double, int, boolean);

    extern int conjugate_gradient_mkernel(float *, float *, float *, int,
					   double, int);

#endif

#ifdef __cplusplus
}
#endif
