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

#ifdef WIN32
#include <io.h>
#endif
#include <stdlib.h>
#include <stddef.h>
#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif
#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#endif
#include <string.h>
#include <fcntl.h>

#include "gvplugin_render.h"
#include "gvplugin_device.h"
#include "gvcint.h"	/* for gvc->g for agget */

#ifdef HAVE_LIBGD
#include "gd.h"

#ifndef INT32_MAX
#define INT32_MAX              (2147483647)
#endif
#ifndef UINT32_MAX
#define UINT32_MAX             (4294967295U)
#endif


typedef enum {
	FORMAT_GIF,
	FORMAT_JPEG,
	FORMAT_PNG,
	FORMAT_WBMP,
	FORMAT_GD,
	FORMAT_GD2,
	FORMAT_XBM,
} format_type;

extern boolean mapbool(char *);
extern pointf Bezier(pointf * V, int degree, double t, pointf * Left, pointf * Right);

#define BEZIERSUBDIVISION 10

static void gdgen_resolve_color(GVJ_t * job, gvcolor_t * color)
{
    gdImagePtr im = (gdImagePtr) job->context;
    int alpha;

    if (!im)
	return;

    /* convert alpha (normally an "opacity" value) to gd's "transparency" */
    alpha = (255 - color->u.rgba[3]) * gdAlphaMax / 255;

    if(alpha == gdAlphaMax)
	color->u.index = gdImageGetTransparent(im);
    else
        color->u.index = gdImageColorResolveAlpha(im,
			  color->u.rgba[0],
			  color->u.rgba[1],
			  color->u.rgba[2],
			  alpha);
    color->type = COLOR_INDEX;
}

static int white, black, transparent, basecolor;

#define GD_XYMAX INT32_MAX

static void gdgen_begin_page(GVJ_t * job)
{
    char *bgcolor_str = NULL, *truecolor_str = NULL;
    boolean truecolor_p = FALSE;	/* try to use cheaper paletted mode */
    boolean bg_transparent_p = FALSE;
    gdImagePtr im = NULL;

    truecolor_str = agget((graph_t*)(job->gvc->g), "truecolor");	/* allow user to force truecolor */
    bgcolor_str = agget((graph_t*)(job->gvc->g), "bgcolor");

    if (truecolor_str && truecolor_str[0])
	truecolor_p = mapbool(truecolor_str);

    if (bgcolor_str && strcmp(bgcolor_str, "transparent") == 0) {
	bg_transparent_p = TRUE;
	if (job->render.features->flags & GVDEVICE_DOES_TRUECOLOR)
	    truecolor_p = TRUE;	/* force truecolor */
    }

    if (GD_has_images(job->gvc->g))
	truecolor_p = TRUE;	/* force truecolor */

    if (job->external_context) {
	if (job->common->verbose)
	    fprintf(stderr, "%s: using existing GD image\n", job->common->cmdname);
	im = (gdImagePtr) (job->context);
    } else {
        if (job->width * job->height >= GD_XYMAX) {
	    double scale = sqrt(GD_XYMAX / (job->width * job->height));
	    job->width *= scale;
	    job->height *= scale;
	    job->zoom *= scale;
	    fprintf(stderr,
		"%s: graph is too large for gd-renderer bitmaps. Scaling by %g to fit\n",
		job->common->cmdname, scale);
	}
	if (truecolor_p) {
	    if (job->common->verbose)
		fprintf(stderr,
			"%s: allocating a %dK TrueColor GD image (%d x %d pixels)\n",
			job->common->cmdname,
			ROUND(job->width * job->height * 4 / 1024.),
			job->width, job->height);
	    im = gdImageCreateTrueColor(job->width, job->height);
	} else {
	    if (job->common->verbose)
		fprintf(stderr,
			"%s: allocating a %dK PaletteColor GD image (%d x %d pixels)\n",
			job->common->cmdname,
			ROUND(job->width * job->height / 1024.),
			job->width, job->height);
	    im = gdImageCreate(job->width, job->height);
	}
        job->context = (void *) im;
    }

    if (!im) {
	job->common->errorfn("gdImageCreate returned NULL. Malloc problem?\n");
	return;
    }

    /* first color is the default background color */
    /*   - used for margins - if any */
    transparent = gdImageColorResolveAlpha(im,
					   gdRedMax - 1, gdGreenMax,
					   gdBlueMax, gdAlphaTransparent);
    gdImageColorTransparent(im, transparent);

    white = gdImageColorResolveAlpha(im,
				     gdRedMax, gdGreenMax, gdBlueMax,
				     gdAlphaOpaque);

    black = gdImageColorResolveAlpha(im, 0, 0, 0, gdAlphaOpaque);

    /* Blending must be off to lay a transparent basecolor.
       Nothing to blend with anyway. */
    gdImageAlphaBlending(im, FALSE);
    gdImageFill(im, im->sx / 2, im->sy / 2, transparent);
    /* Blend everything else together,
       especially fonts over non-transparent backgrounds */
    gdImageAlphaBlending(im, TRUE);
}

