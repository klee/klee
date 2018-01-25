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

#ifndef GVPLUGIN_QUARTZ_H
#define GVPLUGIN_QUARTZ_H

#include <Availability.h>

#if defined(__IPHONE_OS_VERSION_MIN_REQUIRED)
#include <CoreGraphics/CoreGraphics.h>
#elif defined(__MAC_OS_X_VERSION_MIN_REQUIRED)
#include <ApplicationServices/ApplicationServices.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	FORMAT_NONE,
	FORMAT_CGIMAGE,
	FORMAT_BMP,
	FORMAT_EXR,
	FORMAT_GIF,
	FORMAT_JPEG,
	FORMAT_JPEG2000,
	FORMAT_PDF,
	FORMAT_PICT,
	FORMAT_PNG,
	FORMAT_PSD,
	FORMAT_SGI,
	FORMAT_TIFF,
	FORMAT_TGA
} format_type;

static const int BYTE_ALIGN = 15;			/* align to 16 bytes */
static const int BITS_PER_COMPONENT = 8;	/* bits per color component */
static const int BYTES_PER_PIXEL = 4;		/* bytes per pixel */

#if __ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__ >= 1040
extern CFStringRef format_uti [];
#endif

extern CGDataConsumerCallbacks device_data_consumer_callbacks;

/* gvtextlayout_quartz.c in Mac OS X: layout is a CoreText CTLineRef */
/* GVTextLayout.m in iPhoneOS: layout is a custom Objective-C GVTextLayout */

void *quartz_new_layout(char* fontname, double fontsize, char* text);
void quartz_size_layout(void *layout, double* width, double* height, double* yoffset_layout);
void quartz_draw_layout(void *layout, CGContextRef context, CGPoint position);
void quartz_free_layout(void *layout);

#ifdef __cplusplus
}
#endif

#endif
