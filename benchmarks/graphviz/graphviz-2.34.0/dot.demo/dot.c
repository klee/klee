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

#include <gvc.h>

int main(int argc, char **argv)
{
    graph_t *g, *prev = NULL;
    GVC_t *gvc;

    gvc = gvContext();
    gvParseArgs(gvc, argc, argv);

    while ((g = gvNextInputGraph(gvc))) {
	if (prev) {
	    gvFreeLayout(gvc, prev);
	    agclose(prev);
	}
	gvLayoutJobs(gvc, g);
	gvRenderJobs(gvc, g);
	prev = g;
    }
    return (gvFreeContext(gvc));
}