extern int gvdevice_gd_putBuf (gdIOCtx *context, const void *buffer, int len);
extern void gvdevice_gd_putC (gdIOCtx *context, int C);

static void gdgen_end_page(GVJ_t * job)
{
    gdImagePtr im = (gdImagePtr) job->context;

    gdIOCtx ctx;

    ctx.putBuf = gvdevice_gd_putBuf;
    ctx.putC = gvdevice_gd_putC;
    ctx.tell = (void*)job;    /* hide *job here */

    if (!im)
	return;
    if (job->external_context) {
	/* leave image in memory to be handled by Gdtclft output routines */
#ifdef MYTRACE
	fprintf(stderr, "gdgen_end_graph (to memory)\n");
#endif
    } else {
	/* Only save the alpha channel in outputs that support it if
	   the base color was transparent.   Otherwise everything
	   was blended so there is no useful alpha info */
	gdImageSaveAlpha(im, (basecolor == transparent));
	switch (job->render.id) {
	case FORMAT_GIF:
#ifdef HAVE_GD_GIF
	    gdImageTrueColorToPalette(im, 0, 256);
	    gdImageGifCtx(im, &ctx);
#endif
	    break;
	case FORMAT_JPEG:
#ifdef HAVE_GD_JPEG
	    /*
	     * Write IM to OUTFILE as a JFIF-formatted JPEG image, using
	     * quality JPEG_QUALITY.  If JPEG_QUALITY is in the range
	     * 0-100, increasing values represent higher quality but also
	     * larger image size.  If JPEG_QUALITY is negative, the
	     * IJG JPEG library's default quality is used (which should
	     * be near optimal for many applications).  See the IJG JPEG
	     * library documentation for more details.  */
#define JPEG_QUALITY -1
	    gdImageJpegCtx(im, &ctx, JPEG_QUALITY);
#endif

	    break;
	case FORMAT_PNG:
#ifdef HAVE_GD_PNG
	    gdImagePngCtx(im, &ctx);
#endif
	    break;

#ifdef HAVE_GD_GIF
	case FORMAT_WBMP:
	    {
	        /* Use black for the foreground color for the B&W wbmp image. */
		int black = gdImageColorResolveAlpha(im, 0, 0, 0, gdAlphaOpaque);
		gdImageWBMPCtx(im, black, &ctx);
	    }
	    break;
#endif

	case FORMAT_GD:
	    gdImageGd(im, job->output_file);
	    break;

#ifdef HAVE_LIBZ
	case FORMAT_GD2:
#define GD2_CHUNKSIZE 128
#define GD2_RAW 1
#define GD2_COMPRESSED 2
	    gdImageGd2(im, job->output_file, GD2_CHUNKSIZE, GD2_COMPRESSED);
	    break;
#endif

	case FORMAT_XBM:
#if 0
/* libgd support only reading .xpm files */
#ifdef HAVE_GD_XPM
	    gdImageXbm(im, job->output_file);
#endif
#endif
	    break;
	}
	gdImageDestroy(im);
#ifdef MYTRACE
	fprintf(stderr, "gdgen_end_graph (to file)\n");
#endif
	job->context = NULL;
    }
}

