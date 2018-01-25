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

#define WPU widget->u.p
#define WINDOW widget->u.p->window
#define GC widget->u.p->gc

static long gstyles[5] = {
    /* G_SOLID */       PS_SOLID,
    /* G_DASHED */      PS_DASH,
    /* G_DOTTED */      PS_DOT,
    /* G_LONGDASHED */  PS_DASH,
    /* G_SHORTDASHED */ PS_DASH,
};

static char grays[][4] = {
    { 0x00, 0x00, 0x00, 0x00 },
    { 0x08, 0x00, 0x00, 0x00 },
    { 0x08, 0x00, 0x02, 0x00 },
    { 0x0A, 0x00, 0x02, 0x00 },
    { 0x0A, 0x00, 0x0A, 0x00 },
    { 0x0A, 0x04, 0x0A, 0x00 },
    { 0x0A, 0x04, 0x0A, 0x01 },
    { 0x0A, 0x05, 0x0A, 0x01 },
    { 0x0A, 0x05, 0x0A, 0x05 },
    { 0x0E, 0x05, 0x0A, 0x05 },
    { 0x0E, 0x05, 0x0B, 0x05 },
    { 0x0F, 0x05, 0x0B, 0x05 },
    { 0x0F, 0x05, 0x0F, 0x05 },
    { 0x0F, 0x0D, 0x0F, 0x05 },
    { 0x0F, 0x0D, 0x0F, 0x07 },
    { 0x0F, 0x0F, 0x0F, 0x07 },
    { 0x0F, 0x0F, 0x0F, 0x0F }
};

char *Gpscanvasname = "out.emf";

static void bezier (PIXpoint_t, PIXpoint_t, PIXpoint_t, PIXpoint_t);
static HFONT findfont (char *, int);
static int scalebitmap (Gwidget_t *, Gbitmap_t *, Gsize_t, int, int);
static void setgattr (Gwidget_t *, Ggattr_t *);

static PIXrect_t  rdrawtopix (Gwidget_t *, Grect_t);
static PIXpoint_t pdrawtopix (Gwidget_t *, Gpoint_t);
static PIXsize_t  sdrawtopix (Gwidget_t *, Gsize_t);
static PIXrect_t  rdrawtobpix (Gbitmap_t *, Grect_t);
static PIXpoint_t pdrawtobpix (Gbitmap_t *, Gpoint_t);

