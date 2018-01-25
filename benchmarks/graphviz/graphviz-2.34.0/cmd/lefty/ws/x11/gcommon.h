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
#if XlibSpecificationRelease < 5
typedef char *XPointer;
#endif

/* point and rect structures */
typedef XPoint PIXxy_t;
typedef PIXxy_t PIXpoint_t;
typedef PIXxy_t PIXsize_t;
typedef struct PIXrect_t {
    PIXxy_t o, c;
} PIXrect_t;

extern Widget Groot;
extern Display *Gdisplay;
extern int Gpopdownflag;
extern int Gscreenn;
extern int Gdepth;

extern Arg argp[];
extern int argn;
#define MAXARGS 50
#define RESETARGS (argn = 0)
#define ADD2ARGS(var, val) ( \
    XtSetArg (argp[argn], (var), (val)), argn++ \
)

/*  structures used to minimize number of calls to
    XtManage and XtRealize functions (which are expensive)
*/
#define LAZYREALIZE 1
#define LAZYRHINTS  2
#define LAZYMANAGE  4

#define LAZYQNUM 100
typedef struct Glazyq_t {
    int flag;
    Widget rw;
    XSizeHints hints;
    Widget mws[LAZYQNUM];
    int mwn;
} Glazyq_t;
extern Glazyq_t Glazyq;

typedef struct Gfont_t {
    char *name;
    XFontStruct *font;
} Gfont_t;
extern Gfont_t *Gfontp;
extern int Gfontn;
#define FONTSIZE sizeof (Gfont_t)
#define SETFONT(font) { \
    XGCValues gcv; \
    if (font != WCU->font) { \
        WCU->font = font, gcv.font = font->fid; \
        XChangeGC (Gdisplay, GC, GCFont, &gcv); \
    } \
}

extern char *Gbufp;
extern int Gbufn, Gbufi;
#define BUFINCR 1024
#define BUFSIZE sizeof (char)

extern PIXpoint_t *Gppp;
extern int Gppn, Gppi;
#define PPINCR 100
#define PPSIZE sizeof (PIXpoint_t)

#define GETSIZE(sin, sout, smin) ( \
    sout.x = (sin.x > smin) ? sin.x + 0.5 : smin, \
    sout.y = (sin.y > smin) ? sin.y + 0.5 : smin \
)
#define GETORIGIN(oin, oout) ( \
    oout.x = oin.x + 0.5, oout.y = oin.y + 0.5 \
)

int Ginitgraphics (void);
int Gtermgraphics (void);
void Gflushlazyq (void);
void Glazyrealize (Widget, int, XSizeHints *);
void Glazymanage (Widget);
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

void Gcwbutaction (Widget, XEvent *, char **, unsigned int *);
void Gcwkeyaction (Widget, XEvent *, char **, unsigned int *);
void Glwbutaction (Widget, XEvent *, char **, unsigned int *);
void Glwkeyaction (Widget, XEvent *, char **, unsigned int *);
void Gtweolaction (Widget, XEvent *, char **, unsigned int *);
void Gqwpopaction (Widget, XEvent *, char **, unsigned int *);
void Gqwdelaction (Widget, XEvent *, char **, unsigned int *);
void Gwmdelaction (Widget, XEvent *, char **, unsigned int *);
extern XtTranslations Gtweoltable;
extern XtTranslations Gqwpoptable;
extern XtTranslations Glwanytable;
extern XtTranslations Gcwanytable;
extern XtTranslations Gqwdeltable;
extern XtTranslations Gwmdeltable;

extern Atom Gqwdelatom, Gwmdelatom;

#endif /* _GCOMMON_H */

#ifdef __cplusplus
}
#endif

