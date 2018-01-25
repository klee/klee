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

#include "cgraph.h"
#include "ingraphs.h"
#include <stdio.h>
#include <stdlib.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#else
#include "compat_getopt.h"
#endif

char **Files;
int chkOnly;

static char *useString = "Usage: nop [-p?] <files>\n\
  -p - check for valid DOT\n\
  -? - print usage\n\
If no files are specified, stdin is used\n";

#ifdef WIN32 //*dependencies
    #pragma comment( lib, "cgraph.lib" )
    #pragma comment( lib, "ingraphs.lib" )
#endif

static void usage(int v)
{
    printf("%s",useString);
    exit(v);
}

static void init(int argc, char *argv[])
{
    int c;

    opterr = 0;
    while ((c = getopt(argc, argv, "p")) != -1) {
	switch (c) {
	case 'p':
	    chkOnly = 1;
	    break;
	case '?':
	    if (optopt == '?')
		usage(0);
	    else
		fprintf(stderr, "nop: option -%c unrecognized - ignored\n",
			optopt);
	    break;
	}
    }
    argv += optind;
    argc -= optind;

    if (argc)
	Files = argv;
}

static Agraph_t *gread(FILE * fp)
{
    return agread(fp, (Agdisc_t *) 0);
}

int main(int argc, char **argv)
{
    Agraph_t *g;
    ingraph_state ig;

    init(argc, argv);
    newIngraph(&ig, Files, gread);

    while ((g = nextGraph(&ig)) != 0) {
	if (!chkOnly) agwrite(g, stdout);
	agclose(g);
    }

    return(ig.errors | agerrors());
}
