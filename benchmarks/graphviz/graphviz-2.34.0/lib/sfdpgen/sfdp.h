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

#ifndef SFDP_H
#define SFDP_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "render.h"

void sfdp_layout (graph_t * g);
void sfdp_cleanup (graph_t * g);
int fdpAdjust (graph_t * g);

#endif
 