static void gdgen_missingfont(char *err, char *fontreq)
{
    static char *lastmissing = 0;
    static int n_errors = 0;

    if (n_errors >= 20)
	return;
    if ((lastmissing == 0) || (strcmp(lastmissing, fontreq))) {
#ifdef HAVE_GD_FONTCONFIG
#if 0
/* FIXME - error function */
	agerr(AGERR, "%s : %s\n", err, fontreq);
#endif
#else
	char *p = getenv("GDFONTPATH");
	if (!p)
	    p = DEFAULT_FONTPATH;
#if 0
/* FIXME - error function */
	agerr(AGERR, "%s : %s in %s\n", err, fontreq, p);
#endif
#endif
	if (lastmissing)
	    free(lastmissing);
	lastmissing = strdup(fontreq);
	n_errors++;
#if 0
/* FIXME - error function */
	if (n_errors >= 20)
	    agerr(AGWARN, "(font errors suppressed)\n");
#endif
    }
}

extern gdFontPtr gdFontTiny, gdFontSmall, gdFontMediumBold, gdFontLarge, gdFontGiant;

/* fontsize at which text is omitted entirely */
#define FONTSIZE_MUCH_TOO_SMALL 0.15
/* fontsize at which text is rendered by a simple line */
#define FONTSIZE_TOO_SMALL 1.5

extern gdFontPtr gdFontTiny, gdFontSmall, gdFontMediumBold, gdFontLarge, gdFontGiant;

void gdgen_text(gdImagePtr im, pointf spf, pointf epf, int fontcolor, double fontsize, int fontdpi, double fontangle, char *fontname, char *str)
{
    gdFTStringExtra strex;
    point sp, ep; /* start point, end point, in pixels */

    PF2P(spf, sp);
    PF2P(epf, ep);

    strex.flags = gdFTEX_RESOLUTION;
    strex.hdpi = strex.vdpi = fontdpi;

    if (strstr(fontname, "/"))
        strex.flags |= gdFTEX_FONTPATHNAME;
    else
        strex.flags |= gdFTEX_FONTCONFIG;

    if (fontsize <= FONTSIZE_MUCH_TOO_SMALL) {
        /* ignore entirely */
    } else if (fontsize <= FONTSIZE_TOO_SMALL) {
        /* draw line in place of text */
        gdImageLine(im, sp.x, sp.y, ep.x, ep.y, fontcolor);
    } else {
#ifdef HAVE_GD_FREETYPE
        char *err;
        int brect[8];
#ifdef HAVE_GD_FONTCONFIG
        char* fontlist = fontname;
#else
        extern char *gd_alternate_fontlist(char *font);
        char* fontlist = gd_alternate_fontlist(fontname);
#endif
        err = gdImageStringFTEx(im, brect, fontcolor,
                fontlist, fontsize, fontangle, sp.x, sp.y, str, &strex);

        if (err) {
            /* revert to builtin fonts */
            gdgen_missingfont(err, fontname);
#endif
            sp.y += 2;
            if (fontsize <= 8.5) {
                gdImageString(im, gdFontTiny, sp.x, sp.y - 9, (unsigned char*)str, fontcolor);
            } else if (fontsize <= 9.5) {
                gdImageString(im, gdFontSmall, sp.x, sp.y - 12, (unsigned char*)str, fontcolor);
            } else if (fontsize <= 10.5) {
                gdImageString(im, gdFontMediumBold, sp.x, sp.y - 13, (unsigned char*)str, fontcolor);
            } else if (fontsize <= 11.5) {
                gdImageString(im, gdFontLarge, sp.x, sp.y - 14, (unsigned char*)str, fontcolor);
            } else {
                gdImageString(im, gdFontGiant, sp.x, sp.y - 15, (unsigned char*)str, fontcolor);
            }
#ifdef HAVE_GD_FREETYPE
        }
#endif
    }
}