int GPcreatewidget (
    Gwidget_t *parent, Gwidget_t *widget, int attrn, Gwattr_t *attrp
) {
    PIXpoint_t po;
    PIXsize_t ps;
    PRINTDLG pd;
    DOCINFO di;
    DEVMODE *dmp;
    /* the 2 here is to provide enough space for palPalEntry[0] and [1] */
    LOGPALETTE pal[2];

    HBRUSH brush;
    HPEN pen;
    HBITMAP bmap;
    char *s, *s1;
    int color, lflag, ai, dpix, dpiy, i;

    s = Gpscanvasname;
    lflag = FALSE;
    po.x = po.y = 0;
    ps.x = ps.y = MINPWSIZE;
    for (ai = 0; ai < attrn; ai++) {
        switch (attrp[ai].id) {
        case G_ATTRORIGIN:
            GETORIGIN (attrp[ai].u.p, po);
            break;
        case G_ATTRSIZE:
            GETSIZE (attrp[ai].u.s, ps, MINPWSIZE);
            break;
        case G_ATTRNAME:
            if (attrp[ai].u.t && attrp[ai].u.t[0])
                s = attrp[ai].u.t;
            break;
        case G_ATTRMODE:
            if (strcmp ("landscape", attrp[ai].u.t) == 0)
                lflag = TRUE;
            else if (strcmp ("portrait", attrp[ai].u.t) == 0)
                lflag = FALSE;
            else {
                Gerr (POS, G_ERRBADATTRVALUE, attrp[ai].u.t);
                return -1;
            }
            break;
        case G_ATTRCOLOR:
            /* will do it after the widget is created */
            break;
        case G_ATTRVIEWPORT:
            /* will do it after the widget is created */
            break;
        case G_ATTRWINDOW:
            /* will do it after the widget is created */
            break;
        case G_ATTRWINDOWID:
            Gerr (POS, G_ERRCANNOTSETATTR1, "windowid");
            return -1;
        case G_ATTRUSERDATA:
            widget->udata = attrp[ai].u.u;
            break;
        default:
            Gerr (POS, G_ERRBADATTRID, attrp[ai].id);
            return -1;
        }
    }
    s1 = s + strlen (s) - 3;
    if (s1 > s && strncmp (s1, "emf", 3) == 0) {
        WPU->mode = 1;
        ps.x *= 8.235, ps.y *= 8.235;
        if (!(GC = CreateEnhMetaFile (NULL, s, NULL, "LEFTY\\0GRAPH\\0\\0"))) {
            Gerr (POS, G_ERRCANNOTCREATEWIDGET);
            return -1;
        }
    } else { /* open the printer device */
        WPU->mode = 2;
        di.cbSize = sizeof (DOCINFO);
        di.lpszDocName = NULL;
        di.lpszOutput = NULL;
        di.lpszDatatype = "LEFTY";
        di.fwType = 0;
        pd.lStructSize = sizeof (pd);
        pd.hwndOwner = NULL;
        pd.hDevMode = NULL;
        pd.hDevNames = NULL;
        pd.hDC = NULL;
        pd.Flags = PD_RETURNDC | PD_RETURNDEFAULT;
        pd.nFromPage = 0;
        pd.nToPage = 0;
        pd.nMinPage = 0;
        pd.nMaxPage = 0;
        pd.nCopies = 1;
        pd.hInstance = NULL;
        pd.lCustData = NULL;
        pd.lpfnPrintHook = NULL;
        pd.lpfnSetupHook = NULL;
        pd.lpPrintTemplateName = NULL;
        pd.lpSetupTemplateName = NULL;
        pd.hPrintTemplate = NULL;
        pd.hSetupTemplate = NULL;
        if (!PrintDlg (&pd)) {
            Gerr (POS, G_ERRCANNOTCREATEWIDGET);
            return -1;
        }
        if (lflag && pd.hDevMode) {
            dmp = (DEVMODE *) GlobalLock (pd.hDevMode);
            dmp->dmOrientation = DMORIENT_LANDSCAPE;
            GlobalUnlock (pd.hDevMode);
            pd.Flags = PD_RETURNDC;
            if (!PrintDlg (&pd)) {
                Gerr (POS, G_ERRCANNOTCREATEWIDGET);
                return -1;
            }
        }
        GC = pd.hDC;
        dpix = GetDeviceCaps (GC, LOGPIXELSX);
        if (dpix != 300)
            ps.x = ps.x * (double) dpix / 300.0;
        dpiy = GetDeviceCaps (GC, LOGPIXELSY);
        if (dpiy != 300)
            ps.y = ps.y * (double) dpiy / 300.0;
        if (StartDoc (GC, &di) <= 0 || StartPage (GC) <= 0) {
            Gerr (POS, G_ERRCANNOTCREATEWIDGET);
            return -1;
        }
    }
    WPU->wrect.o.x = 0.0, WPU->wrect.o.y = 0.0;
    WPU->wrect.c.x = 1.0, WPU->wrect.c.y = 1.0;
    WPU->vsize.x = ps.x, WPU->vsize.y = ps.y;
    WPU->ncolor = 2;
    pal[0].palVersion = 0x300; /* HA HA HA */
    pal[0].palNumEntries = 2;
    pal[0].palPalEntry[0].peRed = 255;
    pal[0].palPalEntry[0].peGreen = 255;
    pal[0].palPalEntry[0].peBlue = 255;
    pal[0].palPalEntry[0].peFlags = 0;
    pal[0].palPalEntry[1].peRed = 0;
    pal[0].palPalEntry[1].peGreen = 0;
    pal[0].palPalEntry[1].peBlue = 0;
    pal[0].palPalEntry[1].peFlags = 0;
    WPU->cmap = CreatePalette (&pal[0]);
    WPU->colors[0].color = pal[0].palPalEntry[0];
    for (i = 1; i < G_MAXCOLORS; i++)
        WPU->colors[i].color = pal[0].palPalEntry[1];
    SelectPalette (GC, WPU->cmap, FALSE);
    RealizePalette (GC);
    WPU->colors[0].inuse = TRUE;
    WPU->colors[1].inuse = TRUE;
    for (i = 2; i < G_MAXCOLORS; i++)
        WPU->colors[i].inuse = FALSE;
    WPU->gattr.color = 1;
    brush = CreateSolidBrush (PALETTEINDEX (1));
    SelectObject (GC, brush);
    pen = CreatePen (PS_SOLID, 1, PALETTEINDEX (1));
    SelectObject (GC, pen);
    SetTextColor (GC, PALETTEINDEX (1));
    SetBkMode (GC, TRANSPARENT);
    WPU->gattr.width = 0;
    WPU->gattr.mode = G_SRC;
    WPU->gattr.fill = 0;
    WPU->gattr.style = 0;
    WPU->defgattr = WPU->gattr;
    WPU->font = NULL;
    if (Gdepth == 1) {
        for (i = 0; i < 17; i++) {
            if (!(bmap = CreateBitmap (4, 4, 1, 1, &grays[i][0])))
                continue;
            WPU->grays[i] = CreatePatternBrush (bmap);
        }
    }
    for (ai = 0; ai < attrn; ai++) {
        switch (attrp[ai].id) {
        case G_ATTRCOLOR:
            color = attrp[ai].u.c.index;
            if (color < 0 || color > G_MAXCOLORS) {
                Gerr (POS, G_ERRBADCOLORINDEX, color);
                return -1;
            }
            WPU->colors[color].color.peRed = attrp[ai].u.c.r;
            WPU->colors[color].color.peGreen = attrp[ai].u.c.g;
            WPU->colors[color].color.peBlue = attrp[ai].u.c.b;
            WPU->colors[color].color.peFlags = 0;
            if (color >= WPU->ncolor)
                ResizePalette (WPU->cmap, color + 1), WPU->ncolor = color + 1;
            SetPaletteEntries (
                WPU->cmap, (int) color, 1, &WPU->colors[color].color
            );
            RealizePalette (GC);
            WPU->colors[color].inuse = TRUE;
            if (color == WPU->gattr.color)
                WPU->gattr.color = -1;
            break;
        case G_ATTRVIEWPORT:
            if (attrp[ai].u.s.x == 0)
                attrp[ai].u.s.x = 1;
            if (attrp[ai].u.s.y == 0)
                attrp[ai].u.s.y = 1;
            WPU->vsize.x = attrp[ai].u.s.x + 0.5;
            WPU->vsize.y = attrp[ai].u.s.y + 0.5;
            SetWindowPos (
                widget->w, (HWND) NULL, 0, 0, WPU->vsize.x,
                WPU->vsize.y, SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOMOVE
            );
            break;
        case G_ATTRWINDOW:
            if (attrp[ai].u.r.o.x == attrp[ai].u.r.c.x)
                attrp[ai].u.r.c.x = attrp[ai].u.r.o.x + 1;
            if (attrp[ai].u.r.o.y == attrp[ai].u.r.c.y)
                attrp[ai].u.r.c.y = attrp[ai].u.r.o.y + 1;
            WPU->wrect = attrp[ai].u.r;
            break;
        }
    }
    return 0;
}

