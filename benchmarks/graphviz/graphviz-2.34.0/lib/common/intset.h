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

#ifndef INTSET_H
#define INTSET_H

#include <cdt.h>

typedef struct {
    int       id;
    Dtlink_t  link;
} intitem;

extern Dt_t* openIntSet (void);
extern void addIntSet (Dt_t*, int);
extern int inIntSet (Dt_t*, int);
#endif
