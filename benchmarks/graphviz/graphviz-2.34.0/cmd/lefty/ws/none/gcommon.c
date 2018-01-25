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

/* Lefteris Koutsofios - AT&T Labs Research */

#include "common.h"
#include "g.h"
#include "gcommon.h"
#include "mem.h"

int Gxfd;
Widget Groot;
Display *Gdisplay;
int Gpopdownflag;
int Gscreenn;
int Gdepth;

int Ginitgraphics (void) {
    return 0;
}

int Gtermgraphics (void) {
    return 0;
}

int Gsync (void) {
    return 0;
}

int Gresetbstate (int wi) {
    return 0;
}

int Gprocessevents (int waitflag, int mode) {
    return 0;
}
