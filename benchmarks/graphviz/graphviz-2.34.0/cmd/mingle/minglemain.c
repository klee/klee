
/*************************************************************************
 * Copyright (c) 2011 AT&T Intellectual Property 
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 *************************************************************************/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef WIN32 /*dependencies*/
    #pragma comment( lib, "cgraph.lib" )
    #pragma comment( lib, "gvc.lib" )
    #pragma comment( lib, "ingraphs.lib" )
    #pragma comment( lib, "sparse.lib" )
    #pragma comment( lib, "sfdp.lib" )
    #pragma comment( lib, "minglelib.lib" )
    #pragma comment( lib, "neatogen.lib" )
    #pragma comment( lib, "rbtree.lib" )
    #pragma comment( lib, "cdt.lib" )
#endif   /* not WIN32_DLL */

#include <cgraph.h>
#include <agxbuf.h>
#include <ingraphs.h>
#include <pointset.h>
#ifdef HAVE_GETOPT_H
#include <getopt.h>
#else
#include "compat_getopt.h"
#endif

#include "DotIO.h"
#include "edge_bundling.h"
#include "nearest_neighbor_graph.h"

typedef enum {
	FMT_GV,
	FMT_SIMPLE,
} fmt_t;

typedef struct {
	Agrec_t hdr;
	int idx;
} etoi_t;

#define ED_idx(e) (((etoi_t*)AGDATA(e))->idx)

typedef struct {
	int outer_iter;
	int method; 
	int compatibility_method;
	real K;
	fmt_t fmt;
	int nneighbors;
	int max_recursion;
	real angle_param;
	real angle;
} opts_t;

static char *fname;
static FILE *outfile;

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

static char* use_msg =
"Usage: mingle <options> <file>\n\
    -a t - max. turning angle [0-180] (40)\n\
    -c i - compatability measure; 0 : distance, 1: full (default)\n\
    -i iter: number of outer iterations/subdivisions (4)\n\
    -k k - number of neighbors in the nearest neighbor graph of edges (10)\n\
    -K k - the force constant\n\
    -m method - method used. 0 (force directed), 1 (agglomerative ink saving, default), 2 (cluster+ink saving)\n\
    -o fname - write output to file fname (stdout)\n\
    -p t - balance for avoiding sharp angles\n\
           The larger the t, the more sharp angles are allowed\n\
    -r R - max. recursion level with agglomerative ink saving method (100)\n\
    -T fmt - output format: gv (default) or simple\n\
    -v - verbose\n";

static void
usage (int eval)
{
	fputs (use_msg, stderr);
	if (eval >= 0) exit (eval);
}

/* checkG:
 * Return non-zero if g has loops or multiedges.
 * Relies on multiedges occurring consecutively in edge list.
 */
static int
checkG (Agraph_t* g)
{
	Agedge_t* e;
	Agnode_t* n;
	Agnode_t* h;
	Agnode_t* prevh = NULL;

	for (n = agfstnode (g); n; n = agnxtnode (g, n)) {
		for (e = agfstout (g, n); e; e = agnxtout (g, e)) {
			if ((h = aghead(e)) == n) return 1;   // loop
			if (h == prevh) return 1;            // multiedge
			prevh = h;
		}
		prevh = NULL;  // reset
	}
	return 0;
}

