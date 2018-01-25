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


#include "gvplugin_device.h"
#include "gvplugin_render.h"
#include "gvplugin_gdiplus.h"

extern "C" size_t gvwrite(GVJ_t *job, const unsigned char *s, unsigned int len);







using namespace Gdiplus;

static void gdiplus_format(GVJ_t *job)
{
	UseGdiplus();

	/* allocate memory and attach stream to it */
	HGLOBAL buffer = GlobalAlloc(GMEM_MOVEABLE, 0);
	IStream *stream = NULL;
	CreateStreamOnHGlobal(buffer, FALSE, &stream);	/* FALSE means don't deallocate buffer when releasing stream */
	
	Bitmap bitmap(
		job->width,						/* width in pixels */
		job->height,					/* height in pixels */
		job->width * BYTES_PER_PIXEL,	/* bytes per row: exactly width # of pixels */
		PixelFormat32bppPARGB,			/* pixel format: corresponds to CAIRO_FORMAT_ARGB32 */
		(BYTE*)job->imagedata);				/* pixel data from job */
	SaveBitmapToStream(bitmap, stream, job->device.id);
	
	/* blast the streamed buffer back to the gvdevice */
	/* NOTE: this is somewhat inefficient since we should be streaming directly to gvdevice rather than buffering first */
	/* ... however, GDI+ requires any such direct IStream to implement Seek Read, Write, Stat methods and gvdevice really only offers a write-once model */
	stream->Release();
	gvwrite(job, (const unsigned char*)GlobalLock(buffer), GlobalSize(buffer));

	GlobalFree(buffer);
}

static gvdevice_engine_t gdiplus_engine = {
    NULL,		/* gdiplus_initialize */
    gdiplus_format,
    NULL,		/* gdiplus_finalize */
};

static gvdevice_features_t device_features_gdiplus = {
	GVDEVICE_BINARY_FORMAT        
          | GVDEVICE_DOES_TRUECOLOR,/* flags */
	{0.,0.},                    /* default margin - points */
	{0.,0.},                    /* default page width, height - points */
	{96.,96.},                  /* dpi */
};

gvplugin_installed_t gvdevice_gdiplus_types_for_cairo[] = {
	{FORMAT_BMP, "bmp:cairo", 8, &gdiplus_engine, &device_features_gdiplus},
	{FORMAT_GIF, "gif:cairo", 8, &gdiplus_engine, &device_features_gdiplus},
	{FORMAT_JPEG, "jpe:cairo", 8, &gdiplus_engine, &device_features_gdiplus},
	{FORMAT_JPEG, "jpeg:cairo", 8, &gdiplus_engine, &device_features_gdiplus},
	{FORMAT_JPEG, "jpg:cairo", 8, &gdiplus_engine, &device_features_gdiplus},
	{FORMAT_PNG, "png:cairo", 8, &gdiplus_engine, &device_features_gdiplus},
	{FORMAT_TIFF, "tif:cairo", 8, &gdiplus_engine, &device_features_gdiplus},
	{FORMAT_TIFF, "tiff:cairo", 8, &gdiplus_engine, &device_features_gdiplus},
	{0, NULL, 0, NULL, NULL}
};
