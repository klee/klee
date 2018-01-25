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
#ifdef WIN32
#define strdup(x) _strdup(x)
#endif
enum {POINTS_ALL = 1, POINTS_LABEL, POINTS_RANDOM};
enum {maxlen = 10000000};
enum {MAX_GRPS = 10000};
static char swork[maxlen];

#ifdef WIN32
    #pragma comment( lib, "cgraph.lib" )
    #pragma comment( lib, "gvc.lib" )
    #pragma comment( lib, "ingraphs.lib" )
    #pragma comment( lib, "sparse.lib" )
    #pragma comment( lib, "neatogen.lib" )
    #pragma comment( lib, "gvc.lib" )
    #pragma comment( lib, "pathplan.lib" )
    #pragma comment( lib, "neatogen.lib" )
    #pragma comment( lib, "circogen.lib" )
    #pragma comment( lib, "twopigen.lib" )
    #pragma comment( lib, "fdpgen.lib" )
    #pragma comment( lib, "sparse.lib" )
    #pragma comment( lib, "cdt.lib" )
    #pragma comment( lib, "gts.lib" )
    #pragma comment( lib, "glib-2.0.lib" )
    #pragma comment( lib, "vpsc.lib" )
    #pragma comment( lib, "patchwork.lib" )
    #pragma comment( lib, "gvortho.lib" )
    #pragma comment( lib, "sfdp.lib" )
    #pragma comment( lib, "rbtree.lib" )
#endif   /* not WIN32_DLL */



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

typedef struct {
    char* cmd;
    char **infiles; 
    FILE* outfile;
    int dim;
    real shore_depth_tol;
    int nrandom; 
    int show_points; 
    real bbox_margin[2]; 
    int whatout;
    int useClusters;
    int plotedges;
    int color_scheme;
    real line_width;
    char *opacity;
    char *plot_label;
    real *bg_color;
    int improve_contiguity_n;
    int nart;
    int color_optimize;
    int maxcluster;
    int nedgep;
    char *line_color;
    int include_OK_points;
    int highlight_cluster;
    int seed;      /* seed used to calculate Fiedler vector */
} params_t;

int string_split(char *s, char sp, char ***ss0, int *ntokens0){
  /* give a string s ending with a \0, split into an array of strings using separater sp, 
     with \0 inserted. Return 0 if successful, 1 if length of string exceed buffer size */
  int ntokens = 0, len = 0;

  int i;
  char **ss;
  char *swork1 = swork;
  if (strlen(s) > maxlen) {
    swork1 = MALLOC(sizeof(char)*maxlen);
  }

  for (i = 0; i < strlen(s); i++){
    if (s[i] == sp){
      ntokens++;
    } else if (s[i] == '\n' || s[i]== EOF){
      ntokens++;
      break;
    }
  }

  ss = malloc(sizeof(char*)*(ntokens+1));
  ntokens = 0;
  for (i = 0; i < strlen(s); i++){
    if (s[i] == sp){
      swork1[len++] = '\0';
      ss[ntokens] = malloc(sizeof(char)*(len+1));
      strcpy(ss[ntokens], swork1);
      len = 0;
      ntokens++;
      continue;
    } else if (s[i] == '\n' || s[i] == EOF){
      swork1[len++] = '\0';
      ss[ntokens] = malloc(sizeof(char)*(len+1));
      strcpy(ss[ntokens], swork1);
      len = 0;
      ntokens++;
      break;
    }
    swork1[len++] = s[i];
  }

  *ntokens0 = ntokens;

  *ss0 = ss;

  if (swork1 != swork) FREE(swork1);
  return 0;

}

