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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>

#include "gvplugin_render.h"
#include "gvplugin_device.h"
#include "gvio.h"
#include "agxbuf.h"
#include "utils.h"
#include "color.h"

#include "const.h"

/* Number of points to split splines into */
#define BEZIERSUBDIVISION 6

#define PIC_COORDS_PER_LINE (16)        /* to avoid stdio BUF overflow */

typedef enum { FORMAT_PIC, } format_type;

static int BezierSubdivision = 10;
static int onetime = TRUE;
static double Fontscale;

/* There are a couple of ways to generate output: 
    1. generate for whatever size is given by the bounding box
       - the drawing at its "natural" size might not fit on a physical page
         ~ dot size specification can be used to scale the drawing
         ~ and it's not difficult for user to scale the pic output to fit (multiply 4 (3 distinct) numbers on 3 lines by a scale factor)
       - some troff implementations may clip large graphs
         ~ handle by scaling to manageable size
       - give explicit width and height as parameters to .PS
       - pic scale variable is reset to 1.0
       - fonts are printed as size specified by caller, modified by user scaling
    2. scale to fit on a physical page
       - requires an assumption of page size (GNU pic assumes 8.5x11.0 inches)
         ~ any assumption is bound to be wrong more often than right
       - requires separate scaling of font point sizes since pic's scale variable doesn't affect text
         ~ possible, as above
       - likewise for line thickness
       - GNU pic does this (except for fonts) if .PS is used without explicit width or height; DWB pic does not
         ~ pic variants likely to cause trouble
  The first approach is used here.
*/

static const char *EscComment = ".\\\" ";       /* troff comment */
static const char picgen_msghdr[] = "dot pic plugin: ";

static void unsupported(char *s)
{
    agerr(AGWARN, "%s%s unsupported\n", picgen_msghdr, s);
}
static void warn(char *s)
{
    agerr(AGWARN, "%s%s\n", picgen_msghdr, s);
}

/* troff font mapping */
typedef struct {
    char trname[3], *psname;
} fontinfo;

static fontinfo fonttab[] = {
    {"AB", "AvantGarde-Demi"},
    {"AI", "AvantGarde-BookOblique"},
    {"AR", "AvantGarde-Book"},
    {"AX", "AvantGarde-DemiOblique"},
    {"B ", "Times-Bold"},
    {"BI", "Times-BoldItalic"},
    {"CB", "Courier-Bold"},
    {"CO", "Courier"},
    {"CX", "Courier-BoldOblique"},
    {"H ", "Helvetica"},
    {"HB", "Helvetica-Bold"},
    {"HI", "Helvetica-Oblique"},
    {"HX", "Helvetica-BoldOblique"},
    {"Hb", "Helvetica-Narrow-Bold"},
    {"Hi", "Helvetica-Narrow-Oblique"},
    {"Hr", "Helvetica-Narrow"},
    {"Hx", "Helvetica-Narrow-BoldOblique"},
    {"I ", "Times-Italic"},
    {"KB", "Bookman-Demi"},
    {"KI", "Bookman-LightItalic"},
    {"KR", "Bookman-Light"},
    {"KX", "Bookman-DemiItalic"},
    {"NB", "NewCenturySchlbk-Bold"},
    {"NI", "NewCenturySchlbk-Italic"},
    {"NR", "NewCenturySchlbk-Roman"},
    {"NX", "NewCenturySchlbk-BoldItalic"},
    {"PA", "Palatino-Roman"},
    {"PB", "Palatino-Bold"},
    {"PI", "Palatino-Italic"},
    {"PX", "Palatino-BoldItalic"},
    {"R ", "Times-Roman"},
    {"S ", "Symbol"},
    {"ZD", "ZapfDingbats"},
    {"\000\000", (char *) 0}
};

