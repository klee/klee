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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "gml2gv.h"
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#else
#include "compat_getopt.h"
#endif

#define N_NEW(n,t)       (t*)malloc((n)*sizeof(t))

static int Verbose;
static char* gname = "";
static FILE *outFile;
static char *CmdName;
static char **Files;
#ifdef WIN32 //*dependencies
    #pragma comment( lib, "cgraph.lib" )
    #pragma comment( lib, "cdt.lib" )
    #pragma comment( lib, "ingraphs.lib" )
    #pragma comment( lib, "gvc.lib" )
#endif

static FILE *getFile(void)
{
    FILE *rv = NULL;
    static FILE *savef = NULL;
    static int cnt = 0;

    if (Files == NULL) {
	if (cnt++ == 0) {
	    rv = stdin;
	}
    } else {
	if (savef)
	    fclose(savef);
	while (Files[cnt]) {
	    if ((rv = fopen(Files[cnt++], "r")) != 0)
		break;
	    else
		fprintf(stderr, "Can't open %s\n", Files[cnt - 1]);
	}
    }
    savef = rv;
    return rv;
}

static FILE *openFile(char *name, char *mode)
{
    FILE *fp;
    char *modestr;

    fp = fopen(name, mode);
    if (!fp) {
        if (*mode == 'r')
            modestr = "reading";
        else
            modestr = "writing";
        fprintf(stderr, "%s: could not open file %s for %s\n",
                CmdName, name, modestr);
        perror(name);
        exit(1);
    }
    return fp;
}


static char *useString = "Usage: %s [-v?] [-g<name>] [-o<file>] <files>\n\
  -g<name>  : use <name> as template for graph names\n\
  -v        : verbose mode\n\
  -o<file>  : output to <file> (stdout)\n\
  -?        : print usage\n\
If no files are specified, stdin is used\n";

static void usage(int v)
{
    printf(useString, CmdName);
    exit(v);
}

static char *cmdName(char *path)
{
    char *sp;

    sp = strrchr(path, '/');
    if (sp)
        sp++;
    else
        sp = path;
    return sp;
}

static void initargs(int argc, char **argv)
{
    int c;

    CmdName = cmdName(argv[0]);
    opterr = 0;
    while ((c = getopt(argc, argv, ":g:vo:")) != -1) {
	switch (c) {
	case 'g':
	    gname = optarg;
	    break;
	case 'v':
	    Verbose = 1;
	    break;
	case 'o':
	    outFile = openFile(optarg, "w");
	    break;
	case ':':
	    fprintf(stderr, "%s: option -%c missing argument\n", CmdName, optopt);
	    usage(1);
	    break;
	case '?':
	    if (optopt == '?')
		usage(0);
	    else {
		fprintf(stderr, "%s: option -%c unrecognized\n", CmdName,
			optopt);
		usage(1);
	    }
	}
    }

    argv += optind;
    argc -= optind;

    if (argc)
	Files = argv;
    if (!outFile)
	outFile = stdout;
}

static char*
nameOf (char* name, int cnt)
{
    static char* buf = 0;

    if (*name == '\0')
	return name;
    if (cnt) {
	if (!buf)
	    buf = N_NEW (strlen(name)+32,char);  /* 32 to handle any integer plus null byte */
	sprintf (buf, "%s%d", name, cnt);
	return buf;
    }
    else
	return name;
}

int main(int argc, char **argv)
{
    Agraph_t *G;
    Agraph_t *prev = 0;
    FILE *inFile;
    int gcnt, cnt, rv;

    rv = 0;
    gcnt = 0;
    initargs(argc, argv);
    while ((inFile = getFile())) {
	cnt = 0;
	while ((G = gml_to_gv(nameOf(gname, gcnt), inFile, cnt, &rv))) {
	    cnt++;
	    gcnt++;
	    if (prev)
		agclose(prev);
	    prev = G;
	    if (Verbose) 
		fprintf (stderr, "%s: %d nodes %d edges\n",
		    agnameof (G), agnnodes(G), agnedges(G));
	    agwrite(G, outFile);
	    fflush(outFile);
	}
    }
    exit(rv);
}