static void init(int argc, char *argv[], opts_t* opts)
{
	unsigned int c;
	char* CmdName = argv[0];
	real s;
    int i;

    opterr = 0;
	opts->outer_iter = 4;
	opts->method = METHOD_INK_AGGLOMERATE;
	opts->compatibility_method = COMPATIBILITY_FULL;
	opts->K = -1;
	opts->fmt = FMT_GV;
	opts->nneighbors = 10;
	opts->max_recursion = 100;
	opts->angle_param = -1;
	opts->angle = 40.0/180.0*M_PI;

	while ((c = getopt(argc, argv, ":a:c:i:k:K:m:o:p:r:T:v:")) != -1) {
		switch (c) {
		case 'a':
            if ((sscanf(optarg,"%lf",&s) > 0) && (s >= 0))
				opts->angle =  M_PI*s/180;
			else 
				fprintf (stderr, "-a arg %s must be positive real - ignored\n", optarg); 
			break;
		case 'c':
            if ((sscanf(optarg,"%d",&i) > 0) && (0 <= i) && (i <= COMPATIBILITY_FULL))
				opts->compatibility_method =  i;
			else 
				fprintf (stderr, "-c arg %s must be an integer in [0,%d] - ignored\n", optarg, COMPATIBILITY_FULL); 
			break;
		case 'i':
            if ((sscanf(optarg,"%d",&i) > 0) && (i >= 0))
				opts->outer_iter =  i;
			else 
				fprintf (stderr, "-i arg %s must be a non-negative integer - ignored\n", optarg); 
			break;
		case 'k':
            if ((sscanf(optarg,"%d",&i) > 0) && (i >= 2))
				opts->nneighbors =  i;
			else 
				fprintf (stderr, "-k arg %s must be an integer >= 2 - ignored\n", optarg); 
			break;
		case 'K':
            if ((sscanf(optarg,"%lf",&s) > 0) && (s > 0))
				opts->K =  s;
			else 
				fprintf (stderr, "-K arg %s must be positive real - ignored\n", optarg); 
			break;
		case 'm':
            if ((sscanf(optarg,"%d",&i) > 0) && (0 <= i) && (i <= METHOD_INK))
				opts->method =  i;
			else 
				fprintf (stderr, "-k arg %s must be an integer >= 2 - ignored\n", optarg); 
			break;
		case 'o':
			outfile = openFile(optarg, "w", CmdName);
			break;
		case 'p':
            if ((sscanf(optarg,"%lf",&s) > 0))
				opts->angle_param =  s;
			else 
				fprintf (stderr, "-p arg %s must be real - ignored\n", optarg); 
			break;
		case 'r':
            if ((sscanf(optarg,"%d",&i) > 0) && (i >= 0))
				opts->max_recursion =  i;
			else 
				fprintf (stderr, "-r arg %s must be a non-negative integer - ignored\n", optarg); 
			break;
		case 'T':
			if ((*optarg == 'g') && ((*(optarg+1) == 'v'))) 
				opts->fmt = FMT_GV;
			else if ((*optarg == 's') && (!strcmp(optarg+1,"imple"))) 
				opts->fmt = FMT_SIMPLE;
			else
				fprintf (stderr, "-T arg %s must be \"gv\" or \"simple\" - ignored\n", optarg); 
			break;
		case 'v':
			Verbose = 1;
            if ((sscanf(optarg,"%d",&i) > 0) && (i >= 0))
				Verbose =  i;
			else 
				optind--;
			break;
		case ':':
			if (optopt == 'v')
				Verbose = 1;
			else {
				fprintf(stderr, "%s: option -%c missing argument\n", CmdName, optopt);
				usage(1);
			}
			break;
		case '?':
			if (optopt == '?')
				usage(0);
			else
				fprintf(stderr, "%s: option -%c unrecognized - ignored\n", CmdName, optopt);
			break;
		}
    }
    argv += optind;
    argc -= optind;

    if (argc > 0)
		Files = argv;
    if (!outfile) outfile = stdout;
    if (Verbose) {
       fprintf (stderr, "Mingle params:\n");
       fprintf (stderr, "  outer_iter = %d\n", opts->outer_iter);
       fprintf (stderr, "  method = %d\n", opts->method);
       fprintf (stderr, "  compatibility_method = %d\n", opts->compatibility_method);
       fprintf (stderr, "  K = %.02f\n", opts->K);
       fprintf (stderr, "  fmt = %s\n", (opts->fmt?"simple":"gv"));
       fprintf (stderr, "  nneighbors = %d\n", opts->nneighbors);
       fprintf (stderr, "  max_recursion = %d\n", opts->max_recursion);
       fprintf (stderr, "  angle_param = %.02f\n", opts->angle_param);
       fprintf (stderr, "  angle = %.02f\n", 180*opts->angle/M_PI);
    }
}

/* genBundleSpline:
 * We have ninterval+1 points. We drop the ninterval-1 internal points, and add 4 points to the first
 * and last intervals, and 3 to the rest, giving the needed 3*ninterval+4 points.
 */
