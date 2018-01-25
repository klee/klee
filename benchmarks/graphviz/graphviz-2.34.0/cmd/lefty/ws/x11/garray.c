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

#define WAU widget->u.a

extern WidgetClass arrayWidgetClass;

static void awcallback (Widget, XtPointer, XtPointer);

int GAcreatewidget (
    Gwidget_t *parent, Gwidget_t *widget, int attrn, Gwattr_t *attrp
) {
    PIXsize_t ps;
    int ai;
    XColor c;
    int color;

    if (!parent) {
        Gerr (POS, G_ERRNOPARENTWIDGET);
        return -1;
    }
    WAU->func = NULL;
    ps.x = ps.y = MINAWSIZE;
    RESETARGS;
    for (ai = 0; ai < attrn; ai++) {
        switch (attrp[ai].id) {
        case G_ATTRSIZE:
            GETSIZE (attrp[ai].u.s, ps, MINAWSIZE);
            break;
        case G_ATTRBORDERWIDTH:
            ADD2ARGS (XtNborderWidth, attrp[ai].u.i);
            break;
        case G_ATTRMODE:
            if (strcmp ("horizontal", attrp[ai].u.t) == 0) {
                ADD2ARGS (XtNorientation, XtorientHorizontal);
                WAU->mode = G_AWHARRAY;
            } else if (strcmp ("vertical", attrp[ai].u.t) == 0) {
                WAU->mode = G_AWVARRAY;
            } else {
                Gerr (POS, G_ERRBADATTRVALUE, attrp[ai].u.t);
                return -1;
            }
            break;
        case G_ATTRLAYOUT:
            if (strcmp ("on", attrp[ai].u.t) == 0)
                Gawsetmode (widget, FALSE);
            else if (strcmp ("off", attrp[ai].u.t) == 0)
                Gawsetmode (widget, TRUE);
            else {
                Gerr (POS, G_ERRBADATTRVALUE, attrp[ai].u.t);
                return -1;
            }
            break;
        case G_ATTRCOLOR:
            color = attrp[ai].u.c.index;
            if (color != 0 && color != 1) {
                Gerr (POS, G_ERRBADCOLORINDEX, color);
                return -1;
            }
            c.red = attrp[ai].u.c.r * 257;
            c.green = attrp[ai].u.c.g * 257;
            c.blue = attrp[ai].u.c.b * 257;
            if (XAllocColor (
                Gdisplay, DefaultColormap (Gdisplay, Gscreenn), &c
            )) {
                if (color == 0)
                    ADD2ARGS (XtNbackground, c.pixel);
                else
                    ADD2ARGS (XtNforeground, c.pixel);
	    }
            break;
        case G_ATTRWINDOWID:
            Gerr (POS, G_ERRCANNOTSETATTR1, "windowid");
            return -1;
        case G_ATTRRESIZECB:
            WAU->func = (Gawcoordscb) attrp[ai].u.func;
            break;
        case G_ATTRUSERDATA:
            widget->udata = attrp[ai].u.u;
            break;
        default:
            Gerr (POS, G_ERRBADATTRID, attrp[ai].id);
            return -1;
        }
    }
    ADD2ARGS (XtNwidth, ps.x);
    ADD2ARGS (XtNheight, ps.y);
    if (!(widget->w = XtCreateWidget (
        "array", arrayWidgetClass, parent->w, argp, argn
    ))) {
        Gerr (POS, G_ERRCANNOTCREATEWIDGET);
        return -1;
    }
    XtAddCallback (widget->w, XtNcallback, awcallback, (XtPointer) NULL);
    Glazymanage (widget->w);
    return 0;
}

int GAsetwidgetattr (Gwidget_t *widget, int attrn, Gwattr_t *attrp) {
    PIXsize_t ps;
    int ai;

    RESETARGS;
    for (ai = 0; ai < attrn; ai++) {
        switch (attrp[ai].id) {
        case G_ATTRSIZE:
            GETSIZE (attrp[ai].u.s, ps, MINAWSIZE);
            ADD2ARGS (XtNwidth, ps.x);
            ADD2ARGS (XtNheight, ps.y);
            break;
        case G_ATTRBORDERWIDTH:
            ADD2ARGS (XtNborderWidth, attrp[ai].u.i);
            break;
        case G_ATTRMODE:
            Gerr (POS, G_ERRCANNOTSETATTR2, "mode");
            return -1;
        case G_ATTRLAYOUT:
            if (strcmp ("on", attrp[ai].u.t) == 0)
                Gawsetmode (widget, FALSE);
            else if (strcmp ("off", attrp[ai].u.t) == 0)
                Gawsetmode (widget, TRUE);
            else {
                Gerr (POS, G_ERRBADATTRVALUE, attrp[ai].u.t);
                return -1;
            }
            break;
        case G_ATTRWINDOWID:
            Gerr (POS, G_ERRCANNOTSETATTR2, "windowid");
            return -1;
        case G_ATTRRESIZECB:
            WAU->func = (Gawcoordscb) attrp[ai].u.func;
            break;
        case G_ATTRUSERDATA:
            widget->udata = attrp[ai].u.u;
            break;
        default:
            Gerr (POS, G_ERRBADATTRID, attrp[ai].id);
            return -1;
        }
    }
    XtSetValues (widget->w, argp, argn);
    return 0;
}