int GPsetwidgetattr (Gwidget_t *widget, int attrn, Gwattr_t *attrp) {
    PIXsize_t ps;
    int color, ai;

    for (ai = 0; ai < attrn; ai++) {
        switch (attrp[ai].id) {
        case G_ATTRORIGIN:
            break;
        case G_ATTRSIZE:
            break;
        case G_ATTRNAME:
            break;
        case G_ATTRMODE:
            break;
        case G_ATTRCOLOR:
            color = attrp[ai].u.c.index;
            if (color < 0 || color > G_MAXCOLORS) {
                Gerr (POS, G_ERRBADCOLORINDEX, color);
                return -1;
            }
            WPU->colors[color].color.peRed = attrp[ai].u.c.r;
            WPU->colors[color].color.peGreen = attrp[ai].u.c.g;
            WPU->colors[color].color.peBlue = attrp[ai].u.c.b;
            WPU->colors[color].color.peFlags = 0;
            if (color >= WPU->ncolor)
                ResizePalette (WPU->cmap, color + 1), WPU->ncolor = color + 1;
            SetPaletteEntries (
                WPU->cmap, (int) color, 1, &WPU->colors[color].color
            );
            RealizePalette (GC);
            WPU->colors[color].inuse = TRUE;
            if (color == WPU->gattr.color)
                WPU->gattr.color = -1;
            break;
        case G_ATTRVIEWPORT:
            if (attrp[ai].u.s.x == 0)
                attrp[ai].u.s.x = 1;
            if (attrp[ai].u.s.y == 0)
                attrp[ai].u.s.y = 1;
            WPU->vsize.x = attrp[ai].u.s.x + 0.5;
            WPU->vsize.y = attrp[ai].u.s.y + 0.5;
            ps.x = WPU->vsize.x, ps.y = WPU->vsize.y;
            break;
        case G_ATTRWINDOW:
            if (attrp[ai].u.r.o.x == attrp[ai].u.r.c.x)
                attrp[ai].u.r.c.x = attrp[ai].u.r.o.x + 1;
            if (attrp[ai].u.r.o.y == attrp[ai].u.r.c.y)
                attrp[ai].u.r.c.y = attrp[ai].u.r.o.y + 1;
            WPU->wrect = attrp[ai].u.r;
            break;
        case G_ATTRWINDOWID:
            Gerr (POS, G_ERRCANNOTSETATTR2, "windowid");
            return -1;
        case G_ATTRUSERDATA:
            widget->udata = attrp[ai].u.u;
            break;
        default:
            Gerr (POS, G_ERRBADATTRID, attrp[ai].id);
            return -1;
        }
    }
    return 0;
}

int GPgetwidgetattr (Gwidget_t *widget, int attrn, Gwattr_t *attrp) {
    PALETTEENTRY *cp;
    int color, ai;

    for (ai = 0; ai < attrn; ai++) {
        switch (attrp[ai].id) {
        case G_ATTRORIGIN:
            break;
        case G_ATTRSIZE:
            break;
        case G_ATTRNAME:
            break;
        case G_ATTRMODE:
            break;
        case G_ATTRCOLOR:
            color = attrp[ai].u.c.index;
            if (color < 0 || color > G_MAXCOLORS) {
                Gerr (POS, G_ERRBADCOLORINDEX, color);
                return -1;
            }
            if (WPU->colors[color].inuse) {
                cp = &WPU->colors[color].color;
                attrp[ai].u.c.r = cp->peRed;
                attrp[ai].u.c.g = cp->peGreen;
                attrp[ai].u.c.b = cp->peBlue;
            } else {
                attrp[ai].u.c.r = -1;
                attrp[ai].u.c.g = -1;
                attrp[ai].u.c.b = -1;
            }
            break;
        case G_ATTRVIEWPORT:
            attrp[ai].u.s = WPU->vsize;
            break;
        case G_ATTRWINDOW:
            attrp[ai].u.r = WPU->wrect;
            break;
        case G_ATTRWINDOWID:
            Gerr (POS, G_ERRCANNOTGETATTR, "windowid");
            break;
        case G_ATTRUSERDATA:
            widget->udata = attrp[ai].u.u;
            break;
        default:
            Gerr (POS, G_ERRBADATTRID, attrp[ai].id);
            return -1;
        }
    }
    return 0;
}

int GPdestroywidget (Gwidget_t *widget) {
    HENHMETAFILE mfile;

    if (WPU->mode == 1) {
        mfile = CloseEnhMetaFile (GC);
        OpenClipboard (NULL);
        EmptyClipboard ();
        SetClipboardData (CF_ENHMETAFILE, mfile);
        CloseClipboard ();
        DeleteMetaFile (mfile);
    } else {
        EndPage (GC);
        EndDoc (GC);
    }
    return 0;
}

int GPcanvasclear (Gwidget_t *widget) {
    HBRUSH brush, pbrush;

    /* FIXME: drain repaint messages */
    brush = CreateSolidBrush (PALETTEINDEX (0));
    pbrush = SelectObject (GC, brush);
    Rectangle (GC, 0, 0, (int) WPU->vsize.x, (int) WPU->vsize.y);
    SelectObject (GC, pbrush);
    return 0;
}

int GPsetgfxattr (Gwidget_t *widget, Ggattr_t *ap) {
    setgattr (widget, ap);
    WPU->defgattr = WPU->gattr;
    return 0;
}

int GPgetgfxattr (Gwidget_t *widget, Ggattr_t *ap) {
    if ((ap->flags & G_GATTRCOLOR))
        ap->color = WPU->gattr.color;
    if ((ap->flags & G_GATTRWIDTH))
        ap->width = WPU->gattr.width;
    if ((ap->flags & G_GATTRMODE))
        ap->mode = WPU->gattr.mode;
    if ((ap->flags & G_GATTRFILL))
        ap->fill = WPU->gattr.fill;
    if ((ap->flags & G_GATTRSTYLE))
        ap->style = WPU->gattr.style;
    return 0;
}

int GParrow (Gwidget_t *widget, Gpoint_t gp1, Gpoint_t gp2, Ggattr_t *ap) {
    PIXpoint_t pp1, pp2, pa, pb, pd;
    double tangent, l;

    pp1 = pdrawtopix (widget, gp1), pp2 = pdrawtopix (widget, gp2);
    pd.x = pp1.x - pp2.x, pd.y = pp1.y - pp2.y;
    if (pd.x == 0 && pd.y == 0)
        return 0;
    tangent = atan2 ((double) pd.y, (double) pd.x);
    if ((l = sqrt ((double) (pd.x * pd.x + pd.y * pd.y))) > 30)
        l = 30;
    pa.x = l * cos (tangent + M_PI / 7) + pp2.x;
    pa.y = l * sin (tangent + M_PI / 7) + pp2.y;
    pb.x = l * cos (tangent - M_PI / 7) + pp2.x;
    pb.y = l * sin (tangent - M_PI / 7) + pp2.y;
    setgattr (widget, ap);
    MoveToEx (GC, pp1.x, pp1.y, NULL), LineTo (GC, pp2.x, pp2.y);
    MoveToEx (GC, pa.x, pa.y, NULL), LineTo (GC, pp2.x, pp2.y);
    MoveToEx (GC, pb.x, pb.y, NULL), LineTo (GC, pp2.x, pp2.y);
    return 0;
}