static char *picfontname(char *psname)
{
    char *rv;
    fontinfo *p;

    for (p = fonttab; p->psname; p++)
        if (strcmp(p->psname, psname) == 0)
            break;
    if (p->psname)
        rv = p->trname;
    else {
        agerr(AGERR, "%s%s is not a troff font\n", picgen_msghdr, psname);
        /* try base font names, e.g. Helvetica-Outline-Oblique -> Helvetica-Outline -> Helvetica */
        if ((rv = strrchr(psname, '-'))) {
            *rv = '\0';         /* psname is not specified as const ... */
            rv = picfontname(psname);
        } else
            rv = "R";
    }
    return rv;
}

static void pic_set_color(GVJ_t *job, char *name)
{
    gvcolor_t color;

    colorxlate(name, &color, HSVA_DOUBLE);
    /* just v used to set grayscale value */
    gvprintf(job, "setfillval %f\n", color.u.HSVA[2]);
}

static void pic_set_style(GVJ_t *job, char **s)
{
    const char *line, *p;
    char skip = 0;
    char buf[BUFSIZ];

    buf[0] = '\0';
    gvprintf(job, "define attrs%d %%", 0);
    while ((p = line = *s++)) {
        while (*p)
            p++;
        p++;
        while (*p) {
            if (!strcmp(line, "setlinewidth")) {        /* a hack to handle the user-defined (PS) style spec in proc3d.gv */
                long n = atol(p);

                sprintf(buf,
                        "oldlinethick = linethick;linethick = %ld * scalethickness / %.0f\n",
                        n, Fontscale);
                skip = 1;
            } else
                gvprintf(job, " %s", p);
            while (*p)
                p++;
            p++;
        }
        if (!skip)
            gvprintf(job, " %s", line);
        skip = 0;
    }
    gvprintf(job, " %%\n");
    gvprintf(job, "%s", buf);
}

static void picptarray(GVJ_t *job, pointf * A, int n, int close)
{
    int i;
    point p;

    for (i = 0; i < n; i++) {
	PF2P(A[i],p);
        gvprintf(job, " %d %d", p.x, p.y);
    }
    if (close) {
	PF2P(A[0],p);
        gvprintf(job, " %d %d", p.x, p.y);
    }
    gvputs(job, "\n");
}

static char *pic_string(char *s)
{
    static char *buf = NULL;
    static int bufsize = 0;
    int pos = 0;
    char *p;
    unsigned char c;

    if (!buf) {
        bufsize = 64;
        buf = malloc(bufsize * sizeof(char));
    }

    p = buf;
    while ((c = *s++)) {
        if (pos > (bufsize - 8)) {
            bufsize *= 2;
            buf = realloc(buf, bufsize * sizeof(char));
            p = buf + pos;
        }
        if (isascii(c)) {
            if (c == '\\') {
                *p++ = '\\';
                pos++;
            }
            *p++ = c;
            pos++;
        } else {
            *p++ = '\\';
            sprintf(p, "%03o", c);
            p += 3;
            pos += 4;
        }
    }
    *p = '\0';
    return buf;
}

static int picColorResolve(int *new, int r, int g, int b)
{
#define maxColors 256
    static int top = 0;
    static short red[maxColors], green[maxColors], blue[maxColors];
    int c;
    int ct = -1;
    long rd, gd, bd, dist;
    long mindist = 3 * 255 * 255;       /* init to max poss dist */

    *new = 0;                   /* in case it is not a new color */
    for (c = 0; c < top; c++) {
        rd = (long) (red[c] - r);
        gd = (long) (green[c] - g);
        bd = (long) (blue[c] - b);
        dist = rd * rd + gd * gd + bd * bd;
        if (dist < mindist) {
            if (dist == 0)
                return c;       /* Return exact match color */
            mindist = dist;
            ct = c;
        }
    }
    /* no exact match.  We now know closest, but first try to allocate exact */
    if (top++ == maxColors)
        return ct;              /* Return closest available color */
    red[c] = r;
    green[c] = g;
    blue[c] = b;
    *new = 1;                   /* flag new color */
    return c;                   /* Return newly allocated color */
}