static char* usestr =
"   where graphfile must contain node positions, and widths and heights for each node. No overlap between nodes should be present. Acceptable options are: \n\
    -a k - average number of artificial points added along the bounding box of the labels. If < 0, a suitable value is selected automatically. (-1)\n\
    -b v - polygon line width, with v < 0 for no line. (0)\n\
    -c k - polygon color scheme (1)\n\
       0 : no polygons\n\
       1 : pastel (default)\n\
       2 : blue to yellow\n\
       3 : white to red\n\
       4 : light grey to red\n\
       5 : primary colors\n\
       6 : sequential single hue red \n\
       7 : Adam color scheme\n\
       8 : Adam blend\n\
       9 : sequential single hue lighter red \n\
      10 : light grey\n\
    -c_opacity=xx - 2-character hex string for opacity of polygons\n\
    -C k - generate at most k clusters. (0)\n\
    -d s - seed used to calculate Fielder vector for optimal coloring\n\
    -D   - use top-level cluster subgraphs to specify clustering\n\
    -e   - show edges\n\
    -g c - bounding box color. If not specified, a bounding box is not drawn.\n\
    -h k - number of artificial points added to maintain bridge between endpoints (0)\n\
    -highlight=k - only draw cluster k\n\
    -k   - increase randomesss of boundary\n\
    -l s - specify label\n\
    -m v - bounding box margin. If 0, auto-assigned (0)\n\
    -o <file> - put output in <file> (stdout)\n\
    -O   - do NOT do color assignment optimization that maximizes color difference between neighboring countries\n\
    -p k - show points. (0)\n\
       0 : no points\n\
       1 : all points\n\
       2 : label points\n\
       3 : random/artificial points\n\
    -r k - number of random points k used to define sea and lake boundaries. If 0, auto assigned. (0)\n\
    -s v - depth of the sea and lake shores in points. If 0, auto assigned. (0)\n\
    -t n - improve contiguity up to n times. (0)\n\
    -v   - verbose\n\
    -z c - polygon line color (black)\n";

/* 

   -q f - output format (3)\n\
       0 : Mathematica\n\
       1 : PostScript\n\
       2 : country map\n\
       3 : dot format\n\
*/
    /* e.g., 
       1 [cluster=10, clustercolor="#ff0000"]
       2 [cluster=10]
       (and no ther nodes are in cluster10)

       then since we can only use 1 color for the cluster 10, both 1 and 2 will be colored based on the color of node 2. However if you have

       2 [cluster=10]
       1 [cluster=10, clustercolor="#ff0000"]

       then you get both colored red.
       
    */

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

#define HLPFX "ighlight="
#define N_HLPFX (sizeof(HLPFX)-1)

