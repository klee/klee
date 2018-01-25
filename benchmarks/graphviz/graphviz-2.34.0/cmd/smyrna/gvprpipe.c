/* $Id$Revision: */
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

#include "smyrnadefs.h"
#include "gvprpipe.h"
#include "const.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <glade/glade.h>
#include <gtk/gtk.h>
#include "draw.h"
#include "gui.h"
#include "topviewsettings.h"
/* #include "topview.h" */
#include <viewport.h>
//#include <gltemplate.h> 

#include <gvpr.h>
extern GladeXML *xml;		//global libglade vars

/*    typedef struct {
	char* def;
	char *script;
	char *args;
	char *attr_name;	
	void* obj;
	gvpr_arg_type arg_type;
    } gvprscript;*/
    //_on_click="(gvpr_no_arg)N{node.color="red"){N.color="blue"}";


/*arg_type get_arg_type(char* str)
{
    


}*/

#if 0
gvprscript* scr_from_string(char* str,void* obj)
{

    return NULL;
}
#endif


static ssize_t outfn(void *sp, const char *buf, size_t nbyte, void *dp)
{

    append_textview((GtkTextView *)
		    glade_xml_get_widget(xml, "gvprtextoutput"), buf,
		    nbyte);
    append_textview((GtkTextView *)
		    glade_xml_get_widget(xml, "mainconsole"), buf, nbyte);
    return (ssize_t)nbyte;
}

#ifdef UNUSED
static ssize_t errfn(void *sp, const char *buf, size_t nbyte, void *dp)
{
    return 0;
}
static void set_refresh(ViewInfo* v,char* script)
{
    if(strstr(script,"pos"))
	v->refresh.pos=1;
    if(strstr(script,"color"))
	v->refresh.color=1;
    if(strstr(script,"visible"))
	v->refresh.visibility=1;
    if(strstr(script,"size"))
	v->refresh.nodesize=1;
    if(strstr(script,"selected"))
	v->refresh.selection=1;
}
#endif

int run_gvpr(Agraph_t * srcGraph, int argc, char *argv[])
{
    int i, rv = 1;
    gvpropts opts;
    Agraph_t *gs[2];
    static int count;
    char buf[SMALLBUF];

    gs[0] = srcGraph;
    gs[1] = 0;
    memset (&opts, 0, sizeof(opts));
    opts.ingraphs = gs;
    opts.out = outfn;
    opts.err = outfn;
    opts.flags = GV_USE_OUTGRAPH;

    rv = gvpr(argc, argv, &opts);

    if (rv) {			/* error */
	fprintf(stderr, "Error in gvpr\n");
    } else if (opts.n_outgraphs) 
    {
	refreshViewport(0);
	sprintf(buf, "<%d>", ++count);
	if (opts.outgraphs[0] != view->g[view->activeGraph])
	    add_graph_to_viewport(opts.outgraphs[0], buf);
	if (opts.n_outgraphs > 1)
	    fprintf(stderr, "Warning: multiple output graphs-discarded\n");
	for (i = 1; i < opts.n_outgraphs; i++) {
	    agclose(opts.outgraphs[i]);
	}
    } else 
    { 
	/* set_refresh(view,argv[1]); */
	updateRecord (srcGraph);
        update_graph_from_settings(srcGraph);
//	update_topview(srcGraph, view->Topview, 0);
    }
    return rv;
}