int GAgetwidgetattr (Gwidget_t *widget, int attrn, Gwattr_t *attrp) {
    Dimension width, height;
    int ai;

    for (ai = 0; ai < attrn; ai++) {
        RESETARGS;
        switch (attrp[ai].id) {
        case G_ATTRSIZE:
            ADD2ARGS (XtNwidth, &width);
            ADD2ARGS (XtNheight, &height);
            XtGetValues (widget->w, argp, argn);
            attrp[ai].u.s.x = width, attrp[ai].u.s.y = height;
            break;
        case G_ATTRBORDERWIDTH:
            ADD2ARGS (XtNborderWidth, &width);
            XtGetValues (widget->w, argp, argn);
            attrp[ai].u.i = width;
            break;
        case G_ATTRMODE:
            attrp[ai].u.t = (
                WAU->mode == G_AWHARRAY
            ) ? "horizontal" : "vertical";
            break;
        case G_ATTRLAYOUT:
            attrp[ai].u.t = (Gawgetmode (widget)) ? "off" : "on";
            break;
        case G_ATTRWINDOWID:
            sprintf (&Gbufp[0], "0x%lx", XtWindow (widget->w));
            attrp[ai].u.t = &Gbufp[0];
            break;
        case G_ATTRRESIZECB:
            attrp[ai].u.func = WAU->func;
            break;
        case G_ATTRUSERDATA:
            attrp[ai].u.u = widget->udata;
            break;
        default:
            Gerr (POS, G_ERRBADATTRID, attrp[ai].id);
            return -1;
        }
    }
    return 0;
}

int GAdestroywidget (Gwidget_t *widget) {
    XtDestroyWidget (widget->w);
    return 0;
}

static void awcallback (Widget w, XtPointer clientdata, XtPointer calldata) {
    Gwidget_t *widget;

    if (!(widget = findwidget ((unsigned long) w, G_ARRAYWIDGET)))
        return;
    if (WAU->func)
        (*WAU->func) (widget - &Gwidgets[0], (Gawdata_t *) calldata);
    else
        Gawdefcoordscb (widget - &Gwidgets[0], (Gawdata_t *) calldata);
}

/* the rest of this file contains the implementation of the array widget */

#include <X11/IntrinsicP.h>

typedef struct _ArrayClassRec *ArrayWidgetClass;
typedef struct _ArrayRec *ArrayWidget;

typedef struct _ArrayClassPart {
    int dummy; /* not used */
} ArrayClassPart;

typedef struct _ArrayClassRec {
    CoreClassPart core_class;
    CompositeClassPart composite_class;
    ArrayClassPart array_class;
} ArrayClassRec;

typedef struct _ArrayPart {
    XtCallbackList callbacks;
    XtOrientation orientation;
    Gawdata_t data;
    int batchmode;
} ArrayPart;

typedef struct _ArrayRec {
    CorePart core;
    CompositePart composite;
    ArrayPart array;
} ArrayRec;

#define CHILDINCR 10
#define CHILDSIZE sizeof (Gawcarray_t)

static XtResource resources[] = {
    {
        XtNcallback,
        XtCCallback,
        XtRCallback,
        sizeof (XtPointer),
        XtOffsetOf (ArrayRec, array.callbacks),
        XtRCallback, (XtPointer) NULL
    },
    {
        XtNorientation,
        XtCOrientation,
        XtROrientation,
        sizeof (XtOrientation),
        XtOffsetOf (ArrayRec, array.orientation),
        XtRImmediate, (XtPointer) XtorientVertical
    }
};

static void ClassInitialize (void);
static void Initialize (Widget, Widget, ArgList, Cardinal *);
static void Destroy (Widget);
static void Resize (Widget);
static Boolean SetValues (Widget, Widget, Widget, ArgList, Cardinal *);
static XtGeometryResult GeometryManager (
    Widget, XtWidgetGeometry *, XtWidgetGeometry *
);
static void ChangeManaged (Widget);
static void InsertChild (Widget);
static void DeleteChild (Widget);
static void dolayout (ArrayWidget, int);

