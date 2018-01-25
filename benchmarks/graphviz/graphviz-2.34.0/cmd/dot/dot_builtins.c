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

#include "gvplugin.h"

#if defined(GVDLL) && !defined(ENABLE_LTDL)
#define extern	__declspec(dllimport)
#endif

extern gvplugin_library_t gvplugin_dot_layout_LTX_library;
extern gvplugin_library_t gvplugin_neato_layout_LTX_library;
#ifdef HAVE_LIBGD
extern gvplugin_library_t gvplugin_gd_LTX_library;
#endif
#ifdef HAVE_PANGOCAIRO
extern gvplugin_library_t gvplugin_pango_LTX_library;
#ifdef HAVE_WEBP
extern gvplugin_library_t gvplugin_webp_LTX_library;
#endif
#endif
extern gvplugin_library_t gvplugin_core_LTX_library;


lt_symlist_t lt_preloaded_symbols[] = {
	{ "gvplugin_dot_layout_LTX_library", (void*)(&gvplugin_dot_layout_LTX_library) },
	{ "gvplugin_neato_layout_LTX_library", (void*)(&gvplugin_neato_layout_LTX_library) },
#ifdef HAVE_PANGOCAIRO
	{ "gvplugin_pango_LTX_library", (void*)(&gvplugin_pango_LTX_library) },
#ifdef HAVE_WEBP
	{ "gvplugin_webp_LTX_library", (void*)(&gvplugin_webp_LTX_library) },
#endif
#endif
#ifdef HAVE_LIBGD
	{ "gvplugin_gd_LTX_library", (void*)(&gvplugin_gd_LTX_library) },
#endif
	{ "gvplugin_core_LTX_library", (void*)(&gvplugin_core_LTX_library) },
	{ 0, 0 }
};
