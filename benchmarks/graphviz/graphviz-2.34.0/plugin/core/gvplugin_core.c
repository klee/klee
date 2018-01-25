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

extern gvplugin_installed_t gvdevice_dot_types[];
extern gvplugin_installed_t gvdevice_fig_types[];
extern gvplugin_installed_t gvdevice_map_types[];
extern gvplugin_installed_t gvdevice_ps_types[];
extern gvplugin_installed_t gvdevice_svg_types[];
extern gvplugin_installed_t gvdevice_tk_types[];
extern gvplugin_installed_t gvdevice_vml_types[];
extern gvplugin_installed_t gvdevice_pic_types[];
extern gvplugin_installed_t gvdevice_pov_types[];

extern gvplugin_installed_t gvrender_dot_types[];
extern gvplugin_installed_t gvrender_fig_types[];
extern gvplugin_installed_t gvrender_map_types[];
extern gvplugin_installed_t gvrender_ps_types[];
extern gvplugin_installed_t gvrender_svg_types[];
extern gvplugin_installed_t gvrender_tk_types[];
extern gvplugin_installed_t gvrender_vml_types[];
extern gvplugin_installed_t gvrender_pic_types[];
extern gvplugin_installed_t gvrender_pov_types[];

extern gvplugin_installed_t gvloadimage_core_types[];




static gvplugin_api_t apis[] = {
    {API_device, gvdevice_dot_types},
    {API_device, gvdevice_fig_types},
    {API_device, gvdevice_map_types},
    {API_device, gvdevice_ps_types},
    {API_device, gvdevice_svg_types},
    {API_device, gvdevice_tk_types},
    {API_device, gvdevice_vml_types},
    {API_device, gvdevice_pic_types},
    {API_device, gvdevice_pov_types},

    {API_render, gvrender_dot_types},
    {API_render, gvrender_fig_types},
    {API_render, gvrender_map_types},
    {API_render, gvrender_ps_types},
    {API_render, gvrender_svg_types},
    {API_render, gvrender_tk_types},
    {API_render, gvrender_vml_types},
    {API_render, gvrender_pic_types},
    {API_render, gvrender_pov_types},

    {API_loadimage, gvloadimage_core_types},

    {(api_t)0, 0},
};

#ifdef WIN32_DLL
#ifndef GVPLUGIN_CORE_EXPORTS
__declspec(dllimport) gvplugin_library_t gvplugin_core_LTX_library = { "core", apis };
#else
__declspec(dllexport) gvplugin_library_t gvplugin_core_LTX_library = { "core", apis };
#endif
#endif



#ifndef WIN32_DLL
#ifdef GVDLL
__declspec(dllexport) gvplugin_library_t gvplugin_core_LTX_library = { "core", apis };
#else
gvplugin_library_t gvplugin_core_LTX_library = { "core", apis };
#endif
#endif