int GPline (Gwidget_t *widget, Gpoint_t gp1, Gpoint_t gp2, Ggattr_t *ap) {
    PIXpoint_t pp1, pp2;

    pp1 = pdrawtopix (widget, gp1), pp2 = pdrawtopix (widget, gp2);
    setgattr (widget, ap);
    MoveToEx (GC, pp1.x, pp1.y, NULL);
    LineTo (GC, pp2.x, pp2.y);
    return 0;
}

int GPbox (Gwidget_t *widget, Grect_t gr, Ggattr_t *ap) {
    PIXrect_t pr;
    Grect_t gr2;

    if (gr.o.x <= gr.c.x)
        gr2.o.x = gr.o.x, gr2.c.x = gr.c.x;
    else
        gr2.o.x = gr.c.x, gr2.c.x = gr.o.x;
    if (gr.o.y <= gr.c.y)
        gr2.o.y = gr.o.y, gr2.c.y = gr.c.y;
    else
        gr2.o.y = gr.c.y, gr2.c.y = gr.o.y;
    pr = rdrawtopix (widget, gr);
    setgattr (widget, ap);
    if (WPU->gattr.fill)
        Rectangle (GC, pr.o.x, pr.o.y, pr.c.x, pr.c.y);
    else {
        Gppp[0].x = pr.o.x, Gppp[0].y = pr.o.y;
        Gppp[1].x = pr.c.x, Gppp[1].y = pr.o.y;
        Gppp[2].x = pr.c.x, Gppp[2].y = pr.c.y;
        Gppp[3].x = pr.o.x, Gppp[3].y = pr.c.y;
        Gppp[4].x = pr.o.x, Gppp[4].y = pr.o.y;
        Polyline (GC, Gppp, 5);
    }
    return 0;
}

int GPpolygon (Gwidget_t *widget, int gpn, Gpoint_t *gpp, Ggattr_t *ap) {
    int n, i;

    if (gpn == 0)
        return 0;
    if (gpn + 1 > Gppn) {
        n = (((gpn + 1) + PPINCR - 1) / PPINCR) * PPINCR;
        Gppp = Marraygrow (Gppp, (long) n * PPSIZE);
        Gppn = n;
    }
    for (i = 0; i < gpn; i++)
        Gppp[i] = pdrawtopix (widget, gpp[i]);
    setgattr (widget, ap);
    if (WPU->gattr.fill) {
        if (Gppp[gpn - 1].x != Gppp[0].x || Gppp[gpn - 1].y != Gppp[0].y)
            Gppp[gpn] = Gppp[0], gpn++;
        Polygon (GC, Gppp, (int) gpn);
    } else
        Polyline (GC, Gppp, (int) gpn);
    return 0;
}

int GPsplinegon (Gwidget_t *widget, int gpn, Gpoint_t *gpp, Ggattr_t *ap) {
    PIXpoint_t p0, p1, p2, p3;
    int n, i;

    if (gpn == 0)
        return 0;
    Gppi = 1;
    if (Gppi >= Gppn) {
        n = (((Gppi + 1) + PPINCR - 1) / PPINCR) * PPINCR;
        Gppp = Marraygrow (Gppp, (long) n * PPSIZE);
        Gppn = n;
    }
    Gppp[0] = p3 = pdrawtopix (widget, gpp[0]);
    for (i = 1; i < gpn; i += 3) {
        p0 = p3;
        p1 = pdrawtopix (widget, gpp[i]);
        p2 = pdrawtopix (widget, gpp[i + 1]);
        p3 = pdrawtopix (widget, gpp[i + 2]);
        bezier (p0, p1, p2, p3);
    }
    setgattr (widget, ap);
    if (WPU->gattr.fill) {
        if (Gppp[Gppi - 1].x != Gppp[0].x || Gppp[Gppi - 1].y != Gppp[0].y)
            Gppp[Gppi] = Gppp[0], Gppi++;
        Polygon (GC, Gppp, (int) Gppi);
    } else
        Polyline (GC, Gppp, (int) Gppi);
    return 0;
}

static void bezier (
    PIXpoint_t p0, PIXpoint_t p1, PIXpoint_t p2, PIXpoint_t p3
) {
    Gpoint_t gp0, gp1, gp2;
    Gsize_t s;
    PIXpoint_t p;
    double t;
    int n, i, steps;

    if ((s.x = p3.x - p0.x) < 0)
        s.x = - s.x;
    if ((s.y = p3.y - p0.y) < 0)
        s.y = - s.y;
    if (s.x > s.y)
        steps = s.x / 5 + 1;
    else
        steps = s.y / 5 + 1;
    for (i = 0; i <= steps; i++) {
        t = i / (double) steps;
        gp0.x = p0.x + t * (p1.x - p0.x);
        gp0.y = p0.y + t * (p1.y - p0.y);
        gp1.x = p1.x + t * (p2.x - p1.x);
        gp1.y = p1.y + t * (p2.y - p1.y);
        gp2.x = p2.x + t * (p3.x - p2.x);
        gp2.y = p2.y + t * (p3.y - p2.y);
        gp0.x = gp0.x + t * (gp1.x - gp0.x);
        gp0.y = gp0.y + t * (gp1.y - gp0.y);
        gp1.x = gp1.x + t * (gp2.x - gp1.x);
        gp1.y = gp1.y + t * (gp2.y - gp1.y);
        p.x = gp0.x + t * (gp1.x - gp0.x) + 0.5;
        p.y = gp0.y + t * (gp1.y - gp0.y) + 0.5;
        if (Gppi >= Gppn) {
            n = (((Gppi + 1) + PPINCR - 1) / PPINCR) * PPINCR;
            Gppp = Marraygrow (Gppp, (long) n * PPSIZE);
            Gppn = n;
        }
        Gppp[Gppi++] = p;
    }
}

