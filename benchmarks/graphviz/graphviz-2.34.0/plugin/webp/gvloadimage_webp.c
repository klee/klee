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

#include "gvplugin_loadimage.h"
#include "gvio.h"

#ifdef HAVE_WEBP
#ifdef HAVE_PANGOCAIRO
#include <cairo.h>
#include <webp/decode.h>

#ifdef WIN32 //*dependencies
    #pragma comment( lib, "gvc.lib" )
    #pragma comment( lib, "glib-2.0.lib" )
    #pragma comment( lib, "pango-1.0.lib" )
    #pragma comment( lib, "pangocairo-1.0.lib" )
    #pragma comment( lib, "cairo.lib" )
    #pragma comment( lib, "gobject-2.0.lib" )
    #pragma comment( lib, "graph.lib" )
    #pragma comment( lib, "webp.lib" )
#endif

static const char* const kStatusMessages[] = {
    "OK", "OUT_OF_MEMORY", "INVALID_PARAM", "BITSTREAM_ERROR",
    "UNSUPPORTED_FEATURE", "SUSPENDED", "USER_ABORT", "NOT_ENOUGH_DATA"
};

typedef enum {
    FORMAT_WEBP_CAIRO,
} format_type;

static void webp_freeimage(usershape_t *us)
{
    cairo_surface_destroy((cairo_surface_t*)(us->data));
}

static cairo_surface_t* webp_really_loadimage(const char *in_file, FILE* const in)
{
    WebPDecoderConfig config;
    WebPDecBuffer* const output_buffer = &config.output;
    WebPBitstreamFeatures* const bitstream = &config.input;
    VP8StatusCode status = VP8_STATUS_OK;
    cairo_surface_t *surface = NULL; /* source surface */
    int ok;
    uint32_t data_size = 0;
    void* data = NULL;

    if (!WebPInitDecoderConfig(&config)) {
	fprintf(stderr, "Error: WebP library version mismatch!\n");
	return NULL;
    }

    fseek(in, 0, SEEK_END);
    data_size = ftell(in);
    fseek(in, 0, SEEK_SET);
    data = malloc(data_size);
    ok = (fread(data, data_size, 1, in) == 1);
    if (!ok) {
        fprintf(stderr, "Error: WebP could not read %d bytes of data from %s\n",
            data_size, in_file);
        free(data);
        return NULL;
    }

    status = WebPGetFeatures((const uint8_t*)data, data_size, bitstream);
    if (status != VP8_STATUS_OK) {
	goto end;
    }

    output_buffer->colorspace = MODE_RGBA;
    status = WebPDecode((const uint8_t*)data, data_size, &config);

    /* FIXME - this is ugly */
    if (! bitstream->has_alpha) {
	int x, y;
	unsigned char *p, t;

	for (y = 0; y < output_buffer->height; y++) {
	    p = output_buffer->u.RGBA.rgba + (output_buffer->u.RGBA.stride * y);
	    for (x = 0; x < output_buffer->width; x++) {
		t = p[0];     /* swap red/blue */
		p[0] = p[2];
		p[2] = t;
		p += 4;
	    }
	}
    }

end:
    free(data);
    ok = (status == VP8_STATUS_OK);
    if (!ok) {
	fprintf(stderr, "Error: WebP decoding of %s failed.\n", in_file);
	fprintf(stderr, "Status: %d (%s)\n", status, kStatusMessages[status]);
	return NULL;
    }

    surface = cairo_image_surface_create_for_data (
	    output_buffer->u.RGBA.rgba,
	    CAIRO_FORMAT_ARGB32,
	    output_buffer->width,
	    output_buffer->height,
	    output_buffer->u.RGBA.stride);

    return surface;
}

/* get image either from cached surface, or from freskly loaded surface */
static cairo_surface_t* webp_loadimage(GVJ_t * job, usershape_t *us)
{
    cairo_surface_t *surface = NULL; /* source surface */

    assert(job);
    assert(us);
    assert(us->name);

    if (us->data) {
        if (us->datafree == webp_freeimage)
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
            case FT_WEBP:
		if ((surface = webp_really_loadimage(us->name, us->f)))
                    cairo_surface_reference(surface);
                break;
            default:
                surface = NULL;
        }
        if (surface) {
            us->data = (void*)surface;
            us->datafree = webp_freeimage;
        }
	gvusershape_file_release(us);
    }
    return surface;
}

/* paint image into required location in graph */
static void webp_loadimage_cairo(GVJ_t * job, usershape_t *us, boxf b, boolean filled)
{
    cairo_t *cr = (cairo_t *) job->context; /* target context */
    cairo_surface_t *surface;	 /* source surface */

    surface = webp_loadimage(job, us);
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

static gvloadimage_engine_t engine_webp = {
    webp_loadimage_cairo
};
#endif
#endif

gvplugin_installed_t gvloadimage_webp_types[] = {
#ifdef HAVE_WEBP
#ifdef HAVE_PANGOCAIRO
    {FORMAT_WEBP_CAIRO, "webp:cairo", 1, &engine_webp, NULL},
#endif
#endif
    {0, NULL, 0, NULL, NULL}
};
