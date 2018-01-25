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

#include "types.h"
#include "gvplugin.h"
#include "gvplugin_quartz.h"
#include "gvio.h"

extern gvplugin_installed_t gvrender_quartz_types;
extern gvplugin_installed_t gvtextlayout_quartz_types;
extern gvplugin_installed_t gvloadimage_quartz_types;
extern gvplugin_installed_t gvdevice_quartz_types;

#if __ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__ >= 1040
extern gvplugin_installed_t gvdevice_quartz_types_for_cairo;

/* Uniform Type Identifiers corresponding to each format_type */
CFStringRef format_uti [] = {
	NULL,
	NULL,
	CFSTR("com.microsoft.bmp"),
	CFSTR("com.ilm.openexr-image"),
	CFSTR("com.compuserve.gif"),
	CFSTR("public.jpeg"),
	CFSTR("public.jpeg-2000"),
	CFSTR("com.adobe.pdf"),
	CFSTR("com.apple.pict"),
	CFSTR("public.png"),
	CFSTR("com.adobe.photoshop-image"),
	CFSTR("com.sgi.sgi-image"),
	CFSTR("public.tiff"),
	CFSTR("com.truevision.tga-image")
};

#endif
/* data consumer backed by the gvdevice */

static size_t device_data_consumer_put_bytes (void *info, const void *buffer, size_t count)
{
	return gvwrite((GVJ_t *)info, (const char*)buffer, count);
}

CGDataConsumerCallbacks device_data_consumer_callbacks = {
	device_data_consumer_put_bytes,
	NULL
};

static gvplugin_api_t apis[] = {
    {API_render, &gvrender_quartz_types},
	{API_textlayout, &gvtextlayout_quartz_types},
	{API_loadimage, &gvloadimage_quartz_types},
	{API_device, &gvdevice_quartz_types},
#if __ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__ >= 1040 && defined(HAVE_PANGOCAIRO)
    {API_device, &gvdevice_quartz_types_for_cairo},
#endif
    {(api_t)0, 0},
};

gvplugin_library_t gvplugin_quartz_LTX_library = { "quartz", apis };
