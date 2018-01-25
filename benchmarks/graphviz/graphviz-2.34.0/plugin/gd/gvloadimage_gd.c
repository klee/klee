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
#include <stddef.h>
#include <string.h>

#ifdef HAVE_PANGOCAIRO
#include <cairo.h>
#endif

#include "gvplugin_loadimage.h"
#include "gvio.h"

#ifdef HAVE_LIBGD
#include "gd.h"

typedef enum {
    FORMAT_PNG_GD, FORMAT_GIF_GD, FORMAT_JPG_GD, FORMAT_GD_GD, FORMAT_GD2_GD, FORMAT_XPM_GD, FORMAT_WBMP_GD, FORMAT_XBM_GD,
    FORMAT_PNG_PS, FORMAT_GIF_PS, FORMAT_JPG_PS, FORMAT_GD_PS, FORMAT_GD2_PS, FORMAT_XPM_PS, FORMAT_WBMP_PS, FORMAT_XBM_PS,
    FORMAT_PNG_CAIRO, FORMAT_GIF_CAIRO, FORMAT_JPG_CAIRO, FORMAT_GD_CAIRO, FORMAT_GD2_CAIRO, FORMAT_XPM_CAIRO, FORMAT_WBMP_CAIRO, FORMAT_XBM_CAIRO,
} format_type;


static void gd_freeimage(usershape_t *us)
{
    gdImageDestroy((gdImagePtr)us->data);
}

static gdImagePtr gd_loadimage(GVJ_t * job, usershape_t *us)
{
    assert(job);
    assert(us);
    assert(us->name);

    if (us->data) {
	if (us->datafree != gd_freeimage) {
	     us->datafree(us);        /* free incompatible cache data */
	     us->data = NULL;
	     us->datafree = NULL;
	}
    }
    if (!us->data) { /* read file into cache */
	if (!gvusershape_file_access(us))
	    return NULL;
	switch (us->type) {
#if 0
	    case FT_GD:
		im = gdImageCreateFromGd(us->f);
		break;
	    case FT_GD2:
		im = gdImageCreateFromGd2(us->f);
		break;
#endif
#ifdef HAVE_GD_PNG
	    case FT_PNG:
		us->data = (void*)gdImageCreateFromPng(us->f);
		break;
#endif
#ifdef HAVE_GD_GIF
	    case FT_GIF:
		us->data = (void*)gdImageCreateFromGif(us->f);
		break;
#endif
#ifdef HAVE_GD_JPEG
	    case FT_JPEG:
		us->data = (void*)gdImageCreateFromJpeg(us->f);
		break;
#endif
#if 0
#ifdef HAVE_GD_XPM
	    case FT_XPM:
		us->data = (void*)gdImageCreateFromXpm(us->f);
		break;
#endif
#ifdef HAVE_GD_WBMP
	    case FT_WBMP:
		us->data = (void*)gdImageCreateFromWbmp(us->f);
		break;
#endif
#endif
	    default:
		break;
	}
        if (us->data)
	    us->datafree = gd_freeimage;

	gvusershape_file_release(us);
    }
    return (gdImagePtr)(us->data);
}

static gdImagePtr gd_rotateimage(gdImagePtr im, int rotation)
{
    gdImagePtr im2 = gdImageCreate(im->sy, im->sx);

    gdImageCopyRotated(im2, im, im2->sx / 2., im2->sy / 2.,
                0, 0, im->sx, im->sy, rotation);
    gdImageDestroy(im);
    return im2;
}
	
static void gd_loadimage_gd(GVJ_t * job, usershape_t *us, boxf b, boolean filled)
{
    gdImagePtr im2, im = (gdImagePtr) job->context;

    if ((im2 = gd_loadimage(job, us))) {
        if (job->rotation)
	    im2 = gd_rotateimage(im2, job->rotation);
        gdImageCopyResized(im, im2, ROUND(b.LL.x), ROUND(b.LL.y), 0, 0,
                ROUND(b.UR.x - b.LL.x), ROUND(b.UR.y - b.LL.y), im2->sx, im2->sy);
    }
}

