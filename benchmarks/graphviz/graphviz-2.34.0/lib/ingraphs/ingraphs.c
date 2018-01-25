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
 * Written by Emden Gansner
 */

#include <stdio.h>
#include <stdlib.h>

#define FREE_STATE 1

typedef struct {
    char *dummy;
} Agraph_t;

extern void agsetfile(char *);

#include "ingraphs.h"

/* nextFile:
 * Set next available file.
 * If Files is NULL, we just read from stdin.
 */
static void nextFile(ingraph_state * sp)
{
    void *rv = NULL;
    char *fname;

    if (sp->u.Files == NULL) {
	if (sp->ctr++ == 0) {
	    rv = sp->fns->dflt;
	}
    } else {
	while ((fname = sp->u.Files[sp->ctr++])) {
	    if (*fname == '-') {
		rv = sp->fns->dflt;
		break;
	    } else if ((rv = sp->fns->openf(fname)) != 0)
		break;
	    else {
		fprintf(stderr, "Can't open %s\n", sp->u.Files[sp->ctr - 1]);
		sp->errors++;
	    }
	}
    }
    if (rv)
	agsetfile(fileName(sp));
    sp->fp = rv;
}

/* nextGraph:
 * Read and return next graph; return NULL if done.
 * Read graph from currently open file. If none, open next file.
 */
Agraph_t *nextGraph(ingraph_state * sp)
{
    Agraph_t *g;

    if (sp->ingraphs) {
	g = (Agraph_t*)(sp->u.Graphs[sp->ctr]);
	if (g) sp->ctr++;
	return g;
    }
    if (sp->fp == NULL)
	nextFile(sp);
    g = NULL;

    while (sp->fp != NULL) {
	if ((g = sp->fns->readf(sp->fp)) != 0)
	    break;
	if (sp->u.Files)	/* Only close if not using stdin */
	    sp->fns->closef(sp->fp);
	nextFile(sp);
    }
    return g;
}

/* new_ing:
 * Create new ingraph state. If sp is non-NULL, we
 * assume user is supplying memory.
 */
static ingraph_state*
new_ing(ingraph_state * sp, char **files, Agraph_t** graphs, ingdisc * disc)
{
    if (!sp) {
	sp = (ingraph_state *) malloc(sizeof(ingraph_state));
	if (!sp) {
	    fprintf(stderr, "ingraphs: out of memory\n");
	    return 0;
	}
	sp->heap = 1;
    } else
	sp->heap = 0;
    if (graphs) {
	sp->ingraphs = 1;
	sp->u.Graphs = graphs;
    }
    else {
	sp->ingraphs = 0;
	sp->u.Files = files;
    }
    sp->ctr = 0;
    sp->errors = 0;
    sp->fp = NULL;
    sp->fns = (ingdisc *) malloc(sizeof(ingdisc));
    if (!sp->fns) {
	fprintf(stderr, "ingraphs: out of memory\n");
	if (sp->heap)
	    free(sp);
	return 0;
    }
    if (!disc->openf || !disc->readf || !disc->closef || !disc->dflt) {
	free(sp->fns);
	if (sp->heap)
	    free(sp);
	fprintf(stderr, "ingraphs: NULL field in ingdisc argument\n");
	return 0;
    }
    *sp->fns = *disc;
    return sp;
}

ingraph_state*
newIng(ingraph_state * sp, char **files, ingdisc * disc)
{
    return new_ing(sp, files, 0, disc);
}

/* newIngGraphs:
 * Create new ingraph state using supplied graphs. If sp is non-NULL, we
 * assume user is supplying memory.
 */
ingraph_state*
newIngGraphs(ingraph_state * sp , Agraph_t** graphs, ingdisc *disc)
{
    return new_ing(sp, 0, graphs, disc);
}

static void *dflt_open(char *f)
{
    return fopen(f, "r");
}

static int dflt_close(void *fp)
{
    return fclose((FILE *) fp);
}

typedef Agraph_t *(*xopengfn) (void *);

static ingdisc dflt_disc = { dflt_open, 0, dflt_close, 0 };

/* newIngraph:
 * At present, we require opf to be non-NULL. In
 * theory, we could assume a function agread(FILE*,void*)
 */
ingraph_state *newIngraph(ingraph_state * sp, char **files, opengfn opf)
{
    if (!dflt_disc.dflt)
	dflt_disc.dflt = stdin;
    if (opf)
	dflt_disc.readf = (xopengfn) opf;
    else {
	fprintf(stderr, "ingraphs: NULL graph reader\n");
	return 0;
    }
    return newIng(sp, files, &dflt_disc);
}

/* closeIngraph:
 * Close any open files and free discipline
 * Free sp if necessary.
 */
void closeIngraph(ingraph_state * sp)
{
    if (!sp->ingraphs && sp->u.Files && sp->fp)
	sp->fns->closef(sp->fp);
    free(sp->fns);
    if (sp->heap)
	free(sp);
}

/* fileName:
 * Return name of current file being processed.
 */
char *fileName(ingraph_state * sp)
{
    char *fname;

    if (sp->ingraphs) {
	return "<>";
    }
    else if (sp->u.Files) {
	if (sp->ctr) {
	    fname = sp->u.Files[sp->ctr - 1];
	    if (*fname == '-')
		return "<stdin>";
	    else
		return fname;
	} else
	    return "<>";
    } else
	return "<stdin>";
}

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifndef HAVE_GETOPT_DECL
/*
public domain AT&T getopt source
*/
#include <string.h>

/*LINTLIBRARY*/

#if 0
#define EOF	(-1)
#define ERR(s, c)	if(opterr){\
	char errbuf[2];\
	errbuf[0] = c; errbuf[1] = '\n';\
	(void) write(2, argv[0], (unsigned)strlen(argv[0]));\
	(void) write(2, s, (unsigned)strlen(s));\
	(void) write(2, errbuf, 2);}
#endif
#define ERR(s, c)	if(opterr) fprintf (stderr, "%s%s'%c'\n", argv[0], s, c)

int	opterr = 1;
int	optind = 1;
int	optopt;
char	*optarg;

int
getopt(int argc, char** argv, char* opts)
{
	static int sp = 1;
	register int c;
	register char *cp;

	if(sp == 1) {
		if(optind >= argc ||
		   argv[optind][0] != '-' || argv[optind][1] == '\0')
			return(EOF);
		else if(strcmp(argv[optind], "--") == 0) {
			optind++;
			return(EOF);
		}
	}

	optopt = c = argv[optind][sp];
	if(c == ':' || (cp=strchr(opts, c)) == 0) {
		ERR(": illegal option -- ", c);
		if(argv[optind][++sp] == '\0') {
			optind++;
			sp = 1;
		}
		return('?');
	}
	if(*++cp == ':') {
		if(argv[optind][sp+1] != '\0')
			optarg = &argv[optind++][sp+1];
		else if(++optind >= argc) {
			ERR(": option requires an argument -- ", c);
			sp = 1;
			return('?');
		} else
			optarg = argv[optind++];
		sp = 1;
	} else {
		if(argv[optind][++sp] == '\0') {
			sp = 1;
			optind++;
		}
		optarg = 0;
	}
	return(c);
}
#endif