static void
genBundleSpline (pedge edge, agxbuf* xb)
{
	int k, j, mm, kk;
	char buf[BUFSIZ];
	int dim = edge->dim;
	real* x = edge->x;
	real tt1[3]={0.15,0.5,0.85};
	real tt2[4]={0.15,0.4,0.6,0.85};
	real t, *tt;

	for (j = 0; j < edge->npoints; j++){
		if (j != 0) {
			agxbputc(xb, ' ');
			if ((j == 1) || (j == edge->npoints - 1)) {
				tt = tt2;
				mm = 4;
			} else {
				tt = tt1;
				mm = 3;
			}
			for (kk = 1; kk <= mm; kk++){
				t = tt[kk-1];
				for (k = 0; k < dim; k++) {
					if (k != 0) agxbputc(xb,',');
					sprintf(buf, "%.03f", (x[(j-1)*dim+k]*(1-t)+x[j*dim+k]*(t)));
					agxbput(xb, buf);
				}
				agxbputc(xb,' ');
			}
		}
		if ((j == 0) || (j == edge->npoints - 1)) {
			for (k = 0; k < dim; k++) {
				if (k != 0) agxbputc(xb,',');
				sprintf(buf, "%.03f", x[j*dim+k]);
				agxbput(xb, buf);
			}
		}
    }
}

static void
genBundleInfo (pedge edge, agxbuf* xb)
{
	int k, j;
	char buf[BUFSIZ];
	int dim = edge->dim;
	real* x = edge->x;

	for (j = 0; j < edge->npoints; j++){
		if (j != 0)  agxbputc(xb, ':');
		for (k = 0; k < dim; k++) {
			if (k != 0)  agxbputc(xb, ',');
			sprintf(buf, "%.03f", x[j*dim+k]);
			agxbput(xb, buf);
		}

		if ((j < edge->npoints-1) && (edge->wgts))  {
        	sprintf(buf, ";%.03f", edge->wgts[j]);
			agxbput(xb, buf);
		}
	}
}

static void
genBundleColors (pedge edge, agxbuf* xb, real maxwgt)
{
	int k, j, r, g, b;
	real len, t, len_total0 = 0;
	int dim = edge->dim;
	real* x = edge->x;
	real* lens = MALLOC(sizeof(real)*edge->npoints);
	char buf[BUFSIZ];

	for (j = 0; j < edge->npoints - 1; j++){
		len = 0;
		for (k = 0; k < dim; k++){
			len += (x[dim*j+k] - x[dim*(j+1)+k])*(x[dim*j+k] - x[dim*(j+1)+k]);
		}
		lens[j] = len = sqrt(len/k);
		len_total0 += len;
	}
	for (j = 0; j < edge->npoints - 1; j++){
		t = edge->wgts[j]/maxwgt;
		/* interpolate between red (t = 1) to blue (t = 0) */
		r = 255*t; g = 0; b = 255*(1-t);
		if (j != 0) agxbputc(xb,':');
		sprintf(buf, "#%02x%02x%02x%02x", r, g, b, 85);
		agxbput(xb, buf);
		if (j < edge->npoints-2) {
			sprintf(buf,";%f",lens[j]/len_total0);
			agxbput(xb, buf);
		}
	}
	free (lens);
}

static void
export_dot (FILE* fp, int ne, pedge *edges, Agraph_t* g)
{
	Agsym_t* epos = agattr (g, AGEDGE, "pos", "");
	Agsym_t* esects = agattr (g, AGEDGE, "bundle", "");
	Agsym_t* eclrs = NULL;
	Agnode_t* n;
	Agedge_t* e;
	int i, j;
	real maxwgt = 0;
	pedge edge;
	agxbuf xbuf;
	unsigned char buf[BUFSIZ];

	  /* figure out max number of bundled original edges in a pedge */
	for (i = 0; i < ne; i++){
		edge = edges[i];
		if (edge->wgts){
			for (j = 0; j < edge->npoints - 1; j++){
				maxwgt = MAX(maxwgt, edge->wgts[j]);
			}
		}
	}

	agxbinit(&xbuf, BUFSIZ, buf);
	for (n = agfstnode (g); n; n = agnxtnode (g, n)) {
		for (e = agfstout (g, n); e; e = agnxtout (g, e)) {
			edge = edges[ED_idx(e)];

			genBundleSpline (edge, &xbuf);
			agxset (e, epos, agxbuse(&xbuf));

			genBundleInfo (edge, &xbuf);
			agxset (e, esects, agxbuse(&xbuf));

			if (edge->wgts) {
				if (!eclrs) eclrs = agattr (g, AGEDGE, "color", "");
				genBundleColors (edge, &xbuf, maxwgt);
				agxset (e, eclrs, agxbuse(&xbuf));
			}
		}
	}
	agxbfree(&xbuf);
	agwrite(g, fp);
}

