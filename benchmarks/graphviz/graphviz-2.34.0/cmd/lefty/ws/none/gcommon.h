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

/* Lefteris Koutsofios - AT&T Labs Research */

#ifndef _GCOMMON_H
#define _GCOMMON_H

typedef struct {
    int x, y;
} XPoint;
typedef int Widget;
typedef int Display;

extern Widget Groot;
extern Display *Gdisplay;
extern int Gpopdownflag;
extern int Gscreenn;
extern int Gdepth;

int Ginitgraphics (void);
int Gtermgraphics (void);
int Gsync (void);

int GAcreatewidget (Gwidget_t *, Gwidget_t *, int, Gwattr_t *);
int GAsetwidgetattr (Gwidget_t *, int, Gwattr_t *);
int GAgetwidgetattr (Gwidget_t *, int, Gwattr_t *);
int GAdestroywidget (Gwidget_t *);

int GBcreatewidget (Gwidget_t *, Gwidget_t *, int, Gwattr_t *);
int GBsetwidgetattr (Gwidget_t *, int, Gwattr_t *);
int GBgetwidgetattr (Gwidget_t *, int, Gwattr_t *);
int GBdestroywidget (Gwidget_t *);

int GCcreatewidget (Gwidget_t *, Gwidget_t *, int, Gwattr_t *);
int GCsetwidgetattr (Gwidget_t *, int, Gwattr_t *);
int GCgetwidgetattr (Gwidget_t *, int, Gwattr_t *);
int GCdestroywidget (Gwidget_t *);
int GCcanvasclear (Gwidget_t *);
int GCsetgfxattr (Gwidget_t *, Ggattr_t *);
int GCgetgfxattr (Gwidget_t *, Ggattr_t *);
int GCarrow (Gwidget_t *, Gpoint_t, Gpoint_t, Ggattr_t *);
int GCline (Gwidget_t *, Gpoint_t, Gpoint_t, Ggattr_t *);
int GCbox (Gwidget_t *, Grect_t, Ggattr_t *);
int GCpolygon (Gwidget_t *, int, Gpoint_t *, Ggattr_t *);
int GCsplinegon (Gwidget_t *, int, Gpoint_t *, Ggattr_t *);
int GCarc (Gwidget_t *, Gpoint_t, Gsize_t, double, double, Ggattr_t *);
int GCtext (
    Gwidget_t *, Gtextline_t *, int, Gpoint_t,
    char *, double, char *, Ggattr_t *
);
int GCgettextsize (Gwidget_t *, Gtextline_t *, int, char *, double, Gsize_t *);
int GCcreatebitmap (Gwidget_t *, Gbitmap_t *, Gsize_t);
int GCdestroybitmap (Gbitmap_t *);
int GCreadbitmap (Gwidget_t *, Gbitmap_t *, FILE *);
int GCwritebitmap (Gbitmap_t *, FILE *);
int GCbitblt (Gwidget_t *, Gpoint_t, Grect_t, Gbitmap_t *, char *, Ggattr_t *);
int GCgetmousecoords (Gwidget_t *, Gpoint_t *, int *);

int GLcreatewidget (Gwidget_t *, Gwidget_t *, int, Gwattr_t *);
int GLsetwidgetattr (Gwidget_t *, int, Gwattr_t *);
int GLgetwidgetattr (Gwidget_t *, int, Gwattr_t *);
int GLdestroywidget (Gwidget_t *);

int GMcreatewidget (Gwidget_t *, Gwidget_t *, int, Gwattr_t *);
int GMsetwidgetattr (Gwidget_t *, int, Gwattr_t *);
int GMgetwidgetattr (Gwidget_t *, int, Gwattr_t *);
int GMdestroywidget (Gwidget_t *);
int GMmenuaddentries (Gwidget_t *, int, char **);
int GMmenudisplay (Gwidget_t *, Gwidget_t *);

int GPcreatewidget (Gwidget_t *, Gwidget_t *, int, Gwattr_t *);
int GPsetwidgetattr (Gwidget_t *, int, Gwattr_t *);
int GPgetwidgetattr (Gwidget_t *, int, Gwattr_t *);
int GPdestroywidget (Gwidget_t *);
int GPcanvasclear (Gwidget_t *);
int GPsetgfxattr (Gwidget_t *, Ggattr_t *);
int GPgetgfxattr (Gwidget_t *, Ggattr_t *);
int GParrow (Gwidget_t *, Gpoint_t, Gpoint_t, Ggattr_t *);
int GPline (Gwidget_t *, Gpoint_t, Gpoint_t, Ggattr_t *);
int GPbox (Gwidget_t *, Grect_t, Ggattr_t *);
int GPpolygon (Gwidget_t *, int, Gpoint_t *, Ggattr_t *);
int GPsplinegon (Gwidget_t *, int, Gpoint_t *, Ggattr_t *);
int GParc (Gwidget_t *, Gpoint_t, Gsize_t, double, double, Ggattr_t *);
int GPtext (
    Gwidget_t *, Gtextline_t *, int, Gpoint_t,
    char *, double, char *, Ggattr_t *
);
int GPcreatebitmap (Gwidget_t *, Gbitmap_t *, Gsize_t);
int GPdestroybitmap (Gbitmap_t *);
int GPreadbitmap (Gwidget_t *, Gbitmap_t *, FILE *);
int GPwritebitmap (Gbitmap_t *, FILE *);
int GPbitblt (Gwidget_t *, Gpoint_t, Grect_t, Gbitmap_t *, char *, Ggattr_t *);

int GQcreatewidget (Gwidget_t *, Gwidget_t *, int, Gwattr_t *);
int GQsetwidgetattr (Gwidget_t *, int, Gwattr_t *);
int GQgetwidgetattr (Gwidget_t *, int, Gwattr_t *);
int GQdestroywidget (Gwidget_t *);
int GQqueryask (Gwidget_t *, char *, char *, char *, int);

int GScreatewidget (Gwidget_t *, Gwidget_t *, int, Gwattr_t *);
int GSsetwidgetattr (Gwidget_t *, int, Gwattr_t *);
int GSgetwidgetattr (Gwidget_t *, int, Gwattr_t *);
int GSdestroywidget (Gwidget_t *);

int GTcreatewidget (Gwidget_t *, Gwidget_t *, int, Gwattr_t *);
int GTsetwidgetattr (Gwidget_t *, int, Gwattr_t *);
int GTgetwidgetattr (Gwidget_t *, int, Gwattr_t *);
int GTdestroywidget (Gwidget_t *);

int GVcreatewidget (Gwidget_t *, Gwidget_t *, int, Gwattr_t *);
int GVsetwidgetattr (Gwidget_t *, int, Gwattr_t *);
int GVgetwidgetattr (Gwidget_t *, int, Gwattr_t *);
int GVdestroywidget (Gwidget_t *);

void Gawdefcoordscb (int, Gawdata_t *);

#endif /* _GCOMMON_H */

#ifdef __cplusplus
}
#endif