static void pic_line_style(obj_state_t *obj, int *line_style, double *style_val)
{
    switch (obj->pen) {
	case PEN_DASHED: 
	    *line_style = 1;
	    *style_val = 10.;
	    break;
	case PEN_DOTTED:
	    *line_style = 2;
	    *style_val = 10.;
	    break;
	case PEN_SOLID:
	default:
	    *line_style = 0;
	    *style_val = 0.;
	    break;
    }
}

static void pic_comment(GVJ_t *job, char *str)
{
    gvprintf(job, "%s %s\n", EscComment, str);
}

static void pic_begin_graph(GVJ_t * job)
{
    obj_state_t *obj = job->obj;

    gvprintf(job, "%s Creator: %s version %s (%s)\n",
	EscComment, job->common->info[0], job->common->info[1], job->common->info[2]);
    gvprintf(job, "%s Title: %s\n", EscComment, agnameof(obj->u.g));
    gvprintf(job,
            "%s save point size and font\n.nr .S \\n(.s\n.nr DF \\n(.f\n",
            EscComment);
}

static void pic_end_graph(GVJ_t * job)
{
    gvprintf(job,
            "%s restore point size and font\n.ps \\n(.S\n.ft \\n(DF\n",
            EscComment);
}

static void pic_begin_page(GVJ_t * job)
{
    box pbr = job->pageBoundingBox;
    double height, width;

    if (onetime && job->rotation && (job->rotation != 90)) {
        unsupported("rotation");
        onetime = FALSE;
    }
    height = PS2INCH((double) (pbr.UR.y) - (double) (pbr.LL.y));
    width = PS2INCH((double) (pbr.UR.x) - (double) (pbr.LL.x));
    if (job->rotation == 90) {
        double temp = width;
        width = height;
        height = temp;
    }
    gvprintf(job, ".PS %.5f %.5f\n", width, height);
    gvprintf(job,
            "%s to change drawing size, multiply the width and height on the .PS line above and the number on the two lines below (rounded to the nearest integer) by a scale factor\n",
            EscComment);
    if (width > 0.0) {
        Fontscale = log10(width);
        Fontscale += 3.0 - (int) Fontscale;     /* between 3.0 and 4.0 */
    } else
        Fontscale = 3.0;
    Fontscale = pow(10.0, Fontscale);   /* a power of 10 times width, between 1000 and 10000 */
    gvprintf(job, ".nr SF %.0f\nscalethickness = %.0f\n", Fontscale,
            Fontscale);
    gvprintf(job,
            "%s don't change anything below this line in this drawing\n",
            EscComment);
    gvprintf(job,
            "%s non-fatal run-time pic version determination, version 2\n",
            EscComment);
    gvprintf(job,
            "boxrad=2.0 %s will be reset to 0.0 by gpic only\n",
            EscComment);
    gvprintf(job, "scale=1.0 %s required for comparisons\n",
            EscComment);
    gvprintf(job,
            "%s boxrad is now 0.0 in gpic, else it remains 2.0\n",
            EscComment);
    gvprintf(job,
            "%s dashwid is 0.1 in 10th Edition, 0.05 in DWB 2 and in gpic\n",
            EscComment);
    gvprintf(job,
            "%s fillval is 0.3 in 10th Edition (fill 0 means black), 0.5 in gpic (fill 0 means white), undefined in DWB 2\n",
            EscComment);
    gvprintf(job,
            "%s fill has no meaning in DWB 2, gpic can use fill or filled, 10th Edition uses fill only\n",
            EscComment);
    gvprintf(job,
            "%s DWB 2 doesn't use fill and doesn't define fillval\n",
            EscComment);
    gvprintf(job,
            "%s reset works in gpic and 10th edition, but isn't defined in DWB 2\n",
            EscComment);
    gvprintf(job, "%s DWB 2 compatibility definitions\n",
            EscComment);
    gvprintf(job,
            "if boxrad > 1.0 && dashwid < 0.075 then X\n\tfillval = 1;\n\tdefine fill Y Y;\n\tdefine solid Y Y;\n\tdefine reset Y scale=1.0 Y;\nX\n");
    gvprintf(job, "reset %s set to known state\n", EscComment);
    gvprintf(job, "%s GNU pic vs. 10th Edition d\\(e'tente\n",
            EscComment);
    gvprintf(job,
            "if fillval > 0.4 then X\n\tdefine setfillval Y fillval = 1 - Y;\n\tdefine bold Y thickness 2 Y;\n");
    gvprintf(job,
            "\t%s if you use gpic and it barfs on encountering \"solid\",\n",
            EscComment);
    gvprintf(job,
            "\t%s\tinstall a more recent version of gpic or switch to DWB or 10th Edition pic;\n",
            EscComment);
    gvprintf(job,
            "\t%s\tsorry, the groff folks changed gpic; send any complaint to them;\n",
            EscComment);
    gvprintf(job,
            "X else Z\n\tdefine setfillval Y fillval = Y;\n\tdefine bold Y Y;\n\tdefine filled Y fill Y;\nZ\n");
    gvprintf(job,
            "%s arrowhead has no meaning in DWB 2, arrowhead = 7 makes filled arrowheads in gpic and in 10th Edition\n",
            EscComment);
    gvprintf(job,
            "%s arrowhead is undefined in DWB 2, initially 1 in gpic, 2 in 10th Edition\n",
            EscComment);
    gvprintf(job, "arrowhead = 7 %s not used by graphviz\n",
            EscComment);
    gvprintf(job,
            "%s GNU pic supports a boxrad variable to draw boxes with rounded corners; DWB and 10th Ed. do not\n",
            EscComment);
    gvprintf(job, "boxrad = 0 %s no rounded corners in graphviz\n",
            EscComment);
    gvprintf(job,
            "%s GNU pic supports a linethick variable to set line thickness; DWB and 10th Ed. do not\n",
            EscComment);
    gvprintf(job, "linethick = 0; oldlinethick = linethick\n");
    gvprintf(job,
            "%s .PS w/o args causes GNU pic to scale drawing to fit 8.5x11 paper; DWB does not\n",
            EscComment);
    gvprintf(job,
            "%s maxpsht and maxpswid have no meaning in DWB 2.0, set page boundaries in gpic and in 10th Edition\n",
            EscComment);
    gvprintf(job,
            "%s maxpsht and maxpswid are predefined to 11.0 and 8.5 in gpic\n",
            EscComment);
    gvprintf(job, "maxpsht = %f\nmaxpswid = %f\n", height, width);
    gvprintf(job, "Dot: [\n");
    gvprintf(job,
            "define attrs0 %% %%; define unfilled %% %%; define rounded %% %%; define diagonals %% %%\n");
}

