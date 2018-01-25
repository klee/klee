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

#ifndef _G_H
#define _G_H
#ifdef FEATURE_X11
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>
#include <X11/cursorfont.h>
#include <X11/Xaw/Viewport.h>
#include <X11/Xaw/Text.h>
#include <X11/Xaw/AsciiText.h>
#include <X11/Xaw/Command.h>
#include <X11/Xaw/Dialog.h>
#include <X11/Xaw/SimpleMenu.h>
#include <X11/Xaw/SmeBSB.h>
#include <X11/Xmu/CurUtil.h>
#ifdef FEATURE_GMAP
#ifdef FEATURE_MESAGL
#include <GL/GLwDrawA.h>
#else
#include <X11/GLw/GLwDrawA.h>
#endif
#include <Performer/pf.h>
#endif
#endif

/* general coordinate structures */

typedef struct Gxy_t {
    double x, y;
} Gxy_t;
typedef Gxy_t Gpoint_t;
typedef Gxy_t Gsize_t;
typedef struct Grect_t {
    Gxy_t o, c;
} Grect_t;

/* textline structure */
typedef struct Gtextline_t {
    char *p;
    int n, j;
    int w, h;
} Gtextline_t;

/* Color structure */

typedef struct Gcolor_t {
    int index;
    int r, g, b;
} Gcolor_t;
#define G_MAXCOLORS 256

/* event structures */

/* event consumption modes */
#define G_ONEEVENT   0
#define G_MANYEVENTS 1

/* event types and main structure */
#define G_MOUSE 0
#define G_KEYBD 1

#define G_DOWN 0
#define G_UP   1
#define G_MOVE 2
#define G_LEFT    0
#define G_MIDDLE  1
#define G_RIGHT   2
#define G_BUTTON3 3
#define G_BUTTON4 4
typedef struct Gevent_t {
    int type;
    int wi;
    int code;
    int data;
    Gpoint_t p;
} Gevent_t;

/* Widgets */

/* minimum widget sizes */
#define MINVWSIZE 100
#define MINTWSIZE 40
#define MINBWSIZE 40
#define MINLWSIZE 25
#define MINAWSIZE 25
#define MINSWSIZE 40
#define MINCWSIZE 100
#define MINPWSIZE 100

/* gfx attributes for the [p]canvas widget */

/* drawing styles */
#define G_SOLID       0
#define G_DASHED      1
#define G_DOTTED      2
#define G_LONGDASHED  3
#define G_SHORTDASHED 4

/* drawing modes */
#ifdef FEATURE_X11
#define G_SRC GXcopy
#define G_XOR GXxor
#else
#ifdef FEATURE_WIN32
#define G_SRC R2_COPYPEN
#define G_XOR R2_NOT
#else
#define G_SRC 0
#define G_XOR 1
#endif
#endif

/* gfx attributes and the attribute structure */
#define G_GATTRCOLOR  1
#define G_GATTRWIDTH  2
#define G_GATTRMODE   4
#define G_GATTRFILL   8
#define G_GATTRSTYLE 16

typedef struct Ggattr_t {
    int flags;
    int color;
    int width;
    int mode;
    int fill;
    int style;
} Ggattr_t;

/* widget attributes structures */

#define G_ATTRTYPEINT     0
#define G_ATTRTYPELONG    1
#define G_ATTRTYPEFLOAT   2
#define G_ATTRTYPEDOUBLE  3
#define G_ATTRTYPETEXT    4
#define G_ATTRTYPEPOINT   5
#define G_ATTRTYPESIZE    6
#define G_ATTRTYPERECT    7
#define G_ATTRTYPECOLOR   8
#define G_ATTRTYPEFUNC    9
#define G_ATTRTYPEULONG  10