static void 
init(int argc, char **argv, params_t* pm)
{
  char* cmd = argv[0];
  unsigned int c;
  real s;
  int v, r;
  char stmp[3];  /* two character string plus '\0' */

  pm->outfile = NULL;
  pm->opacity = NULL;
  pm->nrandom = -1;
  pm->dim = 2;
  pm->shore_depth_tol = 0;
  pm->highlight_cluster = 0;
  pm->useClusters = 0;
  pm->plotedges = 0;
  pm->whatout = 0;
  pm->show_points = 0;
  pm->color_scheme = COLOR_SCHEME_PASTEL; 
  pm->line_width = 0;
  pm->plot_label = NULL;
  pm->bg_color = NULL;
  pm->improve_contiguity_n = 0;
  pm->whatout = OUT_DOT;
  pm->nart = -1;
  pm->color_optimize = 1;
  pm->maxcluster = 0;
  pm->nedgep = 0;

  pm->cmd = cmd;
  pm->infiles = NULL;
  pm->line_color = N_NEW(10,char);
  strcpy(pm->line_color,"#000000");
  pm->include_OK_points = FALSE;
  pm->seed = 123;

  /*  bbox_margin[0] =  bbox_margin[1] = -0.2;*/
  pm->bbox_margin[0] =  pm->bbox_margin[1] = 0;

  opterr = 0;
  while ((c = getopt(argc, argv, ":evODko:m:s:r:p:c:C:l:b:g:t:a:h:z:d:")) != -1) {
    switch (c) {
    case 'm':
      if ((sscanf(optarg,"%lf",&s) > 0) && (s != 0)){
	    pm->bbox_margin[0] =  pm->bbox_margin[1] = s;
      } else {
        usage(cmd, 1);
      }
      break;
#if 0
    case 'q':
      if ((sscanf(optarg,"%d",&r) > 0) && r >=0 && r <=OUT_PROCESSING){
        pm->whatout = r;
      } else {
        usage(cmd,1);
      }
      break;
#endif
    case 's':
      if ((sscanf(optarg,"%lf",&s) > 0)){
        pm->shore_depth_tol = s;
      } else {
        usage(cmd,1);
      }
      break;
    case 'h':
      if ((sscanf(optarg,"%d",&v) > 0)){
        pm->nedgep = MAX(0, v);
      } else if (!strncmp(optarg, HLPFX, N_HLPFX) && (sscanf(optarg+N_HLPFX,"%d",&v) > 0)) {
        pm->highlight_cluster = MAX(0, v);
      } else {
        usage(cmd,1);
      }
      break;
     case 'r':
      if ((sscanf(optarg,"%d",&r) > 0)){
        pm->nrandom = r;
      }
      break;
    case 't':
      if ((sscanf(optarg,"%d",&r) > 0) && r > 0){
        pm->improve_contiguity_n = r;
      }
      break;
    case 'p':
      pm->show_points = 1;
      if ((sscanf(optarg,"%d",&r) > 0)){
        pm->show_points = MIN(3, r);
      }
      break;
    case 'k':
      pm->include_OK_points = TRUE;
      break;
    case 'v':
      Verbose = 1;
      break;
    case 'D':
      pm->useClusters = 1;
      break;
    case 'e':
      pm->plotedges = 1;
      break;
    case 'o':
	  pm->outfile = openFile(optarg, "w", pm->cmd);
      break;
    case 'O':
      pm->color_optimize = 0;
      break;
    case 'a':
      if ((sscanf(optarg,"%d",&r) > 0)){
	    pm->nart = r;
      } else {
	    usage(cmd,1);
      }
      break;
    case 'c':
      if (sscanf(optarg,"_opacity=%2s", stmp) > 0 && strlen(stmp) == 2){
        pm->opacity = strdup(stmp);
      } else if ((sscanf(optarg,"%d",&r) > 0) && r >= COLOR_SCHEME_NONE && r <= COLOR_SCHEME_GREY){
        pm->color_scheme = r;
      } else {
        usage(cmd,1);
      }
      break;
    case 'd':
      if (sscanf(optarg,"%d",&v) <= 0){
        usage(cmd,1);
      }
      else
        pm->seed = v;
      break;
    case 'C':
      if (!((sscanf(optarg,"%d",&v) > 0) && v >= 0)){
        usage(cmd,1);
      }
      else
        pm->maxcluster = v;
      break;
    case 'g': {
      gvcolor_t color;
      if (colorxlate(optarg, &color, RGBA_DOUBLE) == COLOR_OK) {
        if (!pm->bg_color) pm->bg_color = N_NEW(3,real);
        pm->bg_color[0] = color.u.RGBA[0];
        pm->bg_color[1] = color.u.RGBA[1];
        pm->bg_color[2] = color.u.RGBA[2];
      }
      break;
    }
    case 'z': {
      FREE (pm->line_color);
      pm->line_color = strdup (optarg);
      break;
    }
    case 'b':
      if (sscanf(optarg,"%lf",&s) > 0) {
        pm->line_width = s;
      } else {
        fprintf (stderr, "%s: unexpected argument \"%s\" for -b flag\n", cmd, optarg);
      }
      break;
    case 'l':
      if (pm->plot_label) free (pm->plot_label);
      pm->plot_label = strdup (optarg);
      break;
    case ':':
      fprintf(stderr, "gvpack: option -%c missing argument - ignored\n", optopt);
      break;
    case '?':
      if (optopt == '?')
        usage(cmd, 0);
      else {
        fprintf(stderr, " option -%c unrecognized - ignored\n", optopt);
        usage(cmd, 0);
      }
      break;
    }
  }

  argv += optind;
  argc -= optind;
  if (argc)
    pm->infiles = argv;
  if (!pm->outfile)
    pm->outfile = stdout;
}

#if 0
static void get_graph_node_attribute(Agraph_t* g, char *tag, char *format, size_t len, void *x_default_val, int *nn, void *x, int *flag){
  /* read a node attribute. Example
     {real x0[2];
     real *x = NULL;

     x0[0] = x0[1] = 0.;
     get_graph_node_attribute(g, "pos", "%lf,%lf", sizeof(real)*2, x0, &n, &x, &flag)

     or, do not supply x0 if you want error flagged when pos tag is not available:

     get_graph_node_attribute(g, "pos", "%lf,%lf", sizeof(real)*2, NULL, &n, x, &flag);

     assert(!flag);
     FREE(x);
     }
  */
  Agnode_t* n;
  int nnodes;


  *flag = 0;

  if (!g) {
    *flag = -1;
    return;
  }

  nnodes = agnnodes (g);
  *nn = nnodes;
  for (n = agfstnode (g); n; n = agnxtnode (g, n)) {
    if (agget(n, tag)){
      if (strcmp(format,"%s") == 0){
	strcpy(x, agget(n, tag));
      } else {
	sscanf(agget(n, tag), format, x);
      }

    } else if (x_default_val){
      memcpy(x, x_default_val, len);
    } else {
      *flag = -1;
      return;
    }
    x += len;
  }

}
#endif

