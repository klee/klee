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
#include <sys/stat.h>

#include "gvplugin_loadimage.h"

#ifdef HAVE_PANGOCAIRO
#ifdef HAVE_RSVG
#include <librsvg/rsvg.h>
#include <librsvg/rsvg-cairo.h>
#include <cairo/cairo-svg.h>

#ifdef WIN32
#define NUL_FILE "nul"
#else
#define NUL_FILE "/dev/null"
#endif

typedef enum {
    FORMAT_SVG_CAIRO,
} format_type;


static void gvloadimage_rsvg_free(usershape_t *us)
{
    rsvg_handle_close((RsvgHandle*)us->data, NULL);
}

static RsvgHandle* gvloadimage_rsvg_load(GVJ_t * job, usershape_t *us)
{
    RsvgHandle* rsvgh = NULL;
    guchar *fileBuf = NULL;
    GError *err = NULL;
    gsize fileSize;
    gint result;

    int fd;
    struct stat stbuf;

    assert(job);
    assert(us);
    assert(us->name);

    if (us->data) {
        if (us->datafree == gvloadimage_rsvg_free)
             rsvgh = (RsvgHandle*)(us->data); /* use cached data */
        else {
             us->datafree(us);        /* free incompatible cache data */
             us->data = NULL;
        }

    }

    if (!rsvgh) { /* read file into cache */
	if (!gvusershape_file_access(us))
	    return NULL;
        switch (us->type) {
            case FT_SVG:

		rsvg_init();

      		rsvgh = rsvg_handle_new();
		
		if (rsvgh == NULL) {
			fprintf(stderr, "rsvg_handle_new_from_file returned an error: %s\n", err->message);
			rsvg_term();
			return NULL;
		} 

	 	fd = fileno(us->f);
		fstat(fd, &stbuf);
  		fileSize = stbuf.st_size;	

		fileBuf = calloc(fileSize + 1, sizeof(guchar));

		if (fileBuf == NULL) {
			rsvg_handle_free(rsvgh);
			rsvg_term();
			return NULL;
		}
	
		rewind(us->f);

		if ((result = fread(fileBuf, 1, fileSize, us->f)) < fileSize) {
			rsvg_handle_free(rsvgh);
			rsvg_term();
			return NULL;
		}

		if (rsvg_handle_write(rsvgh, (const guchar *)fileBuf, (gsize)fileSize, &err) == FALSE) {
			fprintf(stderr, "rsvg_handle_write returned an error: %s\n", err->message);
			free(fileBuf);
			rsvg_handle_free(rsvgh);
			rsvg_term();
			return NULL;
		} 

		free(fileBuf);

		rsvg_handle_close(rsvgh, &err);
		rsvg_handle_set_dpi(rsvgh, POINTS_PER_INCH);

                break;
            default:
                rsvgh = NULL;
        }

        if (rsvgh) {
            us->data = (void*)rsvgh;
            us->datafree = gvloadimage_rsvg_free;
        }

	gvusershape_file_release(us);
    }

    return rsvgh;
}

static void gvloadimage_rsvg_cairo(GVJ_t * job, usershape_t *us, boxf b, boolean filled)
{
    RsvgHandle* rsvgh = gvloadimage_rsvg_load(job, us);

    cairo_t *cr = (cairo_t *) job->context; /* target context */
    cairo_surface_t *surface;	 /* source surface */

    if (rsvgh) {
        cairo_save(cr);

       	surface = cairo_svg_surface_create(NUL_FILE, us->w, us->h); 

	cairo_surface_reference(surface);

        cairo_set_source_surface(cr, surface, 0, 0);

        cairo_translate(cr, ROUND(b.LL.x), ROUND(-b.UR.y));
        cairo_scale(cr, (b.UR.x - b.LL.x) / us->w,
                       (b.UR.y - b.LL.y) / us->h);

	rsvg_handle_render_cairo(rsvgh, cr);

        cairo_paint (cr);
        cairo_restore(cr);
    }
}

static gvloadimage_engine_t engine_cairo = {
    gvloadimage_rsvg_cairo
};
#endif
#endif

gvplugin_installed_t gvloadimage_rsvg_types[] = {
#ifdef HAVE_PANGOCAIRO
#ifdef HAVE_RSVG
    {FORMAT_SVG_CAIRO, "svg:cairo", 1, &engine_cairo, NULL},
#endif
#endif
    {0, NULL, 0, NULL, NULL}
};