typedef struct Gwattrmap_t {
    int id;
    int type;
    char *name;
} Gwattrmap_t;
typedef struct Gwattr_t {
    int id;
    union {
        int i;
        long l;
        float f;
        double d;
        char *t;
        Gpoint_t p;
        Gsize_t s;
        Grect_t r;
        Gcolor_t c;
        void *func;
        unsigned long u;
    } u;
} Gwattr_t;
typedef struct Gwlist_t {
    int wid;
    char *wname;
    int *attrid;
} Gwlist_t;

/* attribute ids */
#define G_ATTRORIGIN       0
#define G_ATTRSIZE         1
#define G_ATTRBORDERWIDTH  2
#define G_ATTRNAME         3
#define G_ATTRTEXT         4
#define G_ATTRAPPENDTEXT   5
#define G_ATTRSELECTION    6
#define G_ATTRCURSOR       7
#define G_ATTRMODE         8
#define G_ATTRLAYOUT       9
#define G_ATTRZORDER      10
#define G_ATTRCOLOR       11
#define G_ATTRVIEWPORT    12
#define G_ATTRWINDOW      13
#define G_ATTRWINDOWID    14
#define G_ATTRCHILDCENTER 15
#define G_ATTRNEWLINECB   16
#define G_ATTRRESIZECB    17
#define G_ATTRBUTTONCB    18
#define G_ATTREVENTCB     19
#define G_ATTRUSERDATA    20

/* array widget structs */
typedef struct Gawcarray_t {
#ifdef FEATURE_X11
    Widget w;
#else
#ifdef FEATURE_WIN32
    HWND w;
#else
    int w;
#endif
#endif
    int flag;
    int ox, oy, sx, sy, bs;
} Gawcarray_t;
#define AWCARRAYINCR 10
#define AWCARRAYSIZE sizeof (Gawcarray_t)

#define G_AWHARRAY 1
#define G_AWVARRAY 2
typedef struct Gawdata_t {
    int type;
    int sx, sy;
    Gawcarray_t *carray;
    int cj, cn;
#ifdef FEATURE_WIN32
    int batchmode, working;
#endif
} Gawdata_t;

/* query widget macros */
#define G_QWSTRING 1 /* string query */
#define G_QWFILE   2 /* file query */
#define G_QWCHOICE 3 /* choose 1 query */
#define G_QWMODES 3 /* total number of modes */

/* widget callbacks */
typedef void (*Gtwnewlinecb) (int, char *);
typedef void (*Gbuttoncb) (int, void *);
typedef void (*Glabelcb) (Gevent_t *);
typedef void (*Gcanvascb) (Gevent_t *);
typedef void (*Gawordercb) (void *, Gawdata_t *);
typedef void (*Gawcoordscb) (int, Gawdata_t *);
typedef void (*Gviewcb) (Gevent_t *);

#define G_ARRAYWIDGET    0
#define G_BUTTONWIDGET   1
#define G_CANVASWIDGET   2
#define G_LABELWIDGET    3
#define G_MENUWIDGET     4
#define G_PCANVASWIDGET  5
#define G_QUERYWIDGET    6
#define G_SCROLLWIDGET   7
#define G_TEXTWIDGET     8
#define G_VIEWWIDGET     9
#define G_WTYPESIZE     10

/* predefined widgets */

/* --- array --- */
typedef struct Gaw_t {
    Gawcoordscb func;
    int mode;
#ifdef FEATURE_WIN32
    Gawdata_t data;
#endif
} Gaw_t;
#define AWSIZE sizeof (Gaw_t)

/* --- button --- */
typedef struct Gbw_t {
    Gbuttoncb func;
} Gbw_t;
#define BWSIZE sizeof (Gbw_t)

