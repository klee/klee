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
#include <pack.h>

int main (int argc, char* argv[])
{
    graph_t *g;
    graph_t *sg;
    FILE *fp;
    graph_t** cc;
    int       i, ncc;
    GVC_t *gvc;

    gvc = gvContext();

    if (argc > 1)
        fp = fopen(argv[1], "r");
    else
        fp = stdin;
#ifdef WITH_CGRAPH
    g = agread(fp, 0);
#else
    g = agread(fp);
#endif

    cc = ccomps(g, &ncc, (char*)0);

    for (i = 0; i < ncc; i++) {
        sg = cc[i];
        nodeInduce (sg);
        gvLayout(gvc, sg, "neato");
    }
    pack_graph (ncc, cc, g, 0);

    gvRender(gvc, g, "ps", stdout);

    for (i = 0; i < ncc; i++) {
        sg = cc[i];
        gvFreeLayout(gvc, sg);
        agdelete(g, sg);
    }

    agclose(g);

    return (gvFreeContext(gvc));

}