static int
bundle (Agraph_t* g, opts_t* opts)
{
	real *x = NULL;
	real *label_sizes = NULL;
	int n_edge_label_nodes;
	int dim = 2;
	SparseMatrix A;
	SparseMatrix B;
	pedge* edges;
    real *xx, eps = 0.;
    int nz = 0;
    int *ia, *ja, i, j, k;
	int rv = 0;

	if (checkG(g)) {
		agerr (AGERR, "Graph %s (%s) contains loops or multiedges\n");
		return 1;
	}
    initDotIO(g);
	A = SparseMatrix_import_dot(g, dim, &label_sizes, &x, &n_edge_label_nodes, NULL, FORMAT_CSR, NULL);
	if (!A){
		agerr (AGERR, "Error: could not convert graph %s (%s) into matrix\n", agnameof(g), fname);
		return 1;
    }
    if (x == NULL) {
		agerr (AGPREV, " in file %s\n",  fname);
		return 1;
    }

	A = SparseMatrix_symmetrize(A, TRUE);
	if (opts->fmt == FMT_GV) {
		PointMap* pm = newPM();    /* map from node id pairs to edge index */
		Agnode_t* n;
		Agedge_t* e;
		int idx = 0;

		ia = A->ia; ja = A->ja;
		for (i = 0; i < A->m; i++){
			for (j = ia[i]; j < ia[i+1]; j++){
				if (ja[j] > i){
					insertPM (pm, i, ja[j], idx++);
				}
			}
		}
		for (i = 0, n = agfstnode(g); n; n = agnxtnode(g,n)) {
			setDotNodeID(n, i++);
		}
		for (i = 0, n = agfstnode(g); n; n = agnxtnode(g,n)) {
			for (e = agfstout (g, n); e; e = agnxtout (g, e)) {
				i = getDotNodeID (agtail(e));
				j = getDotNodeID (aghead(e));
				if (j < i) {
					k = i;
					i = j;
					j = k;
				}
				k = insertPM (pm, i, j, -1);
				assert (k >= 0);
				agbindrec (e, "info", sizeof(etoi_t), TRUE);
				ED_idx(e) = k;
			}
		}
		freePM (pm);
	}
		
	ia = A->ia; ja = A->ja;
	nz = A->nz;
	xx = MALLOC(sizeof(real)*nz*4);
	nz = 0;
	dim = 4;
	for (i = 0; i < A->m; i++){
		for (j = ia[i]; j < ia[i+1]; j++){
			if (ja[j] > i){
				xx[nz*dim] = x[i*2];
				xx[nz*dim+1] = x[i*2+1];
				xx[nz*dim+2] = x[ja[j]*2];
				xx[nz*dim+3] = x[ja[j]*2+1];
				nz++;
			}
		}
	}
	if (Verbose)
		fprintf(stderr,"n = %d nz = %d\n",A->m, nz);

	B = nearest_neighbor_graph(nz, MIN(opts->nneighbors, nz), dim, xx, eps);

	SparseMatrix_delete(A);
	A = B;
	FREE(x);
	x = xx;

	dim = 2;

	edges = edge_bundling(A, 2, x, opts->outer_iter, opts->K, opts->method, opts->nneighbors, opts->compatibility_method, opts->max_recursion, opts->angle_param, opts->angle, 0);
	
	if (opts->fmt == FMT_GV)
    	export_dot (outfile, A->m, edges, g);
    else
    	pedge_export_gv(outfile, A->m, edges);
	return rv;
}

static Agraph_t *gread(FILE * fp)
{
    return agread(fp, (Agdisc_t *) 0);
}

int main(int argc, char *argv[])
{
	Agraph_t *g;
	Agraph_t *prev = NULL;
	ingraph_state ig;
	int rv = 0;
	opts_t opts;

	init(argc, argv, &opts);
	newIngraph(&ig, Files, gread);

	while ((g = nextGraph(&ig)) != 0) {
		if (prev)
		    agclose(prev);
		prev = g;
		fname = fileName(&ig);
		if (Verbose)
		    fprintf(stderr, "Process graph %s in file %s\n", agnameof(g),
			    fname);
		rv |= bundle (g, &opts);
	}

	return rv;
}