/* --- canvas --- */
typedef struct Gcw_t {
    int needredraw;
    int buttonsdown;
    char bstate[5];
    struct Gcwcolor_t {
        int inuse;
#ifdef FEATURE_X11
        XColor color;
#else
#ifdef FEATURE_WIN32
        PALETTEENTRY color;
#endif
#endif
    } colors[G_MAXCOLORS];
    char allocedcolor[2];
    Ggattr_t gattr, defgattr;
    Grect_t wrect;
    Gsize_t vsize;
    Grect_t clip;
    Gcanvascb func;
#ifdef FEATURE_X11
    Window window;
    Colormap cmap;
    GC gc;
    Pixmap grays[17];
    XFontStruct *font;
#else
#ifdef FEATURE_WIN32
    HPALETTE cmap;
    HDC gc;
    HBRUSH grays[17];
    HFONT font;
    int ncolor;
#endif
#endif
#ifdef FEATURE_GMAP
    int gmapmode;
#endif
} Gcw_t;
#define CWSIZE sizeof (Gcw_t)

/* --- label --- */
typedef struct Glw_t {
    Glabelcb func;
} Glw_t;
#define LWSIZE sizeof (Glw_t)

/* --- menu --- */
typedef struct Gmw_t {
    long count;
} Gmw_t;
#define MWSIZE sizeof (Gmw_t)

/* --- postscript --- */
typedef struct Gpw_t {
#ifdef FEATURE_X11
    FILE *fp;
    struct Gpwcolor_t {
        int inuse;
        int r, g, b;
        double nr, ng, nb;
    } colors[G_MAXCOLORS];
    Ggattr_t gattr, defgattr;
    Grect_t wrect;
    Gsize_t vsize;
#else
#ifdef FEATURE_WIN32
    struct Gpwcolor_t {
        int inuse;
        PALETTEENTRY color;
    } colors[G_MAXCOLORS];
    Ggattr_t gattr, defgattr;
    Grect_t wrect;
    Gsize_t vsize;
    HPALETTE cmap;
    HDC gc;
    HBRUSH grays[17];
    HFONT font;
    int ncolor, mode;
#else
    int dummy;
#endif
#endif
} Gpw_t;
#define PWSIZE sizeof (Gpw_t)

/* --- query --- */
typedef struct Gqw_t {
#ifdef FEATURE_X11
    Widget w;
#else
#ifdef FEATURE_WIN32
    HWND w;
#endif
#endif
    int mode;
    int state, button;
} Gqw_t;
#define QWSIZE sizeof (Gqw_t)

/* --- scroll --- */
typedef struct Gsw_t {
    int dummy;
} Gsw_t;
#define SWSIZE sizeof (Gsw_t)

/* --- text --- */
typedef struct Gtw_t {
    Gtwnewlinecb func;
} Gtw_t;
#define TWSIZE sizeof (Gtw_t)

/* --- view --- */
typedef struct Gvw_t {
    Gviewcb func;
    int closing;
} Gvw_t;
#define VWSIZE sizeof (Gvw_t)

/* the main widget structure */
typedef struct Gwidget_t {
    int type;
    int inuse;
    int pwi;
#ifdef FEATURE_X11
    Widget w;
#else
#ifdef FEATURE_WIN32
    HWND w;
#else
    int w;
#endif
#endif
    union {
        Gaw_t *a;
        Gbw_t *b;
        Gcw_t *c;
        Glw_t *l;
        Gmw_t *m;
        Gpw_t *p;
        Gqw_t *q;
        Gsw_t *s;
        Gtw_t *t;
        Gvw_t *v;
    } u;
    unsigned long udata;
} Gwidget_t;
#define WIDGETINCR 20
#define WIDGETSIZE sizeof (Gwidget_t)

/* bitmap data structure */
typedef struct Gbitmap_t {
    int inuse;
    int canvas;
    int ctype; /* type of canvas, eg. G_CANVASWIDGET */
    Gsize_t size;
    Gsize_t scale;
    union {
        struct {
#ifdef FEATURE_X11
            Pixmap orig, scaled;
#else
#ifdef FEATURE_WIN32
            HBITMAP orig, scaled;
#else
            int dummy;
#endif
#endif
        } bmap;
        unsigned char *bits;
    } u;
} Gbitmap_t;

extern Gbitmap_t *Gbitmaps;
extern int Gbitmapn;

