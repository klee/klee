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

extern gvplugin_installed_t gvrender_ming_types[];
extern gvplugin_installed_t gvdevice_ming_types[];

static gvplugin_api_t apis[] = {
    {API_device, gvdevice_ming_types},
    {API_render, gvrender_ming_types},
    {(api_t)0, 0},
};

gvplugin_library_t gvplugin_ming_LTX_library = { "ming", apis };