static void pic_end_page(GVJ_t * job)
{
    gvprintf(job,
	"]\n.PE\n");
}

static void pic_textpara(GVJ_t * job, pointf p, textpara_t * para)
{
    static char *lastname;
    static int lastsize;
    int sz;

    switch (para->just) {
    case 'l': 
        break;
    case 'r': 
        p.x -= para->width;
        break;
    default:
    case 'n': 
        p.x -= para->width / 2;
        break;
    }
#ifdef NOTDEF
    /* Why on earth would we want this? SCN  11/29/2001 */
    p.y -= para->fontsize / (5.0 * POINTS_PER_INCH);
#endif
    /* Why on earth would we do this either. But it works. SCN 2/26/2002 */
    p.y += para->fontsize / (3.0 * POINTS_PER_INCH);
    p.x += para->width / (2.0 * POINTS_PER_INCH);

    if (para->fontname && (!(lastname) || strcmp(lastname, para->fontname))) {
        gvprintf(job, ".ft %s\n", picfontname(para->fontname));
	lastname = para->fontname;
    }
    if ((sz = (int)para->fontsize) < 1);
        sz = 1;
    if (sz != lastsize) {
        gvprintf(job, ".ps %d*\\n(SFu/%.0fu\n", sz, Fontscale);
	lastsize = sz;
    }
    gvprintf(job, "\"%s\" at (%.5f,%.5f);\n",
            pic_string(para->str), p.x, p.y);
}