static int
validateCluster (int n, int* grouping, int clust_num)
{
  int i;
  for (i = 0; i < n; i++) {
      if (grouping[i] == clust_num) return clust_num;
  }
  fprintf (stderr, "Highlighted cluster %d not found - ignored\n", clust_num);
  return 0;
}

static void 
makeMap (SparseMatrix graph, int n, real* x, real* width, int* grouping, 
  char** labels, float* fsz, float* rgb_r, float* rgb_g, float* rgb_b, params_t* pm, Agraph_t* g )
{
  int dim = pm->dim;
  int i, flag = 0;
  SparseMatrix poly_lines, polys, poly_point_map;
  real edge_bridge_tol = 0.;
  int npolys, nverts, *polys_groups, exclude_random;
  real *x_poly, *xcombined;
  enum {max_string_length = 1000};
  enum {MAX_GRPS = 10000};
  SparseMatrix country_graph;
  int improve_contiguity_n = pm->improve_contiguity_n;
#ifdef TIME
  clock_t  cpu;
#endif
  int nr0, nart0;
  int nart, nrandom;

  exclude_random = TRUE;
#if 0
  if (argc >= 2) {
    fp = fopen(argv[1],"r");
    graph = SparseMatrix_import_matrix_market(fp, FORMAT_CSR);
  }

  if (whatout == OUT_M){
    printf("Show[{");
  }
#endif


#ifdef TIME
  cpu = clock();
#endif
  nr0 = nrandom = pm->nrandom; nart0 = nart = pm->nart;
  if (pm->highlight_cluster) {
    pm->highlight_cluster = validateCluster (n, grouping, pm->highlight_cluster);
  }
  make_map_from_rectangle_groups(exclude_random, pm->include_OK_points,
				 n, dim, x, width, grouping, graph, pm->bbox_margin, &nrandom, &nart, pm->nedgep, 
				 pm->shore_depth_tol, edge_bridge_tol, &xcombined, &nverts, &x_poly, &npolys, &poly_lines, 
				 &polys, &polys_groups, &poly_point_map, &country_graph, pm->highlight_cluster, &flag);

  /* compute a good color permutation */
  if (pm->color_optimize && country_graph && rgb_r && rgb_g && rgb_b) 
    map_optimal_coloring(pm->seed, country_graph, rgb_r,  rgb_g, rgb_b);

#ifdef TIME
  fprintf(stderr, "map making time = %f\n",((real) (clock() - cpu)) / CLOCKS_PER_SEC);
#endif


  /* now we check to see if all points in the same group are also in the same polygon, if not, the map is not very
     contiguous so we move point positions to improve contiguity */
  if (graph && improve_contiguity_n) {
    for (i = 0; i < improve_contiguity_n; i++){
      improve_contiguity(n, dim, grouping, poly_point_map, x, graph, width);
      nart = nart0;
      nrandom = nr0;
      make_map_from_rectangle_groups(exclude_random, pm->include_OK_points,
				     n, dim, x, width, grouping, graph, pm->bbox_margin, &nrandom, &nart, pm->nedgep, 
				     pm->shore_depth_tol, edge_bridge_tol, &xcombined, &nverts, &x_poly, &npolys, &poly_lines, 
				     &polys, &polys_groups, &poly_point_map, &country_graph, pm->highlight_cluster, &flag);
    }
    assert(!flag);    
    {
      SparseMatrix D;
      D = SparseMatrix_get_real_adjacency_matrix_symmetrized(graph);
      remove_overlap(dim, D, x, width, 1000, 5000.,
		     ELSCHEME_NONE, 0, NULL, NULL, &flag);
      
      nart = nart0;
      nrandom = nr0;
      make_map_from_rectangle_groups(exclude_random, pm->include_OK_points,
				     n, dim, x, width, grouping, graph, pm->bbox_margin, &nrandom, &nart, pm->nedgep, 
				     pm->shore_depth_tol, edge_bridge_tol, &xcombined, &nverts, &x_poly, &npolys, &poly_lines, 
				     &polys, &polys_groups, &poly_point_map, &country_graph, pm->highlight_cluster, &flag);
    }
    assert(!flag);
    
  }

#if 0
  if (whatout == OUT_DOT){
#endif
    Dot_SetClusterColor(g, rgb_r,  rgb_g,  rgb_b, grouping);
    plot_dot_map(g, n, dim, x, polys, poly_lines, pm->line_width, pm->line_color, x_poly, polys_groups, labels, width, fsz, rgb_r, rgb_g, rgb_b, pm->opacity,
           pm->plot_label, pm->bg_color, (pm->plotedges?graph:NULL), pm->outfile);
#if 0
    }
    goto RETURN;
  }

  if (whatout == OUT_PROCESSING){
    if (plotedges){
      plot_processing_map(g, n, dim, x, polys, poly_lines, line_width, nverts, x_poly, polys_groups, labels, width, fsz, rgb_r, rgb_g, rgb_b, plot_label, bg_color, graph);
    } else {
      plot_processing_map(g, n, dim, x, polys, poly_lines, line_width, nverts, x_poly, polys_groups, labels, width, fsz, rgb_r, rgb_g, rgb_b, plot_label, bg_color, NULL);
    }

    goto RETURN;
  }

  if (whatout == OUT_PS){
    if (plotedges){
      plot_ps_map(n, dim, x, polys, poly_lines, line_width, x_poly, polys_groups, labels, width, fsz, rgb_r, rgb_g, rgb_b, plot_label, bg_color, graph);
    } else {
      plot_ps_map(n, dim, x, polys, poly_lines, line_width, x_poly, polys_groups, labels, width, fsz, rgb_r, rgb_g, rgb_b, plot_label, bg_color, NULL);
    }

    goto RETURN;
  }


  if (whatout == OUT_M_COUNTRY_GRAPH){
    SparseMatrix_print("(*country graph=*)",country_graph);
    goto RETURN;
    
  }

  
  /*plot_polys(FALSE, polys, x_poly, polys_groups, rgb_r, rgb_g, rgb_b);*/
  if (whatout == OUT_M){
    plot_polys(FALSE, polys, x_poly, polys_groups, NULL, NULL, NULL);
    printf(",");

    plot_polys(TRUE, poly_lines, x_poly, polys_groups, NULL, NULL, NULL);
 
    if (show_points){
      if (show_points == POINTS_ALL){
	printf(",");
	plot_points(n + nart + nrandom - 4, dim, xcombined);
      } else if (show_points == POINTS_LABEL){
	printf(",");
	plot_points(n + nart - 4, dim, xcombined);
      } else if (show_points == POINTS_RANDOM){
	printf(",");
	plot_points(MAX(1,nrandom - 4), dim, xcombined+dim*(n+nart));
      } else {
	assert(0);
      }
    }
    if (plotedges){
      printf(",");
      plot_edges(n, dim, x, graph);
    }
    
    if (labels){
      printf(",");
      plot_labels(n, dim, xcombined, labels);
    }
    
    printf("}]\n");
  }

 RETURN:
#endif
  SparseMatrix_delete(polys);
  SparseMatrix_delete(poly_lines);
  SparseMatrix_delete(poly_point_map);
  FREE(xcombined);
  FREE(x_poly);
  FREE(polys_groups);
}


static void mapFromGraph (Agraph_t* g, params_t* pm)
{
    SparseMatrix graph;
  int n;
  real* width = NULL;
  real* x;
  char** labels = NULL;
  int* grouping;
  float* rgb_r;
  float* rgb_g;
  float* rgb_b;
  float* fsz;

  initDotIO(g);
  graph = Import_coord_clusters_from_dot(g, pm->maxcluster, pm->dim, &n, &width, &x, &grouping, 
					   &rgb_r,  &rgb_g,  &rgb_b,  &fsz, &labels, pm->color_scheme, CLUSTERING_MODULARITY, pm->useClusters);
  makeMap (graph, n, x, width, grouping, labels, fsz, rgb_r, rgb_g, rgb_b, pm, g);
}

static Agraph_t *gread(FILE * fp)
{
    return agread(fp, (Agdisc_t *) 0);
}

int main(int argc, char *argv[])
{
  params_t pm;
  Agraph_t* g;
  Agraph_t* prevg = NULL;
  ingraph_state ig;

  init(argc, argv, &pm);

  newIngraph (&ig, pm.infiles, gread);
  while ((g = nextGraph (&ig)) != 0) {
    if (prevg) agclose (prevg);
    mapFromGraph (g, &pm);
    prevg = g;
  }

  return 0; 
}
