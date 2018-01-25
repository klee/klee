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

#include "gvplugin_quartz.h"

#if __ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__ >= 1040 && defined(HAVE_PANGOCAIRO)

const void *memory_data_consumer_get_byte_pointer(void *info)
{
	return info;
}

CGDataProviderDirectCallbacks memory_data_provider_callbacks = {
	0,
	memory_data_consumer_get_byte_pointer,
	NULL,
	NULL,
	NULL
};

static void quartz_format(GVJ_t *job)
{
	/* image destination -> data consumer -> job's gvdevice */
	/* data provider <- job's imagedata */
	CGDataConsumerRef data_consumer = CGDataConsumerCreate(job, &device_data_consumer_callbacks);
	CGImageDestinationRef image_destination = CGImageDestinationCreateWithDataConsumer(data_consumer, format_uti[job->device.id], 1, NULL);
	CGDataProviderRef data_provider = CGDataProviderCreateDirect(job->imagedata, BYTES_PER_PIXEL * job->width * job->height, &memory_data_provider_callbacks);
	
	/* add the bitmap image to the destination and save it */
	CGColorSpaceRef color_space = CGColorSpaceCreateWithName(kCGColorSpaceSRGB);
	CGImageRef image = CGImageCreate (
		job->width,							/* width in pixels */
		job->height,						/* height in pixels */
		BITS_PER_COMPONENT,					/* bits per component */
		BYTES_PER_PIXEL * 8,				/* bits per pixel */
		BYTES_PER_PIXEL * job->width,		/* bytes per row: exactly width # of pixels */
		color_space,						/* color space: sRGB */
		kCGImageAlphaPremultipliedFirst,	/* bitmap info: corresponds to CAIRO_FORMAT_ARGB32 */
		data_provider,						/* data provider: from imagedata */
		NULL,								/* decode: don't remap colors */
		FALSE,								/* don't interpolate */
		kCGRenderingIntentDefault			/* rendering intent (what to do with out-of-gamut colors): default */
	);
	CGImageDestinationAddImage(image_destination, image, NULL);
	CGImageDestinationFinalize(image_destination);
	
	/* clean up */
	CGImageRelease(image);
	CGColorSpaceRelease(color_space);
	CGDataProviderRelease(data_provider);
	if (image_destination)
		CFRelease(image_destination);
	CGDataConsumerRelease(data_consumer);
}

static gvdevice_engine_t quartz_engine = {
    NULL,		/* quartz_initialize */
    quartz_format,
    NULL,		/* quartz_finalize */
};

static gvdevice_features_t device_features_quartz = {
	GVDEVICE_BINARY_FORMAT        
          | GVDEVICE_DOES_TRUECOLOR,/* flags */
	{0.,0.},                    /* default margin - points */
	{0.,0.},                    /* default page width, height - points */
	{96.,96.},                  /* dpi */
};

gvplugin_installed_t gvdevice_quartz_types_for_cairo[] = {
	{FORMAT_BMP, "bmp:cairo", 9, &quartz_engine, &device_features_quartz},
	{FORMAT_GIF, "gif:cairo", 9, &quartz_engine, &device_features_quartz},
	{FORMAT_EXR, "exr:cairo", 9, &quartz_engine, &device_features_quartz},
	{FORMAT_JPEG, "jpe:cairo", 9, &quartz_engine, &device_features_quartz},
	{FORMAT_JPEG, "jpeg:cairo", 9, &quartz_engine, &device_features_quartz},
	{FORMAT_JPEG, "jpg:cairo", 9, &quartz_engine, &device_features_quartz},
	{FORMAT_JPEG2000, "jp2:cairo", 9, &quartz_engine, &device_features_quartz},
	{FORMAT_PICT, "pct:cairo", 9, &quartz_engine, &device_features_quartz},
	{FORMAT_PICT, "pict:cairo", 9, &quartz_engine, &device_features_quartz},
	{FORMAT_PNG, "png:cairo", 9, &quartz_engine, &device_features_quartz},
	{FORMAT_PSD, "psd:cairo", 9, &quartz_engine, &device_features_quartz},
	{FORMAT_SGI, "sgi:cairo", 9, &quartz_engine, &device_features_quartz},
	{FORMAT_TIFF, "tif:cairo", 9, &quartz_engine, &device_features_quartz},
	{FORMAT_TIFF, "tiff:cairo", 9, &quartz_engine, &device_features_quartz},
	{FORMAT_TGA, "tga:cairo", 9, &quartz_engine, &device_features_quartz},
	{0, NULL, 0, NULL, NULL}
};

#endif