static void pic_ellipse(GVJ_t * job, pointf * A, int filled)
{
    /* A[] contains 2 points: the center and corner. */

    gvprintf(job,
		"ellipse attrs%d %swid %.5f ht %.5f at (%.5f,%.5f);\n", 1,
		filled ? "fill " : "",
		PS2INCH(2*(A[1].x - A[0].x)),
		PS2INCH(2*(A[1].y - A[0].y)),
		PS2INCH(A[0].x),
		PS2INCH(A[0].y));
}

static void pic_bezier(GVJ_t * job, pointf * A, int n, int arrow_at_start,
//            start_y, end_x, end_y);
	      int arrow_at_end, int filled)
{
    obj_state_t *obj = job->obj;

    int object_code = 3;        /* always 3 for spline */
    int sub_type;
    int line_style;		/* solid, dotted, dashed */
    int thickness = obj->penwidth;
    int pen_color = obj->pencolor.u.index;
    int fill_color = obj->fillcolor.u.index;
    int pen_style = 0;          /* not used */
    int area_fill;
    double style_val;
    int cap_style = 0;
    int forward_arrow = 0;
    int backward_arrow = 0;
    int npoints = n;
    int i;

    pointf pf, V[4];
    point p;
    int j, step;
    int count = 0;
    int size;

    char *buffer;
    char *buf;
    buffer =
        malloc((npoints + 1) * (BEZIERSUBDIVISION +
                                1) * 20 * sizeof(char));
    buf = buffer;

    pic_line_style(obj, &line_style, &style_val);

    if (filled) {
        sub_type = 5;     /* closed X-spline */
        area_fill = 20;   /* fully saturated color */
        fill_color = job->obj->fillcolor.u.index;
    }
    else {
        sub_type = 4;     /* opened X-spline */
        area_fill = -1;
        fill_color = 0;
    }
    V[3].x = A[0].x;
    V[3].y = A[0].y;
    /* Write first point in line */
    count++;
    PF2P(A[0], p);
    size = sprintf(buf, " %d %d", p.x, p.y);
    buf += size;
    /* write subsequent points */
    for (i = 0; i + 3 < n; i += 3) {
        V[0] = V[3];
        for (j = 1; j <= 3; j++) {
            V[j].x = A[i + j].x;
            V[j].y = A[i + j].y;
        }
        for (step = 1; step <= BEZIERSUBDIVISION; step++) {
            count++;
            pf = Bezier (V, 3, (double) step / BEZIERSUBDIVISION, NULL, NULL);
	    PF2P(pf, p);
            size = sprintf(buf, " %d %d", p.x, p.y);
            buf += size;
        }
    }

//    gvprintf(job, "%d %d %d %d %d %d %d %d %d %.1f %d %d %d %d\n",
//            object_code,
//            sub_type,
//            line_style,
//            thickness,
//            pen_color,
//            fill_color,
//            depth,
//            pen_style,
//            area_fill,
//            style_val, cap_style, forward_arrow, backward_arrow, count);

    gvprintf(job, " %s\n", buffer);      /* print points */
    free(buffer);
    for (i = 0; i < count; i++) {
        gvprintf(job, " %d", i % (count - 1) ? 1 : 0);   /* -1 on all */
    }
    gvputs(job, "\n");
}