/* global array of widgets */
extern Gwidget_t *Gwidgets;
extern int Gwidgetn;

extern Gwlist_t Gwlist[];
extern Gwattrmap_t Gwattrmap[];

extern char *Gdefaultfont;
extern int Gneedredraw;
extern int Gbuttonsdown;
extern int Gerrflag;
extern char *Gpscanvasname;

extern int Gxfd;

#ifdef FEATURE_WIN32
extern int Gnocallbacks;
#endif

/* functions returning an int
    return -1 if there's an error and
    also set the Gerrno variable

    the rendering functions may return +1
    if the graphical object is completely hidden
*/
int Ginit (void);
int Gterm (void);
int Gcreatewidget (int, int, int, Gwattr_t *);
int Gsetwidgetattr (int, int, Gwattr_t *);
int Ggetwidgetattr (int, int, Gwattr_t *);
int Gdestroywidget (int);
int Gqueryask (int, char *, char *, char *, int);
int Gmenuaddentries (int, int, char **);
int Gmenudisplay (int, int);
int Gsync (void);
int Gresetbstate (int);
int Gcanvasclear (int);
int Gsetgfxattr (int, Ggattr_t *);
int Ggetgfxattr (int, Ggattr_t *);
int Garrow (int, Gpoint_t, Gpoint_t, Ggattr_t *);
int Gline (int, Gpoint_t, Gpoint_t, Ggattr_t *);
int Gbox (int, Grect_t, Ggattr_t *);
int Gpolygon (int, int, Gpoint_t *, Ggattr_t *);
int Gsplinegon (int, int, Gpoint_t *, Ggattr_t *);
int Garc (int, Gpoint_t, Gsize_t, double, double, Ggattr_t *);
int Gtext (int, char *, Gpoint_t, char *, double, char *, Ggattr_t *);
int Ggettextsize (int, char *, char *, double, Gsize_t *);
int Gcreatebitmap (int, Gsize_t);
int Gdestroybitmap (int);
int Greadbitmap (int, FILE *);
int Gwritebitmap (FILE *, int);
int Gbitblt (int, Gpoint_t, Grect_t, int, char *, Ggattr_t *);
int Ggetmousecoords (int, Gpoint_t *, int *);

int Gprocessevents (int, int);

int Gaworder (Gwidget_t *, void *, Gawordercb);
int Gawsetmode (Gwidget_t *, int);
int Gawgetmode (Gwidget_t *);
void Gawdefcoordscb (int, Gawdata_t *);

Gwidget_t *newwidget (int);
Gwidget_t *findwidget (unsigned long, int);
Gbitmap_t *newbitmap (void);
void Gerr (char *, int, int, ...);

/* error messages */
#define G_ERRBADATTRID           1
#define G_ERRBADATTRVALUE        2
#define G_ERRBADCOLORINDEX       3
#define G_ERRBADPARENTWIDGETID   4
#define G_ERRBADWIDGETID         5
#define G_ERRBADWIDGETTYPE       6
#define G_ERRCANNOTCREATEWIDGET  7
#define G_ERRCANNOTGETATTR       8
#define G_ERRCANNOTOPENFILE      9
#define G_ERRCANNOTSETATTR1     10
#define G_ERRCANNOTSETATTR2     11
#define G_ERRINITFAILED         12
#define G_ERRNOCHILDWIDGET      13
#define G_ERRNOPARENTWIDGET     14
#define G_ERRNOSUCHCURSOR       15
#define G_ERRNOTACANVAS         16
#define G_ERRNOTIMPLEMENTED     17
#define G_ERRNOTSUPPORTED       18
#define G_ERRBADBITMAPID        19
#define G_ERRCANNOTCREATEBITMAP 20
#define G_ERRNOBITMAP           21
#define G_ERRCANNOTREADBITMAP   22

extern int Gerrno;
#endif /* _G_H */

#ifdef __cplusplus
}
#endif

