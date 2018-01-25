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

int GQcreatewidget (
    Gwidget_t *parent, Gwidget_t *widget, int attrn, Gwattr_t *attrp
) {
    return -1;
}

int GQsetwidgetattr (Gwidget_t *widget, int attrn, Gwattr_t *attrp) {
    return 0;
}

int GQgetwidgetattr (Gwidget_t *widget, int attrn, Gwattr_t *attrp) {
    return 0;
}

int GQdestroywidget (Gwidget_t *widget) {
    return 0;
}

int GQqueryask (
    Gwidget_t *widget, char *prompt, char *args,
    char *responsep, int responsen
) {
    return 0;
}
