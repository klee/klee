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

#include <stdio.h>
#include <stdlib.h>
#define STANDALONE
#include "general.h"
#include "QuadTree.h"
#include <time.h>
#include "SparseMatrix.h"
#ifdef HAVE_GETOPT_H
#include <getopt.h>
#else
#include "compat_getopt.h"
#endif
#include "string.h"
#include "make_map.h"
#include "spring_electrical.h"
#include "post_process.h"
#include "overlap.h"
#include "clustering.h"
#include "ingraphs.h"
#include "DotIO.h"
#include "colorutil.h"

enum {POINTS_ALL = 1, POINTS_LABEL, POINTS_RANDOM};
enum {maxlen = 10000000};
enum {MAX_GRPS = 10000};

typedef struct {
  FILE* outfp;
  char** infiles;
  int maxcluster;
  int clustering_method;
} opts_t;

#if 0
void *gmalloc(size_t nbytes)
{
    char *rv;
    if (nbytes == 0)
        return NULL;
    rv = malloc(nbytes);
    if (rv == NULL) {
        fprintf(stderr, "out of memory\n");
        abort();
    }
    return rv;
}

void *grealloc(void *ptr, size_t size)
{
    void *p = realloc(ptr, size);
    if (p == NULL && size) {
        fprintf(stderr, "out of memory\n");
        abort();
    }
    return p;
}

#endif

static char* usestr =
"    -C k - generate no more than k clusters (0)\n\
       0 : no limit\n\
    -c k - use clustering method k (0)\n\
       0 : use modularity\n\
       1 : use modularity quality\n\
    -o <outfile> - output file (stdout)\n\
    -v   - verbose mode\n\
    -?   - print usage\n";

static void usage(char* cmd, int eval)
{
    fprintf(stderr, "Usage: %s <options> graphfile\n", cmd);
    fputs (usestr, stderr);
    exit(eval);
}

static FILE *openFile(char *name, char *mode, char* cmd)
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
		cmd, name, modestr);
	exit(-1);
    }
    return (fp);
}

static void init(int argc, char *argv[], opts_t* opts) {
  char* cmd = argv[0];
  unsigned int c;
  int v;

  opts->maxcluster = 0;
  opts->outfp = stdout;
  Verbose = 0;

  opts->clustering_method =  CLUSTERING_MODULARITY;
  while ((c = getopt(argc, argv, ":vC:c:o:")) != -1) {
    switch (c) {
    case 'c':
      if ((sscanf(optarg,"%d", &v) == 0) || (v < 0)) {
	usage(cmd,1);
      }
      else opts->clustering_method = v;
      break;
    case 'C':
      if ((sscanf(optarg,"%d",&v) == 0) || (v < 0)) {
	usage(cmd,1);
      }
      else opts->maxcluster = v;
      break;
    case 'o':
      opts->outfp = openFile(optarg, "w", cmd);
      break;
    case 'v':
      Verbose = 1;
      break;
    case '?':
      if (optopt == '?')
	usage(cmd, 0);
      else {
	fprintf(stderr, " option -%c unrecognized - ignored\n",
		optopt);
	usage(cmd, 0);
      }
      break;
    }
  }

  argv += optind;
  argc -= optind;
  if (argc)
    opts->infiles = argv;
  else
    opts->infiles = NULL;
}

static Agraph_t *gread(FILE * fp)
{
    return agread(fp, (Agdisc_t *) 0);
}

void clusterGraph (Agraph_t* g, int maxcluster, int clustering_method){
  initDotIO(g);
  attached_clustering(g, maxcluster, clustering_method);
  return;

}

int main(int argc, char *argv[])
{
  Agraph_t *g = 0, *prevg = 0;
  ingraph_state ig;
  opts_t opts;

  init(argc, argv, &opts);

  newIngraph (&ig, opts.infiles, gread);

  while ((g = nextGraph (&ig)) != 0) {
    if (prevg) agclose (prevg);
    clusterGraph (g, opts.maxcluster, opts.clustering_method);
    agwrite(g, opts.outfp);
    prevg = g;
  }

  return 0; 
}