int GParc (
    Gwidget_t *widget, Gpoint_t gc, Gsize_t gs,
    double ang1, double ang2, Ggattr_t *ap
) {
    PIXpoint_t pc;
    PIXsize_t ps;
    double a1, a2;

    pc = pdrawtopix (widget, gc), ps = sdrawtopix (widget, gs);
    setgattr (widget, ap);
    a1 = ang1 * M_PI / 180, a2 = ang2 * M_PI / 180;
    if (WPU->gattr.fill)
        Chord (
            GC, pc.x - ps.x, pc.y - ps.y, pc.x + ps.x, pc.y + ps.y,
            (int) (cos (a1) * ps.x), (int) (sin (a1) * ps.x),
            (int) (cos (a2) * ps.x), (int) (sin (a2) * ps.x)
        );
    else
        Arc (
            GC, pc.x - ps.x, pc.y - ps.y, pc.x + ps.x, pc.y + ps.y,
            (int) (cos (a1) * ps.x), (int) (sin (a1) * ps.x),
            (int) (cos (a2) * ps.x), (int) (sin (a2) * ps.x)
        );
    return 0;
}

int GPtext (
    Gwidget_t *widget, Gtextline_t *tlp, int n, Gpoint_t go, char *fn,
    double fs, char *justs, Ggattr_t *ap
) {
    Gsize_t gs;
    PIXpoint_t po;
    PIXsize_t ps;
    PIXrect_t pr;
    HFONT font;
    SIZE size;
    RECT r;
    int x, y, w, h, i;

    po = pdrawtopix (widget, go);
    gs.x = 0, gs.y = fs;
    ps = sdrawtopix (widget, gs);
    if (!(font = findfont (fn, ps.y))) {
        Rectangle (GC, po.x, po.y, po.x + 1, po.y + 1);
        return 0;
    }
    setgattr (widget, ap);
    if (font != WPU->font) {
        WPU->font = font;
        SelectObject (GC, font);
    }
    for (w = h = 0, i = 0; i < n; i++) {
        if (tlp[i].n)
            GetTextExtentPoint32 (GC, tlp[i].p, (int) tlp[i].n, &size);
        else
            GetTextExtentPoint32 (GC, "M", (int) 1, &size);
        tlp[i].w = size.cx, tlp[i].h = size.cy;
        w = max (w, size.cx), h += size.cy;
    }
    switch (justs[0]) {
    case 'l': po.x += w / 2; break;
    case 'r': po.x -= w / 2; break;
    }
    switch (justs[1]) {
    case 'd': po.y -= h; break;
    case 'c': po.y -= h / 2; break;
    }
    pr.o.x = po.x - w / 2, pr.o.y = po.y;
    pr.c.x = po.x + w / 2, pr.c.y = po.y + h;
    for (i = 0; i < n; i++) {
        switch (tlp[i].j) {
        case 'l': x = po.x - w / 2; break;
        case 'n': x = po.x - tlp[i].w / 2; break;
        case 'r': x = po.x - (tlp[i].w - w / 2); break;
        }
        y = po.y + i * tlp[i].h;
        r.left = x, r.top = y;
        r.right = x + tlp[i].w, r.bottom = y + tlp[i].h;
        DrawText (GC, tlp[i].p, (int) tlp[i].n, &r, DT_LEFT | DT_TOP);
    }
    return 0;
}

static HFONT findfont (char *name, int size) {
    HFONT font;
    int fi;

    if (name[0] == '\000')
        return Gfontp[0].font;

    sprintf (&Gbufp[0], name, size);
    for (fi = 0; fi < Gfontn; fi++)
        if (strcmp (&Gbufp[0], Gfontp[fi].name) == 0 && Gfontp[fi].size == size)
            return Gfontp[fi].font;
    font = CreateFont (
        (int) size, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, &Gbufp[0]
    );
    if (!font)
        font = Gfontp[0].font;

    Gfontp = Marraygrow (Gfontp, (long) (Gfontn + 1) * FONTSIZE);
    Gfontp[Gfontn].name = strdup (&Gbufp[0]);
    Gfontp[Gfontn].size = size;
    Gfontp[Gfontn].font = font;
    Gfontn++;
    return font;
}

int GPcreatebitmap (Gwidget_t *widget, Gbitmap_t *bitmap, Gsize_t s) {
    if (!widget) {
        Gerr (POS, G_ERRNOPARENTWIDGET);
        return -1;
    }
    if (!bitmap) {
        Gerr (POS, G_ERRNOBITMAP);
        return -1;
    }
    if (!(bitmap->u.bmap.orig = CreateBitmap (
        (int) s.x, (int) s.y, 1, Gdepth, NULL
    ))) {
        Gerr (POS, G_ERRCANNOTCREATEBITMAP);
        return -1;
    }
    bitmap->u.bmap.scaled = 0;
    bitmap->scale.x = bitmap->scale.y = 1;
    bitmap->ctype = widget->type;
    bitmap->canvas = widget - &Gwidgets[0];
    bitmap->size = s;
    return 0;
}

int GPdestroybitmap (Gbitmap_t *bitmap) {
    if (!bitmap) {
        Gerr (POS, G_ERRNOBITMAP);
        return -1;
    }
    DeleteObject (bitmap->u.bmap.orig);
    return 0;
}