ArrayClassRec arrayClassRec = {
    { /* core_class fields */
        /* superclass          */ (WidgetClass) &compositeClassRec,
        /* class_name          */ "Array",
        /* widget_size         */ sizeof(ArrayRec),
        /* class_initialize    */ ClassInitialize,
        /* class_part_init     */ NULL,
        /* class_inited        */ FALSE,
        /* initialize          */ Initialize,
        /* initialize_hook     */ NULL,
        /* realize             */ XtInheritRealize,
        /* actions             */ NULL,
        /* num_actions         */ 0,
        /* resources           */ resources,
        /* num_resources       */ XtNumber (resources),
        /* xrm_class           */ NULLQUARK,
        /* compress_motion     */ TRUE,
        /* compress_exposure   */ TRUE,
        /* compress_enterleave */ TRUE,
        /* visible_interest    */ FALSE,
        /* destroy             */ Destroy,
        /* resize              */ Resize,
        /* expose              */ XtInheritExpose,
        /* set_values          */ SetValues,
        /* set_values_hook     */ NULL,
        /* set_values_almost   */ XtInheritSetValuesAlmost,
        /* get_values_hook     */ NULL,
        /* accept_focus        */ NULL,
        /* version             */ XtVersion,
        /* callback_private    */ NULL,
        /* tm_table            */ NULL,
        /* query_geometry      */ XtInheritQueryGeometry,
        /* display_accelerator */ XtInheritDisplayAccelerator,
        /* extension           */ NULL
    },
    { /* composite_class fields */
        /* geometry_manager */ GeometryManager,
        /* change_managed   */ ChangeManaged,
        /* insert_child     */ InsertChild,
        /* delete_child     */ DeleteChild,
        /* extension        */ NULL
    },
    { /* array_class fields */
        /* dummy */ 0
    }
};

WidgetClass arrayWidgetClass = (WidgetClass)&arrayClassRec;

int Gaworder (Gwidget_t *widget, void *data, Gawordercb func) {
    ArrayWidget aw;

    aw = (ArrayWidget) widget->w;
    (*func) (data, &aw->array.data);
    dolayout (aw, TRUE);
    return 0;
}

int Gawsetmode (Gwidget_t *widget, int mode) {
    ArrayWidget aw;

    aw = (ArrayWidget) widget->w;
    aw->array.batchmode = mode;
    dolayout (aw, TRUE);
    return 0;
}

int Gawgetmode (Gwidget_t *widget) {
    ArrayWidget aw;

    aw = (ArrayWidget) widget->w;
    return aw->array.batchmode;
}

void Gawdefcoordscb (int wi, Gawdata_t *dp) {
    Gawcarray_t *cp;
    int sx, sy, csx, csy, ci;

    sx = dp->sx, sy = dp->sy;
    csx = csy = 0;
    for (ci = 0; ci < dp->cj; ci++) {
        cp = &dp->carray[ci];
        if (!cp->flag)
            continue;
        cp->ox = csx, cp->oy = csy;
        if (dp->type == G_AWVARRAY)
            cp->sx = sx - 2 * cp->bs, csy += cp->sy + 2 * cp->bs;
        else
            cp->sy = sy - 2 * cp->bs, csx += cp->sx + 2 * cp->bs;
    }
    if (dp->type == G_AWVARRAY)
        dp->sy = csy;
    else
        dp->sx = csx;
}

static void ClassInitialize (void) {
    XtAddConverter (
        XtRString, XtROrientation, XmuCvtStringToOrientation, NULL, 0
    );
}

static void Initialize (
    Widget reqw, Widget neww, ArgList args, Cardinal *num_args
) {
    ArrayWidget aw;

    aw = (ArrayWidget) neww;
    if (aw->array.orientation == XtorientVertical)
        aw->array.data.type = G_AWVARRAY;
    else
        aw->array.data.type = G_AWHARRAY;
    aw->array.data.carray = Marrayalloc ((long) CHILDINCR * CHILDSIZE);
    aw->array.data.cn = CHILDINCR;
    aw->array.data.cj = 0;
    aw->array.batchmode = FALSE;
    if (aw->core.width == 0)
        aw->core.width = 100;
    if (aw->core.height == 0)
        aw->core.height = 100;
}

static void Destroy (Widget w) {
    ArrayWidget aw;

    aw = (ArrayWidget) w;
    Marrayfree (aw->array.data.carray);
    aw->array.data.cn = aw->array.data.cj = 0;
}

static void Resize (Widget w) {
    dolayout ((ArrayWidget) w, FALSE);
}

