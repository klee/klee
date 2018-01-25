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

#ifndef CONVERT_H
#define CONVERT_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#include <string.h>

#include "cgraph.h"
#include "cghdr.h"

#ifdef WIN32
#define strdup(x) _strdup(x)
#endif

    extern void gv_to_gxl(Agraph_t *, FILE *);
#ifdef HAVE_EXPAT
    extern Agraph_t *gxl_to_gv(FILE *);
#endif

#endif

#ifdef __cplusplus
}
#endif