int GPreadbitmap (Gwidget_t *widget, Gbitmap_t *bitmap, FILE *fp) {
    Gsize_t s;
    HDC gc;
    char bufp[2048];
    unsigned int rgb[3];
    char *s1, *s2;
    char c;
    int bufn, bufi, step, x, y, k;

    if (!widget) {
        Gerr (POS, G_ERRNOPARENTWIDGET);
        return -1;
    }
    if (!bitmap) {
        Gerr (POS, G_ERRNOBITMAP);
        return -1;
    }
    step = 0;
    while (step < 3) {
l1:
        if (!fgets (bufp, 2048, fp)) {
            Gerr (POS, G_ERRCANNOTREADBITMAP);
            return -1;
        }
        s1 = &bufp[0];
l2:
        for (; *s1 && isspace (*s1); s1++)
            ;
        if (!*s1 || *s1 == '#')
            goto l1;
        switch (step) {
        case 0:
            if (strncmp (s1, "P6", 2) != 0) {
                Gerr (POS, G_ERRCANNOTREADBITMAP);
                return -1;
            }
            step++, s1 += 2;
            goto l2;
        case 1:
            for (s2 = s1; *s2 && *s2 >= '0' && *s2 <= '9'; s2++)
                ;
            c = *s2, *s2 = 0;
            if (s2 == s1 || (s.x = atoi (s1)) <= 0) {
                *s2 = c, Gerr (POS, G_ERRCANNOTREADBITMAP);
                return -1;
            }
            *s2 = c, step++, s1 = s2;
            goto l2;
        case 2:
            for (s2 = s1; *s2 && *s2 >= '0' && *s2 <= '9'; s2++)
                ;
            c = *s2, *s2 = 0;
            if (s2 == s1 || (s.y = atoi (s1)) <= 0) {
                *s2 = c, Gerr (POS, G_ERRCANNOTREADBITMAP);
                return -1;
            }
            *s2 = c, step++, s1 = s2;
            goto l2;
        }
    }
    if (!(bitmap->u.bmap.orig = CreateBitmap (
        (int) s.x, (int) s.y, 1, Gdepth, NULL
    ))) {
        Gerr (POS, G_ERRCANNOTCREATEBITMAP);
        return -1;
    }
    gc = CreateCompatibleDC (GC);
    SelectObject (gc, bitmap->u.bmap.orig);
    bitmap->u.bmap.scaled = 0;
    bitmap->scale.x = bitmap->scale.y = 1;
    bitmap->ctype = widget->type;
    bitmap->canvas = widget - &Gwidgets[0];
    bitmap->size = s;
    bufi = bufn = 0;
    bufp[bufi] = 0;
    for (y = 0; y < s.y; y++) {
        for (x = 0; x < s.x; x++) {
            for (k = 0; k < 3; k++) {
                if (bufi == bufn) {
                    if ((bufn = fread (bufp, 1, 2047, fp)) == 0) {
                        if (ferror (fp))
                            bufn = -1;
                        DeleteDC (gc);
                        DeleteObject (bitmap->u.bmap.orig);
                        Gerr (POS, G_ERRCANNOTCREATEBITMAP);
                        return -1;
                    }
                    bufi = 0;
                }
                rgb[k] = (unsigned char) bufp[bufi++];
            }
            SetPixel (gc, x, y, RGB (rgb[0], rgb[1], rgb[2]));
        }
    }
    DeleteDC (gc);
    return 0;
}

int GPwritebitmap (Gbitmap_t *bitmap, FILE *fp) {
    Gwidget_t *widget;
    HDC gc;
    COLORREF color;
    char bufp[2048];
    int bufi, x, y, w, h;

    if (!bitmap) {
        Gerr (POS, G_ERRNOBITMAP);
        return -1;
    }
    if (
        bitmap->canvas < 0 || bitmap->canvas >= Gwidgetn ||
        !Gwidgets[bitmap->canvas].inuse
    ) {
        Gerr (POS, G_ERRBADWIDGETID, bitmap->canvas);
        return -1;
    }
    widget = &Gwidgets[bitmap->canvas];
    if (widget->type != G_CANVASWIDGET && widget->type != G_PCANVASWIDGET) {
        Gerr (POS, G_ERRNOTACANVAS, bitmap->canvas);
        return -1;
    }
    gc = CreateCompatibleDC (GC);
    SelectObject (gc, bitmap->u.bmap.orig);
    fprintf (fp, "P6\n%d %d 255\n", (int) bitmap->size.x, (int) bitmap->size.y);
    bufi = 0;
    w = bitmap->size.x;
    h = bitmap->size.y;
    for (y = 0; y < h; y++) {
        for (x = 0; x < w; x++) {
            color = GetPixel (gc, x, y);
            bufp[bufi++] = GetRValue (color);
            bufp[bufi++] = GetGValue (color);
            bufp[bufi++] = GetBValue (color);
            if (bufi + 3 >= 2048) {
                fwrite (bufp, 1, bufi, fp);
                bufi = 0;
            }
        }
    }
    if (bufi > 0)
        fwrite (bufp, 1, bufi, fp);
    DeleteDC (gc);
    return 0;
}