#ifdef HAVE_PANGOCAIRO
static void gd_loadimage_cairo(GVJ_t * job, usershape_t *us, boxf b, boolean filled)
{
    cairo_t *cr = (cairo_t *) job->context; /* target context */
    unsigned int x, y, stride, width, height, px;
    unsigned char *data;
    cairo_surface_t *surface;    /* source surface */
    gdImagePtr im;

    if ((im = gd_loadimage(job, us))) {
	width = im->sx;
	height = im->sy;
// cairo_format_stride_for_width() not available prior to cairo-1.6.4 or so (fc9)
//stride = cairo_format_stride_for_width (CAIRO_FORMAT_ARGB32, width);
	stride = width*4;
	data = malloc (stride * height);
	surface = cairo_image_surface_create_for_data (data, CAIRO_FORMAT_ARGB32,
							width, height, stride);

	if (im->trueColor) {
	    for (y = 0; y < height; y++) {
		for (x = 0; x < width; x++) {
		    px = gdImageTrueColorPixel(im, x, y);
		    *data++ = gdTrueColorGetBlue(px);
		    *data++ = gdTrueColorGetGreen(px);
		    *data++ = gdTrueColorGetRed(px);
		    *data++ = (0x7F-gdTrueColorGetAlpha(px)) << 1;
		}
	    }
	}
	else {
	    for (y = 0; y < height; y++) {
		for (x = 0; x < width; x++) {
		    px = gdImagePalettePixel(im, x, y);
		    *data++ = im->blue[px];
		    *data++ = im->green[px];
		    *data++ = im->red[px];
		    *data++ = (px==im->transparent)?0x00:0xff;
		}
	    }
	}

        cairo_save(cr);
        cairo_translate(cr,
                (b.LL.x + (b.UR.x - b.LL.x) * (1. - (job->dpi.x) / 96.) / 2.),
                (-b.UR.y + (b.UR.y - b.LL.y) * (1. - (job->dpi.y) / 96.) / 2.));
        cairo_scale(cr,
                ((b.UR.x - b.LL.x) * (job->dpi.x) / (96. * us->w)),
                ((b.UR.y - b.LL.y) * (job->dpi.y) / (96. * us->h)));
        cairo_set_source_surface (cr, surface, 0, 0);
        cairo_paint (cr);
        cairo_restore(cr);

	cairo_surface_destroy(surface);
    }
}
#endif

static void gd_loadimage_ps(GVJ_t * job, usershape_t *us, boxf b, boolean filled)
{
    gdImagePtr im = NULL;
    int X, Y, x, y, px;

    if ((im = gd_loadimage(job, us))) {
	X = im->sx;
	Y = im->sy;

        gvputs(job, "save\n");

	/* define image data as string array (one per raster line) */
        gvputs(job, "/myctr 0 def\n");
        gvputs(job, "/myarray [\n");
        if (im->trueColor) {
            for (y = 0; y < Y; y++) {
		gvputs(job, "<");
                for (x = 0; x < X; x++) {
                    px = gdImageTrueColorPixel(im, x, y);
                    gvprintf(job, "%02x%02x%02x",
                        gdTrueColorGetRed(px),
                        gdTrueColorGetGreen(px),
                        gdTrueColorGetBlue(px));
                }
		gvputs(job, ">\n");
            }
	}
        else {
            for (y = 0; y < Y; y++) {
		gvputs(job, "<");
                for (x = 0; x < X; x++) {
                    px = gdImagePalettePixel(im, x, y);
                    gvprintf(job, "%02x%02x%02x",
                        im->red[px],
                        im->green[px],
                        im->blue[px]);
                }
		gvputs(job, ">\n");
            }
        }
	gvputs(job, "] def\n");
	gvputs(job,"/myproc { myarray myctr get /myctr myctr 1 add def } def\n");

        /* this sets the position of the image */
        gvprintf(job, "%g %g translate\n",
		(b.LL.x + (b.UR.x - b.LL.x) * (1. - (job->dpi.x) / 96.) / 2.),
	        (b.LL.y + (b.UR.y - b.LL.y) * (1. - (job->dpi.y) / 96.) / 2.));

        /* this sets the rendered size to fit the box */
        gvprintf(job,"%g %g scale\n",
		((b.UR.x - b.LL.x) * (job->dpi.x) / 96.),
		((b.UR.y - b.LL.y) * (job->dpi.y) / 96.));
    
        /* xsize ysize bits-per-sample [matrix] */
        gvprintf(job, "%d %d 8 [%d 0 0 %d 0 %d]\n", X, Y, X, -Y, Y);
    
        gvputs(job, "{myproc} false 3 colorimage\n");
    
        gvputs(job, "restore\n");
    }
}

static gvloadimage_engine_t engine = {
    gd_loadimage_gd
};

static gvloadimage_engine_t engine_ps = {
    gd_loadimage_ps
};

#ifdef HAVE_PANGOCAIRO
static gvloadimage_engine_t engine_cairo = {
    gd_loadimage_cairo
};
#endif

#endif

