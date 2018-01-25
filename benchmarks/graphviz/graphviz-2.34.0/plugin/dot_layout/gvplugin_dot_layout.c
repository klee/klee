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

#include "gvplugin.h"

extern gvplugin_installed_t gvlayout_dot_layout[];

static gvplugin_api_t apis[] = {
    {API_layout, gvlayout_dot_layout},
    {(api_t)0, 0},
};
/*visual studio*/
#ifdef WIN32_DLL
#ifndef GVPLUGIN_DOT_LAYOUT_EXPORTS
__declspec(dllimport) gvplugin_library_t gvplugin_dot_layout_LTX_library = { "dot_layout", apis };
#else
__declspec(dllexport) gvplugin_library_t gvplugin_dot_layout_LTX_library = { "dot_layout", apis };
#endif
#endif



/*end visual studio*/


#ifndef WIN32_DLL
#ifdef GVDLL
__declspec(dllexport) gvplugin_library_t gvplugin_dot_layout_LTX_library = { "dot_layout", apis };
#else
gvplugin_library_t gvplugin_dot_layout_LTX_library = { "dot_layout", apis };
#endif
#endif