int GPbitblt (
    Gwidget_t *widget, Gpoint_t gp, Grect_t gr, Gbitmap_t *bitmap,
    char *mode, Ggattr_t *ap
) {
    PIXrect_t pr, r;
    PIXpoint_t pp;
    PIXsize_t s;
    Gsize_t scale;
    Gxy_t p;
    HBITMAP pix;
    HDC gc;
    double tvx, tvy, twx, twy;

    if (gr.o.x > gr.c.x)
        p.x = gr.o.x, gr.o.x = gr.c.x, gr.c.x = p.x;
    if (gr.o.y > gr.c.y)
        p.y = gr.o.y, gr.o.y = gr.c.y, gr.c.y = p.y;
    if (strcmp (mode, "b2c") == 0) {
        tvx = WPU->vsize.x, tvy = WPU->vsize.y;
        twx = WPU->wrect.c.x - WPU->wrect.o.x;
        twy = WPU->wrect.c.y - WPU->wrect.o.y;
        scale.x = tvx / twx, scale.y = tvy / twy;
        if (scale.x == 1 && scale.y == 1) {
            pix = bitmap->u.bmap.orig;
            bitmap->scale = scale;
        } else {
            if (scale.x != bitmap->scale.x || scale.y != bitmap->scale.y)
                scalebitmap (widget, bitmap, scale, TRUE, 1);
            pix = bitmap->u.bmap.scaled;
        }
        pr = rdrawtopix (widget, gr);
        pp = pdrawtobpix (bitmap, gp);
        s.x = pr.c.x - pr.o.x + 1, s.y = pr.c.y - pr.o.y + 1;
        r.o.x = pp.x, r.o.y = pp.y - s.y + 1;
        r.c.x = r.o.x + s.x - 1, r.c.y = r.o.y + s.y - 1;
        if (r.o.x < 0)
            pr.o.x -= r.o.x, r.o.x = 0;
        if (r.o.y < 0)
            pr.o.y -= r.o.y, r.o.y = 0;
        if (r.c.x >= bitmap->size.x * scale.x) {
            pr.c.x -= (r.c.x + 1 - bitmap->size.x * scale.x);
            r.c.x = bitmap->size.x * scale.x - 1;
        }
        if (r.c.y >= bitmap->size.y * scale.y) {
            pr.c.y -= (r.c.y + 1 - bitmap->size.y * scale.y);
            r.c.y = bitmap->size.y * scale.y - 1;
        }
        if (pr.o.x < 0)
            r.o.x -= pr.o.x, pr.o.x = 0;
        if (pr.o.y < 0)
            r.o.y -= pr.o.y, pr.o.y = 0;
        setgattr (widget, ap);
        gc = CreateCompatibleDC (GC);
        SelectObject (gc, pix);
        BitBlt (
            GC, pr.o.x, pr.o.y, r.c.x - r.o.x + 1, r.c.y - r.o.y + 1,
            gc, r.o.x, r.o.y, (WPU->gattr.mode == G_SRC) ? SRCCOPY : SRCINVERT
        );
        DeleteDC (gc);
    } else if (strcmp (mode, "c2b") == 0) {
        tvx = WPU->vsize.x, tvy = WPU->vsize.y;
        twx = WPU->wrect.c.x - WPU->wrect.o.x;
        twy = WPU->wrect.c.y - WPU->wrect.o.y;
        scale.x = tvx / twx, scale.y = tvy / twy;
        if (scale.x == 1 && scale.y == 1) {
            pix = bitmap->u.bmap.orig;
            bitmap->scale = scale;
        } else {
            if (scale.x != bitmap->scale.x || scale.y != bitmap->scale.y)
                scalebitmap (widget, bitmap, scale, FALSE, 1);
            pix = bitmap->u.bmap.scaled;
        }
        pr = rdrawtobpix (bitmap, gr);
        pp = pdrawtopix (widget, gp);
        s.x = pr.c.x - pr.o.x + 1, s.y = pr.c.y - pr.o.y + 1;
        r.o.x = pp.x, r.o.y = pp.y - s.y + 1;
        r.c.x = r.o.x + s.x - 1, r.c.y = r.o.y + s.y - 1;
        if (pr.o.x < 0)
            r.o.x -= pr.o.x, pr.o.x = 0;
        if (pr.o.y < 0)
            r.o.y -= pr.o.y, pr.o.y = 0;
        if (pr.c.x >= bitmap->size.x * scale.x) {
            r.c.x -= (pr.c.x + 1 - bitmap->size.x * scale.x);
            pr.c.x = bitmap->size.x * scale.x - 1;
        }
        if (pr.c.y >= bitmap->size.y * scale.y) {
            r.c.y -= (pr.c.y + 1 - bitmap->size.y * scale.y);
            pr.c.y = bitmap->size.y * scale.y - 1;
        }
        if (r.o.x < 0)
            pr.o.x -= r.o.x, r.o.x = 0;
        if (r.o.y < 0)
            pr.o.y -= r.o.y, r.o.y = 0;
        setgattr (widget, ap);
        gc = CreateCompatibleDC (GC);
        SelectObject (gc, pix);
        BitBlt (
            gc, pr.o.x, pr.o.y, r.c.x - r.o.x + 1, r.c.y - r.o.y + 1,
            GC, r.o.x, r.o.y, (WPU->gattr.mode == G_SRC) ? SRCCOPY : SRCINVERT
        );
        if (pix != bitmap->u.bmap.orig)
            scalebitmap (widget, bitmap, scale, TRUE, -1);
        DeleteDC (gc);
    }
    return 0;
}

static int scalebitmap (
    Gwidget_t *widget, Gbitmap_t *bitmap, Gsize_t scale,
    int copybits, int dir
) {
    Gsize_t nsize, o2n;
    HBITMAP opix, spix;
    COLORREF color;
    HDC gc1, gc2;
    int x, y, x2, y2, xp, yp;
    double prod, rgb[3], xr2, yr2, xl2, yl2, xf2, yf2, xr, yr, xl, yl;

    if (!copybits) {
        if (dir == 1) {
            nsize.x = (int) (bitmap->size.x * scale.x);
            nsize.y = (int) (bitmap->size.y * scale.y);
            if (!(spix = CreateBitmap (
                (int) nsize.x, (int) nsize.y, 1, Gdepth, NULL
            ))) {
                Gerr (POS, G_ERRCANNOTCREATEBITMAP);
                return -1;
            }
            if (bitmap->u.bmap.scaled)
                DeleteObject (bitmap->u.bmap.scaled);
            bitmap->u.bmap.scaled = spix;
            bitmap->scale = scale;
        }
        return 0;
    }
    if (dir == 1) {
        nsize.x = (int) (bitmap->size.x * scale.x);
        nsize.y = (int) (bitmap->size.y * scale.y);
        o2n.x = 1 / scale.x, o2n.y = 1 / scale.y;
        if (!(spix = CreateBitmap (
            (int) nsize.x, (int) nsize.y, 1, Gdepth, NULL
        ))) {
            Gerr (POS, G_ERRCANNOTCREATEBITMAP);
            return -1;
        }
        opix = bitmap->u.bmap.orig;
    } else {
        nsize.x = (int) bitmap->size.x;
        nsize.y = (int) bitmap->size.y;
        o2n.x = scale.x, o2n.y = scale.y;
        spix = bitmap->u.bmap.orig;
        opix = bitmap->u.bmap.scaled;
    }
    gc1 = CreateCompatibleDC (GC);
    SelectObject (gc1, opix);
    gc2 = CreateCompatibleDC (GC);
    SelectObject (gc2, spix);
    prod = o2n.x * o2n.y;
    y = 0;
    yr = o2n.y;
    yl = 0;
    for (yp = 0; yp < nsize.y; yp++) {
        x = 0;
        xr = o2n.x;
        xl = 0;
        for (xp = 0; xp < nsize.x; xp++) {
            y2 = y;
            yr2 = yr;
            yl2 = yl;
            rgb[0] = rgb[1] = rgb[2] = 0;
            do {
                x2 = x;
                xr2 = xr;
                xl2 = xl;
                yf2 = (yl2 + yr2 > 1) ? 1 - yl2 : yr2, yr2 -= yf2;
                do {
                    xf2 = (xl2 + xr2 > 1) ? 1 - xl2 : xr2, xr2 -= xf2;
                    color = GetPixel (gc1, x2, y2);
                    rgb[0] += (GetRValue (color) * xf2 * yf2 / prod);
                    rgb[1] += (GetGValue (color) * xf2 * yf2 / prod);
                    rgb[2] += (GetBValue (color) * xf2 * yf2 / prod);
                    xl2 += xf2;
                    if (xl2 >= 1)
                        x2++, xl2 -= 1;
                } while (xr2 > 0);
                xr2 = o2n.x;
                yl2 += yf2;
                if (yl2 >= 1)
                    y2++, yl2 -= 1;
            } while (yr2 > 0);
            yr2 = o2n.y;
            SetPixel (gc2, xp, yp, RGB (rgb[0], rgb[1], rgb[2]));
            x = x2;
            xr = xr2;
            xl = xl2;
        }
        y = y2;
        yr = yr2;
        yl = yl2;
    }
    DeleteDC (gc1);
    DeleteDC (gc2);
    if (dir == 1) {
        if (bitmap->u.bmap.scaled)
            DeleteObject (bitmap->u.bmap.scaled);
        bitmap->u.bmap.scaled = spix;
        bitmap->scale = scale;
    }
    return 0;
}