gvplugin_installed_t gvloadimage_gd_types[] = {
#ifdef HAVE_LIBGD

    {FORMAT_GD_GD, "gd:gd", 1, &engine, NULL},
    {FORMAT_GD2_GD, "gd2:gd", 1, &engine, NULL},
#ifdef HAVE_GD_GIF
    {FORMAT_GIF_GD, "gif:gd", 1, &engine, NULL},
#endif
#ifdef HAVE_GD_JPEG
    {FORMAT_JPG_GD, "jpeg:gd", 1, &engine, NULL},
    {FORMAT_JPG_GD, "jpe:gd", 1, &engine, NULL},
    {FORMAT_JPG_GD, "jpg:gd", 1, &engine, NULL},
#endif
#ifdef HAVE_GD_PNG
    {FORMAT_PNG_GD, "png:gd", 1, &engine, NULL},
#endif
#ifdef HAVE_GD_WBMP
    {FORMAT_WBMP_GD, "wbmp:gd", 1, &engine, NULL},
#endif
#ifdef HAVE_GD_XPM
    {FORMAT_XBM_GD, "xbm:gd", 1, &engine, NULL},
#endif

    {FORMAT_GD_PS, "gd:ps", 1, &engine_ps, NULL},
    {FORMAT_GD_PS, "gd:lasi", 1, &engine_ps, NULL},
    {FORMAT_GD2_PS, "gd2:ps", 1, &engine_ps, NULL},
    {FORMAT_GD2_PS, "gd2:lasi", 1, &engine_ps, NULL},
#ifdef HAVE_GD_GIF
    {FORMAT_GIF_PS, "gif:ps", 1, &engine_ps, NULL},
    {FORMAT_GIF_PS, "gif:lasi", 1, &engine_ps, NULL},
#endif
#ifdef HAVE_GD_JPEG
    {FORMAT_JPG_PS, "jpeg:ps", 1, &engine_ps, NULL},
    {FORMAT_JPG_PS, "jpg:ps", 1, &engine_ps, NULL},
    {FORMAT_JPG_PS, "jpe:ps", 1, &engine_ps, NULL},
    {FORMAT_JPG_PS, "jpeg:lasi", 1, &engine_ps, NULL},
    {FORMAT_JPG_PS, "jpg:lasi", 1, &engine_ps, NULL},
    {FORMAT_JPG_PS, "jpe:lasi", 1, &engine_ps, NULL},
#endif
#ifdef HAVE_GD_PNG
    {FORMAT_PNG_PS, "png:ps", 1, &engine_ps, NULL},
    {FORMAT_PNG_PS, "png:lasi", 1, &engine_ps, NULL},
#endif
#ifdef HAVE_GD_WBMP
    {FORMAT_WBMP_PS, "wbmp:ps", 1, &engine_ps, NULL},
    {FORMAT_WBMP_PS, "wbmp:lasi", 1, &engine_ps, NULL},
#endif
#ifdef HAVE_GD_XPM
    {FORMAT_XBM_PS, "xbm:ps", 1, &engine_ps, NULL},
    {FORMAT_XBM_PS, "xbm:lasi", 1, &engine_ps, NULL},
#endif

#ifdef HAVE_PANGOCAIRO
    {FORMAT_GD_CAIRO, "gd:cairo", 1, &engine_cairo, NULL},
    {FORMAT_GD2_CAIRO, "gd2:cairo", 1, &engine_cairo, NULL},
#ifdef HAVE_GD_GIF
    {FORMAT_GIF_CAIRO, "gif:cairo", 1, &engine_cairo, NULL},
#endif
#ifdef HAVE_GD_JPEG
    {FORMAT_JPG_CAIRO, "jpeg:cairo", 1, &engine_cairo, NULL},
    {FORMAT_JPG_CAIRO, "jpg:cairo", 1, &engine_cairo, NULL},
    {FORMAT_JPG_CAIRO, "jpe:cairo", 1, &engine_cairo, NULL},
#endif
#ifdef HAVE_GD_PNG
    {FORMAT_PNG_CAIRO, "png:cairo", -1, &engine_cairo, NULL},
#endif
#ifdef HAVE_GD_WBMP
    {FORMAT_WBMP_CAIRO, "wbmp:cairo", 1, &engine_cairo, NULL},
#endif
#ifdef HAVE_GD_XPM
    {FORMAT_XBM_CAIRO, "xbm:cairo", 1, &engine_cairo, NULL},
#endif
#endif /* HAVE_PANGOCAIRO */

#endif /* HAVE_LIBGD */
    {0, NULL, 0, NULL, NULL}
};