extern char* gd_psfontResolve (PostscriptAlias* pa);

static void gdgen_textpara(GVJ_t * job, pointf p, textpara_t * para)
{
    gdImagePtr im = (gdImagePtr) job->context;
    pointf spf, epf;
    double parawidth = para->width * job->zoom * job->dpi.x / POINTS_PER_INCH;
    char* fontname;

    if (!im)
	return;

    switch (para->just) {
    case 'l':
	spf.x = 0.0;
	break;
    case 'r':
	spf.x = -parawidth;
	break;
    default:
    case 'n':
	spf.x = -parawidth / 2;
	break;
    }
    epf.x = spf.x + parawidth;

    if (job->rotation) {
	spf.y = -spf.x + p.y;
	epf.y = epf.x + p.y;
	epf.x = spf.x = p.x;
    }
    else {
	spf.x += p.x;
	epf.x += p.x;
	epf.y = spf.y = p.y - para->yoffset_centerline * job->zoom * job->dpi.x / POINTS_PER_INCH;
    }

#ifdef HAVE_GD_FONTCONFIG
    if (para->postscript_alias)
	fontname = gd_psfontResolve (para->postscript_alias);
    else
#endif
	fontname = para->fontname;

    gdgen_text(im, spf, epf,
	    job->obj->pencolor.u.index,
	    para->fontsize * job->zoom,
	    job->dpi.x,
	    job->rotation ? (M_PI / 2) : 0,
	    fontname,
	    para->str);
}

static int gdgen_set_penstyle(GVJ_t * job, gdImagePtr im, gdImagePtr brush)
{
    obj_state_t *obj = job->obj;
    int i, pen, width, dashstyle[40];

    if (obj->pen == PEN_DASHED) {
	for (i = 0; i < 10; i++)
	    dashstyle[i] = obj->pencolor.u.index;
	for (; i < 20; i++)
	    dashstyle[i] = gdTransparent;
	gdImageSetStyle(im, dashstyle, 20);
	pen = gdStyled;
    } else if (obj->pen == PEN_DOTTED) {
	for (i = 0; i < 2; i++)
	    dashstyle[i] = obj->pencolor.u.index;
	for (; i < 14; i++)
	    dashstyle[i] = gdTransparent;
	gdImageSetStyle(im, dashstyle, 12);
	pen = gdStyled;
    } else {
	pen = obj->pencolor.u.index;
    }

    width = obj->penwidth * job->zoom;
    if (width < PENWIDTH_NORMAL)
	width = PENWIDTH_NORMAL;  /* gd can't do thin lines */
    gdImageSetThickness(im, width);
    /* use brush instead of Thickness to improve end butts */
    if (width != PENWIDTH_NORMAL) {
	if (im->trueColor) {
	    brush = gdImageCreateTrueColor(width,width);
	}
	else {
	    brush = gdImageCreate(width, width);
	    gdImagePaletteCopy(brush, im);
	}
	gdImageFilledRectangle(brush, 0, 0, width - 1, width - 1,
			       obj->pencolor.u.index);
	gdImageSetBrush(im, brush);
	if (pen == gdStyled)
	    pen = gdStyledBrushed;
	else
	    pen = gdBrushed;
    }

    return pen;
}

static void
gdgen_bezier(GVJ_t * job, pointf * A, int n, int arrow_at_start,
	     int arrow_at_end, int filled)
{
    obj_state_t *obj = job->obj;
    gdImagePtr im = (gdImagePtr) job->context;
    pointf p0, p1, V[4];
    int i, j, step, pen;
    boolean pen_ok, fill_ok;
    gdImagePtr brush = NULL;
    gdPoint F[4];

    if (!im)
	return;

    pen = gdgen_set_penstyle(job, im, brush);
    pen_ok = (pen != gdImageGetTransparent(im));
    fill_ok = (filled && obj->fillcolor.u.index != gdImageGetTransparent(im));

    if (pen_ok || fill_ok) {
        V[3] = A[0];
        PF2P(A[0], F[0]);
        PF2P(A[n-1], F[3]);
        for (i = 0; i + 3 < n; i += 3) {
	    V[0] = V[3];
	    for (j = 1; j <= 3; j++)
	        V[j] = A[i + j];
	    p0 = V[0];
	    for (step = 1; step <= BEZIERSUBDIVISION; step++) {
	        p1 = Bezier(V, 3, (double) step / BEZIERSUBDIVISION, NULL, NULL);
	        PF2P(p0, F[1]);
	        PF2P(p1, F[2]);
	        if (pen_ok)
	            gdImageLine(im, F[1].x, F[1].y, F[2].x, F[2].y, pen);
	        if (fill_ok)
		    gdImageFilledPolygon(im, F, 4, obj->fillcolor.u.index);
	        p0 = p1;
	    }
        }
    }
    if (brush)
	gdImageDestroy(brush);
}

