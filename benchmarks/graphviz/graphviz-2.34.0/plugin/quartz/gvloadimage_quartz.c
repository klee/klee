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

#include "gvplugin_loadimage.h"

#include "gvplugin_quartz.h"

static size_t file_data_provider_get_bytes(void *info, void *buffer, size_t count)
{
	return fread(buffer, 1, count, (FILE*)info);
}

static void file_data_provider_rewind(void *info)
{
	fseek((FILE*)info, 0, SEEK_SET);
}

#if __MAC_OS_X_VERSION_MIN_REQUIRED >= 1050 || __IPHONE_OS_VERSION_MIN_REQUIRED >= 20000

static off_t file_data_provider_skip_forward(void *info, off_t count)
{
	fseek((FILE*)info, count, SEEK_CUR);
	return count;
}

/* bridge FILE* to a sequential CGDataProvider */
static CGDataProviderSequentialCallbacks file_data_provider_callbacks = {
	0,
	file_data_provider_get_bytes,
	file_data_provider_skip_forward,
	file_data_provider_rewind,
	NULL
};

#else

static void file_data_provider_skip_bytes(void *info, size_t count)
{
	fseek((FILE*)info, count, SEEK_CUR);
}

/* bridge FILE* to a sequential CGDataProvider */
static CGDataProviderCallbacks file_data_provider_callbacks = {
	file_data_provider_get_bytes,
	file_data_provider_skip_bytes,
	file_data_provider_rewind,
	NULL
};

#endif



static void quartz_freeimage(usershape_t *us)
{
	CGImageRelease((CGImageRef)us->data);
}

static CGImageRef quartz_loadimage(GVJ_t * job, usershape_t *us)
{
    assert(job);
    assert(us);
    assert(us->name);

    if (us->data && us->datafree != quartz_freeimage) {
	     us->datafree(us);        /* free incompatible cache data */
	     us->data = NULL;
	     us->datafree = NULL;
	}
    
    if (!us->data) { /* read file into cache */
		if (!gvusershape_file_access(us))
			return NULL;
			
#if __MAC_OS_X_VERSION_MIN_REQUIRED >= 1050 || __IPHONE_OS_VERSION_MIN_REQUIRED >= 20000
		CGDataProviderRef data_provider = CGDataProviderCreateSequential(us->f, &file_data_provider_callbacks);
#else
		CGDataProviderRef data_provider = CGDataProviderCreate(us->f, &file_data_provider_callbacks);	
#endif
		
#if __MAC_OS_X_VERSION_MIN_REQUIRED >= 1040
		/* match usershape format to a UTI for type hinting, if possible */
		format_type hint_format_type;
		switch (us->type) {
		case FT_BMP:
			hint_format_type = FORMAT_BMP;
			break;
		case FT_GIF:
			hint_format_type = FORMAT_GIF;
			break;
		case FT_PNG:
			hint_format_type = FORMAT_PNG;
			break;
		case FT_JPEG:
			hint_format_type = FORMAT_JPEG;
			break;
		case FT_PDF:
			hint_format_type = FORMAT_PDF;
			break;
		default:
			hint_format_type = FORMAT_NONE;
			break;
		}
		CFDictionaryRef options = hint_format_type == FORMAT_NONE ? NULL : CFDictionaryCreate(
			kCFAllocatorDefault,
			(const void **)&kCGImageSourceTypeIdentifierHint,
			(const void **)(format_uti + hint_format_type),
			1,
			&kCFTypeDictionaryKeyCallBacks,
			&kCFTypeDictionaryValueCallBacks);

		/* get first image from usershape file */
		CGImageSourceRef image_source = CGImageSourceCreateWithDataProvider(data_provider, options);
		us->data = CGImageSourceCreateImageAtIndex(image_source, 0, NULL);
		if (image_source)
			CFRelease(image_source);
		if (options)
			CFRelease(options);
#else
		switch (us->type) {
			case FT_PNG:		
				us->data = CGImageCreateWithPNGDataProvider(data_provider, NULL, false, kCGRenderingIntentDefault);
				break;	
			case FT_JPEG:		
				us->data = CGImageCreateWithJPEGDataProvider(data_provider, NULL, false, kCGRenderingIntentDefault);
				break;
			default:
				us->data = NULL;
				break;
		}
		
#endif
		/* clean up */
		if (us->data)
			us->datafree = quartz_freeimage;
		CGDataProviderRelease(data_provider);
			
		gvusershape_file_release(us);
    }
    return (CGImageRef)(us->data);
}

static void quartz_loadimage_quartz(GVJ_t * job, usershape_t *us, boxf b, boolean filled)
{
	/* get the image from usershape details, then blit it to the context */
	CGImageRef image = quartz_loadimage(job, us);
	if (image)
		CGContextDrawImage((CGContextRef)job->context, CGRectMake(b.LL.x, b.LL.y, b.UR.x - b.LL.x, b.UR.y - b.LL.y), image);
}

static gvloadimage_engine_t engine = {
    quartz_loadimage_quartz
};

gvplugin_installed_t gvloadimage_quartz_types[] = {
#if __MAC_OS_X_VERSION_MIN_REQUIRED >= 1040
	{FORMAT_BMP, "bmp:quartz", 8, &engine, NULL},
	{FORMAT_GIF, "gif:quartz", 8, &engine, NULL},
	{FORMAT_PDF, "pdf:quartz", 8, &engine, NULL},
#endif
	{FORMAT_JPEG, "jpe:quartz", 8, &engine, NULL},
	{FORMAT_JPEG, "jpeg:quartz", 8, &engine, NULL},
	{FORMAT_JPEG, "jpg:quartz", 8, &engine, NULL},
	{FORMAT_PNG, "png:quartz", 8, &engine, NULL},
	{0, NULL, 0, NULL, NULL}
};