static void setgattr (Gwidget_t *widget, Ggattr_t *ap) {
    HBRUSH brush, pbrush;
    HPEN pen, ppen;
    PALETTEENTRY *colorp;
    long color, mode, style, width, flag, pati;
    double intens;

    if (!(ap->flags & G_GATTRCOLOR))
        ap->color = WPU->defgattr.color;
    if (!(ap->flags & G_GATTRWIDTH))
        ap->width = WPU->defgattr.width;
    if (!(ap->flags & G_GATTRMODE))
        ap->mode = WPU->defgattr.mode;
    if (!(ap->flags & G_GATTRFILL))
        ap->fill = WPU->defgattr.fill;
    if (!(ap->flags & G_GATTRSTYLE))
        ap->style = WPU->defgattr.style;
    flag = FALSE;
    mode = ap->mode;
    if (mode != WPU->gattr.mode) {
        WPU->gattr.mode = mode;
        SetROP2 (GC, (int) mode);
    }
    WPU->gattr.fill = ap->fill;
    color = ap->color;
    if (color >= G_MAXCOLORS || !(WPU->colors[color].inuse))
        color = 1;
    if (color != WPU->gattr.color)
        WPU->gattr.color = color, flag = TRUE;
    width = ap->width;
    if (width != WPU->gattr.width)
        WPU->gattr.width = width, flag = TRUE;
    style = ap->style;
    if (style != WPU->gattr.style)
        WPU->gattr.style = style, flag = TRUE;

    if (!flag)
        return;
    WPU->gattr.color = color;
    if (Gdepth == 1) {
        colorp = &WPU->colors[color].color;
        intens = (
            0.3 * colorp->peBlue + 0.59 * colorp->peRed +
            0.11 * colorp->peGreen
        ) / 255.0;
        pati = (intens <= 0.0625) ? 16 : -16.0 * (log (intens) / 2.7725887222);
        brush = WPU->grays[pati];
    } else
        brush = CreateSolidBrush (PALETTEINDEX (WPU->gattr.color));
    pbrush = SelectObject (GC, brush);
    if (Gdepth != 1)
        DeleteObject (pbrush);
    pen = CreatePen (
        (int) gstyles[WPU->gattr.style], WPU->gattr.width,
        PALETTEINDEX (WPU->gattr.color)
    );
    ppen = SelectObject (GC, pen);
    DeleteObject (ppen);
    SetTextColor (GC, PALETTEINDEX (WPU->gattr.color));
}

static PIXrect_t rdrawtopix (Gwidget_t *widget, Grect_t gr) {
    PIXrect_t pr;
    double tvx, tvy, twx, twy;

    tvx = WPU->vsize.x - 1, tvy = WPU->vsize.y - 1;
    twx = WPU->wrect.c.x - WPU->wrect.o.x;
    twy = WPU->wrect.c.y - WPU->wrect.o.y;
    pr.o.x = tvx * (gr.o.x - WPU->wrect.o.x) / twx + 0.5;
    pr.o.y = tvy * (1.0  - (gr.c.y - WPU->wrect.o.y) / twy) + 0.5;
    pr.c.x = tvx * (gr.c.x - WPU->wrect.o.x) / twx + 0.5;
    pr.c.y = tvy * (1.0  - (gr.o.y - WPU->wrect.o.y) / twy) + 0.5;
    return pr;
}

static PIXpoint_t pdrawtopix (Gwidget_t *widget, Gpoint_t gp) {
    PIXpoint_t pp;
    double tvx, tvy, twx, twy;

    tvx = WPU->vsize.x - 1, tvy = WPU->vsize.y - 1;
    twx = WPU->wrect.c.x - WPU->wrect.o.x;
    twy = WPU->wrect.c.y - WPU->wrect.o.y;
    pp.x = tvx * (gp.x - WPU->wrect.o.x) / twx + 0.5;
    pp.y = tvy * (1.0  - (gp.y - WPU->wrect.o.y) / twy) + 0.5;
    return pp;
}

static PIXsize_t sdrawtopix (Gwidget_t *widget, Gsize_t gs) {
    PIXsize_t ps;
    double tvx, tvy, twx, twy;

    tvx = WPU->vsize.x - 1, tvy = WPU->vsize.y - 1;
    twx = WPU->wrect.c.x - WPU->wrect.o.x;
    twy = WPU->wrect.c.y - WPU->wrect.o.y;
    ps.x = tvx * (gs.x - 1) / twx + 1.5;
    ps.y = tvy * (gs.y - 1) / twy + 1.5;
    return ps;
}

static PIXrect_t rdrawtobpix (Gbitmap_t *bitmap, Grect_t gr) {
    PIXrect_t pr;
    double tvy;

    tvy = (int) ((bitmap->size.y - 1) * bitmap->scale.y);
    pr.o.x = gr.o.x + 0.5;
    pr.o.y = tvy - gr.c.y + 0.5;
    pr.c.x = gr.c.x + 0.5;
    pr.c.y = tvy - gr.o.y + 0.5;
    return pr;
}

static PIXpoint_t pdrawtobpix (Gbitmap_t *bitmap, Gpoint_t gp) {
    PIXpoint_t pp;
    double tvy;

    tvy = (int) ((bitmap->size.y - 1) * bitmap->scale.y);
    pp.x = gp.x + 0.5;
    pp.y = tvy - gp.y + 0.5;
    return pp;
}