static gdPoint *points;
static int points_allocated;

static void gdgen_polygon(GVJ_t * job, pointf * A, int n, int filled)
{
    obj_state_t *obj = job->obj;
    gdImagePtr im = (gdImagePtr) job->context;
    gdImagePtr brush = NULL;
    int i;
    int pen;
    boolean pen_ok, fill_ok;

    if (!im)
	return;

    pen = gdgen_set_penstyle(job, im, brush);
    pen_ok = (pen != gdImageGetTransparent(im));
    fill_ok = (filled && obj->fillcolor.u.index != gdImageGetTransparent(im));

    if (pen_ok || fill_ok) {
        if (n > points_allocated) {
	    points = realloc(points, n * sizeof(gdPoint));
	    points_allocated = n;
	}
        for (i = 0; i < n; i++) {
	    points[i].x = ROUND(A[i].x);
	    points[i].y = ROUND(A[i].y);
        }
        if (fill_ok)
	    gdImageFilledPolygon(im, points, n, obj->fillcolor.u.index);
    
        if (pen_ok)
            gdImagePolygon(im, points, n, pen);
    }
    if (brush)
	gdImageDestroy(brush);
}

static void gdgen_ellipse(GVJ_t * job, pointf * A, int filled)
{
    obj_state_t *obj = job->obj;
    gdImagePtr im = (gdImagePtr) job->context;
    double dx, dy;
    int pen;
    boolean pen_ok, fill_ok;
    gdImagePtr brush = NULL;

    if (!im)
	return;

    pen = gdgen_set_penstyle(job, im, brush);
    pen_ok = (pen != gdImageGetTransparent(im));
    fill_ok = (filled && obj->fillcolor.u.index != gdImageGetTransparent(im));

    dx = 2 * (A[1].x - A[0].x);
    dy = 2 * (A[1].y - A[0].y);

    if (fill_ok)
	gdImageFilledEllipse(im, ROUND(A[0].x), ROUND(A[0].y),
			     ROUND(dx), ROUND(dy),
			     obj->fillcolor.u.index);
    if (pen_ok)
        gdImageArc(im, ROUND(A[0].x), ROUND(A[0].y), ROUND(dx), ROUND(dy),
	           0, 360, pen);
    if (brush)
	gdImageDestroy(brush);
}

static void gdgen_polyline(GVJ_t * job, pointf * A, int n)
{
    gdImagePtr im = (gdImagePtr) job->context;
    pointf p, p1;
    int i;
    int pen;
    boolean pen_ok;
    gdImagePtr brush = NULL;

    if (!im)
	return;

    pen = gdgen_set_penstyle(job, im, brush);
    pen_ok = (pen != gdImageGetTransparent(im));

    if (pen_ok) {
        p = A[0];
        for (i = 1; i < n; i++) {
	    p1 = A[i];
	    gdImageLine(im, ROUND(p.x), ROUND(p.y),
		        ROUND(p1.x), ROUND(p1.y), pen);
	    p = p1;
        }
    }
    if (brush)
	gdImageDestroy(brush);
}

