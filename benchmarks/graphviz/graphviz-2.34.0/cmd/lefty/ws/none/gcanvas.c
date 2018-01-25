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

int GCcreatewidget (
    Gwidget_t *parent, Gwidget_t *widget, int attrn, Gwattr_t *attrp
) {
    return -1;
}

int GCsetwidgetattr (Gwidget_t *widget, int attrn, Gwattr_t *attrp) {
    return 0;
}

int GCgetwidgetattr (Gwidget_t *widget, int attrn, Gwattr_t *attrp) {
    return 0;
}

int GCdestroywidget (Gwidget_t *widget) {
    return 0;
}

int GCcanvasclear (Gwidget_t *widget) {
    return 0;
}

int GCsetgfxattr (Gwidget_t *widget, Ggattr_t *ap) {
    return 0;
}

int GCgetgfxattr (Gwidget_t *widget, Ggattr_t *ap) {
    return 0;
}

int GCarrow (Gwidget_t *widget, Gpoint_t gp1, Gpoint_t gp2, Ggattr_t *ap) {
    return 0;
}

int GCline (Gwidget_t *widget, Gpoint_t gp1, Gpoint_t gp2, Ggattr_t *ap) {
    return 0;
}

int GCbox (Gwidget_t *widget, Grect_t gr, Ggattr_t *ap) {
    return 0;
}

int GCpolygon (Gwidget_t *widget, int gpn, Gpoint_t *gpp, Ggattr_t *ap) {
    return 0;
}

int GCsplinegon (Gwidget_t *widget, int gpn, Gpoint_t *gpp, Ggattr_t *ap) {
    return 0;
}

int GCarc (
    Gwidget_t *widget, Gpoint_t gc, Gsize_t gs, double ang1,
    double ang2, Ggattr_t *ap
) {
    return 0;
}

int GCtext (
    Gwidget_t *widget, Gtextline_t *tlp, int n, Gpoint_t go,
    char *fn, double fs, char *justs, Ggattr_t *ap
) {
    return 0;
}

int GCgettextsize (
    Gwidget_t *widget, Gtextline_t *tlp, int n, char *fn,
    double fs, Gsize_t *gsp
) {
    return -1;
}

int GCcreatebitmap (Gwidget_t *widget, Gbitmap_t *bitmap, Gsize_t s) {
    return 0;
}

int GCdestroybitmap (Gbitmap_t *bitmap) {
    return 0;
}

#define COMPDIFF(a, b) (((a) > (b)) ? (a) - (b) : (b) - (a))
#define CDIFF(a, b) ( \
    COMPDIFF (a.red, b[0]) + COMPDIFF (a.green, b[1]) + \
    COMPDIFF (a.blue, b[2]) \
)
#define CMINMAXDIFF 20000

int GCreadbitmap (Gwidget_t *widget, Gbitmap_t *bitmap, FILE *fp) {
    return 0;
}

int GCwritebitmap (Gbitmap_t *bitmap, FILE *fp) {
    return 0;
}

int GCbitblt (
    Gwidget_t *widget, Gpoint_t gp, Grect_t gr, Gbitmap_t *bitmap,
    char *mode, Ggattr_t *ap
) {
    return 0;
}

int GCgetmousecoords (Gwidget_t *widget, Gpoint_t *gpp, int *count) {
    return 0;
}