static void pic_polygon(GVJ_t * job, pointf * A, int n, int filled)
{
    obj_state_t *obj = job->obj;

    int object_code = 2;        /* always 2 for polyline */
    int sub_type = 3;           /* always 3 for polygon */
    int line_style;		/* solid, dotted, dashed */
    int thickness = obj->penwidth;
    int pen_color = obj->pencolor.u.index;
    int fill_color = obj->fillcolor.u.index;
    int pen_style = 0;          /* not used */
    int area_fill = filled ? 20 : -1;
    double style_val;
    int join_style = 0;
    int cap_style = 0;
    int radius = 0;
    int forward_arrow = 0;
    int backward_arrow = 0;
    int npoints = n + 1;

    pic_line_style(obj, &line_style, &style_val);

//    gvprintf(job,
//            "%d %d %d %d %d %d %d %d %d %.1f %d %d %d %d %d %d\n",
//            object_code, sub_type, line_style, thickness, pen_color,
//            fill_color, depth, pen_style, area_fill, style_val, join_style,
//            cap_style, radius, forward_arrow, backward_arrow, npoints);
    picptarray(job, A, n, 1);        /* closed shape */
}

static void pic_polyline(GVJ_t * job, pointf * A, int n)
{
    obj_state_t *obj = job->obj;

    int object_code = 2;        /* always 2 for polyline */
    int sub_type = 1;           /* always 1 for polyline */
    int line_style;		/* solid, dotted, dashed */
    int thickness = obj->penwidth;
    int pen_color = obj->pencolor.u.index;
    int fill_color = 0;
    int pen_style = 0;          /* not used */
    int area_fill = 0;
    double style_val;
    int join_style = 0;
    int cap_style = 0;
    int radius = 0;
    int forward_arrow = 0;
    int backward_arrow = 0;
    int npoints = n;

    pic_line_style(obj, &line_style, &style_val);

//    gvprintf(job,
//            "%d %d %d %d %d %d %d %d %d %.1f %d %d %d %d %d %d\n",
//            object_code, sub_type, line_style, thickness, pen_color,
//            fill_color, depth, pen_style, area_fill, style_val, join_style,
//            cap_style, radius, forward_arrow, backward_arrow, npoints);
    picptarray(job, A, n, 0);        /* open shape */
}

gvrender_engine_t pic_engine = {
    0,				/* pic_begin_job */
    0,				/* pic_end_job */
    pic_begin_graph,
    pic_end_graph,
    0,				/* pic_begin_layer */
    0,				/* pic_end_layer */
    pic_begin_page,
    pic_end_page,
    0,				/* pic_begin_cluster */
    0,				/* pic_end_cluster */
    0,				/* pic_begin_nodes */
    0,				/* pic_end_nodes */
    0,				/* pic_begin_edges */
    0,				/* pic_end_edges */
    0,				/* pic_begin_node */
    0,				/* pic_end_node */
    0,				/* pic_begin_edge */
    0,				/* pic_end_edge */
    0,				/* pic_begin_anchor */
    0,				/* pic_end_anchor */
    0,				/* pic_begin_label */
    0,				/* pic_end_label */
    pic_textpara,
    0,				/* pic_resolve_color */
    pic_ellipse,
    pic_polygon,
    pic_bezier,
    pic_polyline,
    pic_comment,
    0,				/* pic_library_shape */
};


static gvrender_features_t render_features_pic = {
    0,				/* flags */
    4.,                         /* default pad - graph units */
    NULL,			/* knowncolors */
    0,				/* sizeof knowncolors */
    HSVA_DOUBLE,		/* color_type */
};

static gvdevice_features_t device_features_pic = {
    0,				/* flags */
    {0.,0.},			/* default margin - points */
    {0.,0.},			/* default page width, height - points */
    {72.,72.},			/* default dpi */
};

gvplugin_installed_t gvrender_pic_types[] = {
    {FORMAT_PIC, "pic", -1, &pic_engine, &render_features_pic},
    {0, NULL, 0, NULL, NULL}
};

gvplugin_installed_t gvdevice_pic_types[] = {
    {FORMAT_PIC, "pic:pic", -1, NULL, &device_features_pic},
    {0, NULL, 0, NULL, NULL}
};