static gvrender_engine_t gdgen_engine = {
    0,				/* gdgen_begin_job */
    0,				/* gdgen_end_job */
    0,				/* gdgen_begin_graph */
    0,				/* gdgen_end_graph */
    0,				/* gdgen_begin_layer */
    0,				/* gdgen_end_layer */
    gdgen_begin_page,
    gdgen_end_page,
    0,				/* gdgen_begin_cluster */
    0,				/* gdgen_end_cluster */
    0,				/* gdgen_begin_nodes */
    0,				/* gdgen_end_nodes */
    0,				/* gdgen_begin_edges */
    0,				/* gdgen_end_edges */
    0,				/* gdgen_begin_node */
    0,				/* gdgen_end_node */
    0,				/* gdgen_begin_edge */
    0,				/* gdgen_end_edge */
    0,				/* gdgen_begin_anchor */
    0,				/* gdgen_end_anchor */
    0,				/* gdgen_begin_label */
    0,				/* gdgen_end_label */
    gdgen_textpara,
    gdgen_resolve_color,
    gdgen_ellipse,
    gdgen_polygon,
    gdgen_bezier,
    gdgen_polyline,
    0,				/* gdgen_comment */
    0,				/* gdgen_library_shape */
};

static gvrender_features_t render_features_gd = {
    GVRENDER_Y_GOES_DOWN,	/* flags */
    4.,                         /* default pad - graph units */
    NULL,			/* knowncolors */
    0,				/* sizeof knowncolors */
    RGBA_BYTE,			/* color_type */
};

static gvdevice_features_t device_features_gd = {
    GVDEVICE_BINARY_FORMAT,	/* flags */
    {0.,0.},			/* default margin - points */
    {0.,0.},                    /* default page width, height - points */
    {96.,96.},			/* default dpi */
};

static gvdevice_features_t device_features_gd_tc = {
    GVDEVICE_BINARY_FORMAT
      | GVDEVICE_DOES_TRUECOLOR,/* flags */
    {0.,0.},			/* default margin - points */
    {0.,0.},                    /* default page width, height - points */
    {96.,96.},			/* default dpi */
};

static gvdevice_features_t device_features_gd_tc_no_writer = {
    GVDEVICE_BINARY_FORMAT
      | GVDEVICE_DOES_TRUECOLOR
      | GVDEVICE_NO_WRITER,	/* flags */
    {0.,0.},			/* default margin - points */
    {0.,0.},                    /* default page width, height - points */
    {96.,96.},			/* default dpi */
};

#endif

gvplugin_installed_t gvrender_gd_types[] = {
#ifdef HAVE_LIBGD
    {FORMAT_GD, "gd", 1, &gdgen_engine, &render_features_gd},
#endif
    {0, NULL, 0, NULL, NULL}
};

gvplugin_installed_t gvdevice_gd_types2[] = {
#ifdef HAVE_LIBGD
#ifdef HAVE_GD_GIF
    {FORMAT_GIF, "gif:gd", 1, NULL, &device_features_gd_tc},  /* pretend gif is truecolor because it supports transparency */
    {FORMAT_WBMP, "wbmp:gd", 1, NULL, &device_features_gd},
#endif

#ifdef HAVE_GD_JPEG
    {FORMAT_JPEG, "jpe:gd", 1, NULL, &device_features_gd},
    {FORMAT_JPEG, "jpeg:gd", 1, NULL, &device_features_gd},
    {FORMAT_JPEG, "jpg:gd", 1, NULL, &device_features_gd},
#endif

#ifdef HAVE_GD_PNG
    {FORMAT_PNG, "png:gd", 1, NULL, &device_features_gd_tc},
#endif

    {FORMAT_GD, "gd:gd", 1, NULL, &device_features_gd_tc_no_writer},
    
#ifdef HAVE_LIBZ
    {FORMAT_GD2, "gd2:gd", 1, NULL, &device_features_gd_tc_no_writer},
#endif

#if 0
/* libgd has no support for xbm as output */
#ifdef HAVE_GD_XPM
    {FORMAT_XBM, "xbm:gd", 1, NULL, &device_features_gd},
#endif
#endif

#endif
    {0, NULL, 0, NULL, NULL}
};
