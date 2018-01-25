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

/*
 *  graphics code generator wrapper
 *
 *  This library forms the socket for run-time loadable loadimage plugins.  
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>

#include "const.h"
#include "gvplugin_loadimage.h"
#include "gvcint.h"
#include "gvcproc.h"

/* for agerr() */
#if WITH_CGRAPH
#include "cgraph.h"
#else
#include "graph.h"
#endif

static int gvloadimage_select(GVJ_t * job, char *str)
{
    gvplugin_available_t *plugin;
    gvplugin_installed_t *typeptr;

    plugin = gvplugin_load(job->gvc, API_loadimage, str);
    if (plugin) {
        typeptr = plugin->typeptr;
        job->loadimage.engine = (gvloadimage_engine_t *) (typeptr->engine);
        job->loadimage.id = typeptr->id;
        return GVRENDER_PLUGIN;
    }
    return NO_SUPPORT;
}

void gvloadimage(GVJ_t * job, usershape_t *us, boxf b, boolean filled, const char *target)
{
    gvloadimage_engine_t *gvli;
    char type[SMALLBUF];

    strcpy(type, us->stringtype);
    strcat(type, ":");
    strcat(type, target);

    if (gvloadimage_select(job, type) == NO_SUPPORT)
	    agerr (AGWARN, "No loadimage plugin for \"%s\"\n", type);

    if ((gvli = job->loadimage.engine) && gvli->loadimage)
	gvli->loadimage(job, us, b, filled);
}