static Boolean SetValues (
    Widget curw, Widget reqw, Widget neww, ArgList args, Cardinal *num_args
) {
    ArrayWidget curaw;
    ArrayWidget newaw;

    curaw = (ArrayWidget) curw;
    newaw = (ArrayWidget) neww;
    if (curaw->array.orientation != newaw->array.orientation) {
        if (newaw->array.orientation == XtorientVertical)
            newaw->array.data.type = G_AWVARRAY;
        else
            newaw->array.data.type = G_AWHARRAY;
        dolayout (newaw, TRUE);
        return TRUE;
    }
    return FALSE;
}

static XtGeometryResult GeometryManager (
    Widget w, XtWidgetGeometry *req, XtWidgetGeometry *rep
) {
    Dimension width, height;

    if (req->request_mode & ~(CWX | CWY | CWWidth | CWHeight))
        return XtGeometryNo;

    if (req->request_mode & (CWX | CWY | CWWidth | CWHeight)) {
        width = (req->request_mode & CWWidth) ? req->width : w->core.width;
        height = (req->request_mode & CWHeight) ? req->height : w->core.height;
        w->core.width = width, w->core.height = height;
        dolayout ((ArrayWidget) XtParent (w), TRUE);
        return XtGeometryYes;
    }
    return XtGeometryYes;
}

static void ChangeManaged (Widget w) {
    ArrayWidget aw;

    aw = (ArrayWidget) w;
    if (!aw->array.batchmode)
        dolayout (aw, TRUE);
}

static void InsertChild (Widget w) {
    ArrayWidget aw;
    CompositeWidgetClass sclass;

    sclass = (CompositeWidgetClass) compositeWidgetClass;
    (*sclass->composite_class.insert_child) (w);
    aw = (ArrayWidget) XtParent (w);
    if (aw->array.data.cj == aw->array.data.cn) {
        aw->array.data.carray = Marraygrow (
            aw->array.data.carray,
            (long) (aw->array.data.cn + CHILDINCR) * CHILDSIZE
        );
        aw->array.data.cn += CHILDINCR;
    }
    aw->array.data.carray[aw->array.data.cj++].w = w;
}

static void DeleteChild (Widget w) {
    ArrayWidget aw;
    CompositeWidgetClass sclass;
    int ci;

    sclass = (CompositeWidgetClass) compositeWidgetClass;
    (*sclass->composite_class.delete_child) (w);
    aw = (ArrayWidget) XtParent (w);
    for (ci = 0; ci < aw->array.data.cj; ci++)
        if (aw->array.data.carray[ci].w == w)
            break;
    if (ci < aw->array.data.cj) {
        for (; ci + 1 < aw->array.data.cj; ci++)
            aw->array.data.carray[ci].w = aw->array.data.carray[ci + 1].w;
        aw->array.data.cj--;
    }
}

static void dolayout (ArrayWidget aw, int flag) {
    XtWidgetGeometry req, ret_req;
    Gawdata_t *dp;
    Gawcarray_t *cp;
    int sx, sy, ci;

    if (aw->array.batchmode)
        return;
    dp = &aw->array.data;
    for (ci = 0; ci < dp->cj; ci++) {
        if (!XtIsManaged (dp->carray[ci].w)) {
            dp->carray[ci].flag = 0;
            continue;
        }
        cp = &dp->carray[ci];
        cp->flag = 1;
        cp->ox = cp->w->core.x;
        cp->oy = cp->w->core.y;
        cp->sx = cp->w->core.width;
        cp->sy = cp->w->core.height;
        cp->bs = cp->w->core.border_width;
    }
    dp->sx = aw->core.width, dp->sy = aw->core.height;
    XtCallCallbackList ((Widget) aw, aw->array.callbacks, dp);
    if ((sx = dp->sx) < MINAWSIZE)
        sx = MINAWSIZE;
    if ((sy = dp->sy) < MINAWSIZE)
        sy = MINAWSIZE;
    if (flag && (aw->core.width != sx || aw->core.height != sy)) {
        req.width = sx, req.height = sy;
        req.request_mode = CWWidth | CWHeight;
        if (XtMakeGeometryRequest (
            (Widget) aw, &req, &ret_req
        ) == XtGeometryAlmost) {
            req = ret_req;
            XtMakeGeometryRequest ((Widget) aw, &req, &ret_req);
            dp->sx = req.width, dp->sy = req.height;
            XtCallCallbackList ((Widget) aw, aw->array.callbacks, dp);
        }
    }
    for (ci = 0; ci < dp->cj; ci++) {
        cp = &dp->carray[ci];
        if (!cp->flag)
            continue;
        XtConfigureWidget (cp->w, cp->ox, cp->oy, cp->sx, cp->sy, cp->bs);
    }
}
