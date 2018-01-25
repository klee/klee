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
#include <gdk-pixbuf/gdk-pixbuf.h>

#ifdef WIN32 //*dependencies
    #pragma comment( lib, "gvc.lib" )
    #pragma comment( lib, "glib-2.0.lib" )
    #pragma comment( lib, "cairo.lib" )
    #pragma comment( lib, "gobject-2.0.lib" )
    #pragma comment( lib, "graph.lib" )
    #pragma comment( lib, "gdk-pixbuf.lib" )
#endif

typedef enum {
    FORMAT_BMP_CAIRO,
    FORMAT_ICO_CAIRO,
    FORMAT_JPEG_CAIRO,
    FORMAT_PNG_CAIRO,
    FORMAT_TIFF_CAIRO,
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

static void gdk_pixbuf_loadimage_cairo(GVJ_t * job, usershape_t *us, boxf b, boolean filled)
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

static gvloadimage_engine_t engine_cairo = {
    gdk_pixbuf_loadimage_cairo
};

#endif

gvplugin_installed_t gvloadimage_gdk_pixbuf_types[] = {
#if 0
#ifdef HAVE_PANGOCAIRO
    {FORMAT_BMP_CAIRO, "bmp:cairo", -99, &engine_cairo, NULL},
    {FORMAT_ICO_CAIRO, "ico:cairo", -99, &engine_cairo, NULL},
    {FORMAT_JPEG_CAIRO, "jpg:cairo", -99, &engine_cairo, NULL},
    {FORMAT_JPEG_CAIRO, "jpeg:cairo", -99, &engine_cairo, NULL},
    {FORMAT_PNG_CAIRO, "png:cairo", -99, &engine_cairo, NULL},
    {FORMAT_TIFF_CAIRO, "tif:cairo", -99, &engine_cairo, NULL},
    {FORMAT_TIFF_CAIRO, "tiff`:cairo", -99, &engine_cairo, NULL},
#endif
#endif
    {0, NULL, 0, NULL, NULL}
};
