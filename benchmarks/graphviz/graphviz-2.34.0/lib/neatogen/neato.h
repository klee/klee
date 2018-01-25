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


#ifndef         NEATO_H
#define         NEATO_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#define MODEL_SHORTPATH      0
#define MODEL_CIRCUIT        1
#define MODEL_SUBSET         2
#define MODEL_MDS            3

#define MODE_KK          0
#define MODE_MAJOR       1
#define MODE_HIER        2
#define MODE_IPSEP       3

#define INIT_ERROR       -1
#define INIT_SELF        0
#define INIT_REGULAR     1
#define INIT_RANDOM      2

#include	"render.h"
#include	"pathplan.h"
#include	"neatoprocs.h"
#include	"adjust.h"

#endif				/* NEATO_H */
