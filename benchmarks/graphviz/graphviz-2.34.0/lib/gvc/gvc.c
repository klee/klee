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

#include "gvc.h"
#include "const.h"
#include "gvcjob.h"
#include "gvcint.h"
#include "gvcproc.h"
#include "gvconfig.h"
#include "gvio.h"
#ifdef WITH_CGRAPH
#include <stdlib.h>
#endif

#ifdef WIN32 /*dependencies*/
#ifdef WITH_CGRAPH
    #pragma comment( lib, "cgraph.lib" )
#else
    #pragma comment( lib, "graph.lib" )
#endif
    #pragma comment( lib, "cdt.lib" )
    #pragma comment( lib, "ltdl.lib" )
    #pragma comment( lib, "xml2.lib" )
    #pragma comment( lib, "expat.lib" )
    #pragma comment( lib, "zdll.lib" )
#endif


GVC_t *gvContext(void)
{
    GVC_t *gvc;

#ifndef WITH_CGRAPH
    aginit();
    agnodeattr(NULL, "label", NODENAME_ESC);
#else
    agattr(NULL, AGNODE, "label", NODENAME_ESC);
#endif
    /* default to no builtins, demand loading enabled */
    gvc = gvNEWcontext(NULL, TRUE);
    gvconfig(gvc, FALSE); /* configure for available plugins */
    return gvc;
}

GVC_t *gvContextPlugins(const lt_symlist_t *builtins, int demand_loading)
{
    GVC_t *gvc;

#ifndef WITH_CGRAPH
    aginit();
    agnodeattr(NULL, "label", NODENAME_ESC);
#else
    agattr(NULL, AGNODE, "label", NODENAME_ESC);
#endif
    gvc = gvNEWcontext(builtins, demand_loading);
    gvconfig(gvc, FALSE); /* configure for available plugins */
    return gvc;
}



/* gvLayout:
 * Selects layout based on engine and binds it to gvc;
 * does the layout and sets the graph's bbox.
 * Return 0 on success.
 */
int gvLayout(GVC_t *gvc, graph_t *g, const char *engine)
{
    char buf[256];
    int rc;

    rc = gvlayout_select(gvc, engine);
    if (rc == NO_SUPPORT) {
        agerr (AGERR, "Layout type: \"%s\" not recognized. Use one of:%s\n",
                engine, gvplugin_list(gvc, API_layout, engine));
        return -1;
    }

    gvLayoutJobs(gvc, g);

/* set bb attribute for basic layout.
 * doesn't yet include margins, scaling or page sizes because
 * those depend on the renderer being used. */
    if (GD_drawing(g)->landscape)
        sprintf(buf, "%d %d %d %d",
                ROUND(GD_bb(g).LL.y), ROUND(GD_bb(g).LL.x),
                ROUND(GD_bb(g).UR.y), ROUND(GD_bb(g).UR.x));
    else
        sprintf(buf, "%d %d %d %d",
                ROUND(GD_bb(g).LL.x), ROUND(GD_bb(g).LL.y),
                ROUND(GD_bb(g).UR.x), ROUND(GD_bb(g).UR.y));
    agsafeset(g, "bb", buf, "");

    return 0;
}

/* Render layout in a specified format to an open FILE */
int gvRender(GVC_t *gvc, graph_t *g, const char *format, FILE *out)
{
    int rc;
    GVJ_t *job;

    g = g->root;

    /* create a job for the required format */
    rc = gvjobs_output_langname(gvc, format);
    job = gvc->job;
    if (rc == NO_SUPPORT) {
        agerr (AGERR, "Format: \"%s\" not recognized. Use one of:%s\n",
                format, gvplugin_list(gvc, API_device, format));
        return -1;
    }

    job->output_lang = gvrender_select(job, job->output_langname);
    if (!GD_drawing(g) && !(job->flags & LAYOUT_NOT_REQUIRED)) {
	fprintf(stderr, "Layout was not done\n");
	return -1;
    }
    job->output_file = out;
    if (out == NULL)
	job->flags |= OUTPUT_NOT_REQUIRED;
    rc = gvRenderJobs(gvc, g);
    gvrender_end_job(job);
    gvjobs_delete(gvc);

    return rc;
}

