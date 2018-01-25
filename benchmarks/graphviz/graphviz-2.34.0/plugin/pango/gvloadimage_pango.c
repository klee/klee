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

#include "gvplugin_loadimage.h"
#include "gvio.h"

#ifdef HAVE_PANGOCAIRO
#include <cairo.h>

#ifdef WIN32 //*dependencies
    #pragma comment( lib, "gvc.lib" )
    #pragma comment( lib, "glib-2.0.lib" )
    #pragma comment( lib, "pango-1.0.lib" )
    #pragma comment( lib, "pangocairo-1.0.lib" )
    #pragma comment( lib, "cairo.lib" )
    #pragma comment( lib, "gobject-2.0.lib" )
#ifdef WITH_CGRAPH
    #pragma comment( lib, "cgraph.lib" )
#else
    #pragma comment( lib, "graph.lib" )
#endif
#endif




typedef enum {
    FORMAT_PNG_CAIRO, FORMAT_PNG_PS,
} format_type;

static cairo_status_t
reader (void *closure, unsigned char *data, unsigned int length)
{
    if (length == fread(data, 1, length, (FILE *)closure)
     || feof((FILE *)closure))
        return CAIRO_STATUS_SUCCESS;
    return CAIRO_STATUS_READ_ERROR;
}

static void cairo_freeimage(usershape_t *us)
{
    cairo_surface_destroy((cairo_surface_t*)(us->data));
}

static cairo_surface_t* cairo_loadimage(GVJ_t * job, usershape_t *us)
{
    cairo_surface_t *surface = NULL; /* source surface */

    assert(job);
    assert(us);
    assert(us->name);

    if (us->data) {
        if (us->datafree == cairo_freeimage)
             surface = (cairo_surface_t*)(us->data); /* use cached data */
        else {
             us->datafree(us);        /* free incompatible cache data */
             us->datafree = NULL;
             us->data = NULL;
        }
    }
    if (!surface) { /* read file into cache */
	if (!gvusershape_file_access(us))
	    return NULL;
        switch (us->type) {
#ifdef CAIRO_HAS_PNG_FUNCTIONS
            case FT_PNG:
                surface = cairo_image_surface_create_from_png_stream(reader, us->f);
                cairo_surface_reference(surface);
                break;
#endif
            default:
                surface = NULL;
        }
        if (surface) {
            us->data = (void*)surface;
            us->datafree = cairo_freeimage;
        }
	gvusershape_file_release(us);
    }
    return surface;
}

static void pango_loadimage_cairo(GVJ_t * job, usershape_t *us, boxf b, boolean filled)
{
    cairo_t *cr = (cairo_t *) job->context; /* target context */
    cairo_surface_t *surface;	 /* source surface */

    surface = cairo_loadimage(job, us);
    if (surface) {
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
    }
}

static void pango_loadimage_ps(GVJ_t * job, usershape_t *us, boxf b, boolean filled)
{
    cairo_surface_t *surface; 	/* source surface */
    cairo_format_t format;
    int X, Y, x, y, stride;
    unsigned char *data, *ix, alpha, red, green, blue;

    surface = cairo_loadimage(job, us);
    if (surface) {
       	format = cairo_image_surface_get_format(surface);
        if ((format != CAIRO_FORMAT_ARGB32) && (format != CAIRO_FORMAT_RGB24))
	    return;

	X = cairo_image_surface_get_width(surface);
	Y = cairo_image_surface_get_height(surface);
	stride = cairo_image_surface_get_stride(surface);
	data = cairo_image_surface_get_data(surface);

        gvputs(job, "save\n");

	/* define image data as string array (one per raster line) */
	/* see parallel code in gd_loadimage_ps().  FIXME: refactor... */
        gvputs(job, "/myctr 0 def\n");
        gvputs(job, "/myarray [\n");
        for (y = 0; y < Y; y++) {
	    gvputs(job, "<");
	    ix = data + y * stride;
            for (x = 0; x < X; x++) {
		/* FIXME - this code may have endian problems */
		blue = *ix++;
		green = *ix++;
		red = *ix++;
		alpha = *ix++;
		if (alpha < 0x7f)
		    gvputs(job, "ffffff");
		else
                    gvprintf(job, "%02x%02x%02x", red, green, blue);
            }
	    gvputs(job, ">\n");
        }
	gvputs(job, "] def\n");
        gvputs(job,"/myproc { myarray myctr get /myctr myctr 1 add def } def\n");

        /* this sets the position of the image */
        gvprintf(job, "%g %g translate\n",
		(b.LL.x + (b.UR.x - b.LL.x) * (1. - (job->dpi.x) / 96.) / 2.),
		(b.LL.y + (b.UR.y - b.LL.y) * (1. - (job->dpi.y) / 96.) / 2.));

        /* this sets the rendered size to fit the box */
        gvprintf(job,"%g %g scale\n",
		((b.UR.x - b.LL.x) * 72. / 96.),
		((b.UR.y - b.LL.y) * 72. / 96.));

        /* xsize ysize bits-per-sample [matrix] */
        gvprintf(job, "%d %d 8 [%d 0 0 %d 0 %d]\n", X, Y, X, -Y, Y);

        gvputs(job, "{myproc} false 3 colorimage\n");

        gvputs(job, "restore\n");
    }
}

static gvloadimage_engine_t engine_cairo = {
    pango_loadimage_cairo
};

static gvloadimage_engine_t engine_ps = {
    pango_loadimage_ps
};
#endif

gvplugin_installed_t gvloadimage_pango_types[] = {
#ifdef HAVE_PANGOCAIRO
    {FORMAT_PNG_CAIRO, "png:cairo", 1, &engine_cairo, NULL},
    {FORMAT_PNG_PS, "png:lasi", 2, &engine_ps, NULL},
    {FORMAT_PNG_PS, "png:ps", 2, &engine_ps, NULL},
#endif
    {0, NULL, 0, NULL, NULL}
};