/* Render layout in a specified format to an open FILE */
int gvRenderFilename(GVC_t *gvc, graph_t *g, const char *format, const char *filename)
{
    int rc;
    GVJ_t *job;

    g = g->root;

    /* create a job for the required format */
    rc = gvjobs_output_langname(gvc, format);
    job = gvc->job;
    if (rc == NO_SUPPORT) {
	agerr(AGERR, "Format: \"%s\" not recognized. Use one of:%s\n",
                format, gvplugin_list(gvc, API_device, format));
	return -1;
    }

    job->output_lang = gvrender_select(job, job->output_langname);
    if (!GD_drawing(g) && !(job->flags & LAYOUT_NOT_REQUIRED)) {
	fprintf(stderr, "Layout was not done\n");
	return -1;
    }
    gvjobs_output_filename(gvc, filename);
    rc = gvRenderJobs(gvc, g);
    gvrender_end_job(job);
    gvdevice_finalize(job);
    gvjobs_delete(gvc);

    return rc;
}

/* Render layout in a specified format to an external context */
int gvRenderContext(GVC_t *gvc, graph_t *g, const char *format, void *context)
{
    int rc;
    GVJ_t *job;
	
    g = g->root;
	
    /* create a job for the required format */
    rc = gvjobs_output_langname(gvc, format);
    job = gvc->job;
    if (rc == NO_SUPPORT) {
		agerr(AGERR, "Format: \"%s\" not recognized. Use one of:%s\n",
			  format, gvplugin_list(gvc, API_device, format));
		return -1;
    }
	
    job->output_lang = gvrender_select(job, job->output_langname);
    if (!GD_drawing(g) && !(job->flags & LAYOUT_NOT_REQUIRED)) {
		fprintf(stderr, "Layout was not done\n");
		return -1;
    }
	
    job->context = context;
    job->external_context = TRUE;
	
    rc = gvRenderJobs(gvc, g);
    gvrender_end_job(job);
    gvdevice_finalize(job);
    gvjobs_delete(gvc);
	
    return rc;
}

/* Render layout in a specified format to a malloc'ed string */
int gvRenderData(GVC_t *gvc, graph_t *g, const char *format, char **result, unsigned int *length)
{
    int rc;
    GVJ_t *job;

    g = g->root;

    /* create a job for the required format */
    rc = gvjobs_output_langname(gvc, format);
    job = gvc->job;
    if (rc == NO_SUPPORT) {
	agerr(AGERR, "Format: \"%s\" not recognized. Use one of:%s\n",
                format, gvplugin_list(gvc, API_device, format));
	return -1;
    }

    job->output_lang = gvrender_select(job, job->output_langname);
    if (!GD_drawing(g) && !(job->flags & LAYOUT_NOT_REQUIRED)) {
	fprintf(stderr, "Layout was not done\n");
	return -1;
    }

/* page size on Linux, Mac OS X and Windows */
#define OUTPUT_DATA_INITIAL_ALLOCATION 4096

    if(!result || !(*result = malloc(OUTPUT_DATA_INITIAL_ALLOCATION))) {
	agerr(AGERR, "failure malloc'ing for result string");
	return -1;
    }

    job->output_data = *result;
    job->output_data_allocated = OUTPUT_DATA_INITIAL_ALLOCATION;
    job->output_data_position = 0;

    rc = gvRenderJobs(gvc, g);
    gvrender_end_job(job);

    if (rc == 0) {
	*result = job->output_data;
	*length = job->output_data_position;
    }
    gvjobs_delete(gvc);

    return rc;
}

/* gvFreeRenderData:
 * Utility routine to free memory allocated in gvRenderData, as the application code may use
 * a different runtime library.
 */
void gvFreeRenderData (char* data)
{
    free (data);
}

void gvAddLibrary(GVC_t *gvc, gvplugin_library_t *lib)
{
    gvconfig_plugin_install_from_library(gvc, NULL, lib);
}

char **gvcInfo(GVC_t* gvc) { return gvc->common.info; }
char *gvcVersion(GVC_t* gvc) { return gvc->common.info[1]; }
char *gvcBuildDate(GVC_t* gvc) { return gvc->common.info[2]; }
