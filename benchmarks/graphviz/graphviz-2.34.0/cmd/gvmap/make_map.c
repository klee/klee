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

#define STANDALONE
#include "SparseMatrix.h"
#include "general.h"
#include "QuadTree.h"
#include "string.h"
/* #include "types.h" */
#include <cgraph.h>
#include "make_map.h"
/* #include "ps.h" */
#include "stress_model.h"
#include "country_graph_coloring.h"
#include "colorutil.h"
#include "delaunay.h"

#ifdef SINGLE
#define REAL float
#else /* not SINGLE */
#define REAL double
#endif /* not SINGLE */
/* #include "triangle.h" */

void map_optimal_coloring(int seed, SparseMatrix A, float *rgb_r,  float *rgb_g, float *rgb_b){
  int *p = NULL;
  float *u = NULL;
  int n = A->m;
  int i;
  real norm1;

  country_graph_coloring(seed, A, &p, &norm1);

  rgb_r++; rgb_b++; rgb_g++;/* seems necessary, but need to better think about cases when clusters are not contigous */
  vector_float_take(n, rgb_r, n, p, &u);
  for (i = 0; i < n; i++) rgb_r[i] = u[i];
  vector_float_take(n, rgb_g, n, p, &u);
  for (i = 0; i < n; i++) rgb_g[i] = u[i];
  vector_float_take(n, rgb_b, n, p, &u);
  for (i = 0; i < n; i++) rgb_b[i] = u[i];
  FREE(u);

}

static int get_poly_id(int ip, SparseMatrix point_poly_map){
  return point_poly_map->ja[point_poly_map->ia[ip]];
}
 
void improve_contiguity(int n, int dim, int *grouping, SparseMatrix poly_point_map, real *x, SparseMatrix graph, real *label_sizes){
 /* 
     grouping: which group each of the vertex belongs to
     poly_point_map: a matrix of dimension npolys x (n + nrandom), poly_point_map[i,j] != 0 if polygon i contains the point j.
     .  If j < n, it is the original point, otherwise it is artificial point (forming the rectangle around a label) or random points.
  */
  int i, j, *ia, *ja, u, v;
  //int *ib, *jb;
  real *a;
  SparseMatrix point_poly_map, D;
  real dist;
  int nbad = 0, flag;
  int maxit = 10;
  real tol = 0.001;

  D = SparseMatrix_get_real_adjacency_matrix_symmetrized(graph);

  assert(graph->m == n);
  ia = D->ia; ja = D->ja;
  a = (real*) D->a;

  /* point_poly_map: each row i has only 1 entry at column j, which says that point i is in polygon j */
  point_poly_map = SparseMatrix_transpose(poly_point_map);

  //  ib = point_poly_map->ia;
  //  jb = point_poly_map->ja;


  for (i = 0; i < n; i++){
    u = i;
    for (j = ia[i]; j < ia[i+1]; j++){
      v = ja[j];
      dist = distance_cropped(x, dim, u, v);
      /*
	if ((grouping[u] != grouping[v]) || (get_poly_id(u, point_poly_map) == get_poly_id(v, point_poly_map))){
	a[j] = 1.1*dist;
	} else {
	nbad++;
	a[j] = 0.9*dist;
	}
      */
      if (grouping[u] != grouping[v]){
	a[j] = 1.1*dist;
      }	else if (get_poly_id(u, point_poly_map) == get_poly_id(v, point_poly_map)){
	a[j] = dist;
      } else {
	nbad++;
	a[j] = 0.9*dist;
      }

    }
  }

  if (Verbose || 1) fprintf(stderr,"ratio (edges among discontigous regions vs total edges)=%f\n",((real) nbad)/ia[n]);
  stress_model(dim, D, D, &x, FALSE, maxit, tol, &flag);

  assert(!flag);

  SparseMatrix_delete(D);
  SparseMatrix_delete(point_poly_map);
}

/*
struct color_card{
  int ncolor;
  real *rgb;
};

color_card color_card_new(int ncolor, real *rgb){
  color_card c;

  c->ncolor = ncolor;
  c->rgb = MALLOC(sizeof(real)*ncolor);
  for (i = 0; i < ncolor*3; i++){
    c->rgb[i] = rgb[i];
  }
  return c;
}

void color_card_delete(color_card c){
  FREE(c->rgb);
}

color_card_blend(color_card c, real x){
  real c1[3], c2[3];
  int ista, isto;
  if (x > 1) x = 1;
  if (x < 0) x = 0;
  ista = x*(ncolor-1);
  
}

*/

struct Triangle {
  int vertices[3];/* 3 points */
  real center[2]; /* center of the triangle */
};

static void normal(real v[], real normal[]){
  if (v[0] == 0){
    normal[0] = 1; normal[1] = 0;
  } else {
    normal[0] = -v[1];
    normal[1] = v[0];
  }
  return;
}



static void triangle_center(real x[], real y[], real z[], real c[]){
  /* find the "center" c, which is the intersection of the 3 vectors that are normal to each
     of the edges respectively, and which passes through the center of the edges respectively
     center[{x_, y_, z_}] := Module[
     {xy = 0.5*(x + y), yz = 0.5*(y + z), zx = 0.5*(z + x), nxy, nyz, 
     beta, cen},
     nxy = normal[y - x];
     nyz = normal[y - z];
     beta = (y-x).(xy - yz)/(nyz.(y-x));
     cen = yz + beta*nyz;
     Graphics[{Line[{x, y, z, x}], Red, Point[cen], Line[{cen, xy}], 
     Line[{cen, yz}], Green, Line[{cen, zx}]}]
     
     ]
 */
  real xy[2], yz[2], nxy[2], nyz[2], ymx[2], ymz[2], beta, bot;
  int i;

  for (i = 0; i < 2; i++) ymx[i] = y[i] - x[i];
  for (i = 0; i < 2; i++) ymz[i] = y[i] - z[i];
  for (i = 0; i < 2; i++) xy[i] = 0.5*(x[i] + y[i]);
  for (i = 0; i < 2; i++) yz[i] = 0.5*(y[i] + z[i]);


  normal(ymx, nxy);
  normal(ymz, nyz);
  bot = nyz[0]*(x[0]-y[0])+nyz[1]*(x[1]-y[1]);
  if (bot == 0){/* xy and yz are parallel */
    c[0] = xy[0]; c[1] = xy[1];
    return;
  }
  beta = ((x[0] - y[0])*(xy[0] - yz[0])+(x[1] - y[1])*(xy[1] - yz[1]))/bot;
  c[0] = yz[0] + beta*nyz[0];
  c[1] = yz[1] + beta*nyz[1];
  return;

}

static SparseMatrix matrix_add_entry(SparseMatrix A, int i, int j, int val){
  int i1 = i, j1 = j;
  if (i < j) {
    i1 = j; j1 = i;
  }
  A = SparseMatrix_coordinate_form_add_entries(A, 1, &j1, &i1, &val);
  return SparseMatrix_coordinate_form_add_entries(A, 1, &i1, &j1, &val);
}



void plot_polys(int use_line, SparseMatrix polys, real *x_poly, int *polys_groups, float *r, float *g, float *b){
  int i, j, *ia = polys->ia, *ja = polys->ja, *a = (int*) polys->a, npolys = polys->m, nverts = polys->n, ipoly = -1, max_grp,
    min_grp;


  min_grp = max_grp = polys_groups[0];
  for (i = 0; i < npolys; i++) {
    max_grp = MAX(polys_groups[i], max_grp);
    min_grp = MIN(polys_groups[i], min_grp);
  }
  if (max_grp == min_grp) max_grp++;
  if (Verbose) fprintf(stderr,"npolys = %d\n",npolys);
  printf("Graphics[{");
  for (i = 0; i < npolys; i++){
    for (j = ia[i]; j < ia[i+1]; j++){
      assert(ja[j] < nverts && ja[j] >= 0);
      if (a[j] != ipoly){/* the first poly, or a hole */
	ipoly = a[j];
	if (ipoly != a[0]) printf("}],");
	if (use_line){
	  printf("Black,");
	  printf("Line[{");
	} else {
	  /*printf("Hue[%f],",0.6*(polys_groups[i]/(real) (max_grp-min_grp)));*/
	  if (r && g && b){
	    printf("RGBColor[%f,%f,%f],",r[polys_groups[i]], g[polys_groups[i]], b[polys_groups[i]]);
	  } else {
	    printf("Hue[%f],",((polys_groups[i]-min_grp)/(real) (max_grp-min_grp)));
	  }
	  printf("Polygon[{");
	}

      } else {
	if (j > ia[i]) printf(",");
      }
      printf("{%f,%f}",x_poly[2*ja[j]],x_poly[2*ja[j]+1]);
    }
  }
  printf("}]}]");
}

#if 0
void ps_one_poly(psfile_t *f, int use_line, int border, int fill, int close, int is_river, int np, float *xp, float *yp){
  if (use_line){
    if (is_river){
      /*river*/
    } else {
    }
    ps_polygon(f, np, xp, yp, border, fill, close);
  } else {
    ps_polygon(f, np, xp, yp, border, fill, close);
  }
}

void plot_ps_polygons(psfile_t *f, int use_line, SparseMatrix polys, real *x_poly, int *polys_groups){
  int i, j, *ia = polys->ia, *ja = polys->ja, *a = (int*) polys->a, npolys = polys->m, nverts = polys->n, ipoly,first;
  int np = 0, maxlen = 0;
  float *xp, *yp;
  int border = 0, fill = -1, close = 1;
  int is_river;

  for (i = 0; i < npolys; i++) maxlen = MAX(maxlen, ia[i+1]-ia[i]);

  xp = MALLOC(sizeof(float)*maxlen);
  yp = MALLOC(sizeof(float)*maxlen);

  if (Verbose) fprintf(stderr,"npolys = %d\n",npolys);
  first = ABS(a[0]); ipoly = first + 1;
  for (i = 0; i < npolys; i++){
    np = 0;
    for (j = ia[i]; j < ia[i+1]; j++){
      assert(ja[j] < nverts && ja[j] >= 0);
      if (ABS(a[j]) != ipoly){/* the first poly, or a hole */
	ipoly = ABS(a[j]);
	is_river = (a[j] < 0);
	ps_one_poly(f, use_line, border, fill, close, is_river, np, xp, yp);
	np = 0;/* start a new polygon */

      } 
      xp[np] = x_poly[2*ja[j]]; yp[np++] = x_poly[2*ja[j]+1];
    }
    if (use_line) {
      ps_one_poly(f, use_line, border, fill, close, is_river, np, xp, yp);
    } else {
      ps_one_poly(f, use_line, polys_groups[i], polys_groups[i], close, is_river, np, xp, yp);
    }
  }
  FREE(xp);
  FREE(yp);

}

static void plot_line(real *x, real *y){
  printf("Line[{{%f,%f},{%f,%f}}]", x[0], x[1], y[0], y[1]);
}
static void plot_voronoi(int n, SparseMatrix A, struct Triangle *T){ 
  int *val, i, j, nline = 0;
  val = (int*) A->a;
  
  printf("Graphics[{");
  for (i = 0; i < n; i++){
    for (j = A->ia[i]; j < A->ia[i+1]; j++){
      if (j < A->ia[n] - A->ia[0] - 1 && A->ja[j] == A->ja[j+1]){
	if (nline > 0) printf(",");
	nline++;
	plot_line((T)[val[j]].center, (T)[val[j+1]].center);
	j++;
      }
    }
  }
  printf("}]");
}


void plot_ps_labels(psfile_t *f, int n, int dim, real *x, char **labels, real *width, float *fsz){

  int i;

  for (i = 0; i < n; i++){
    if (fsz){
      ps_font(f, NULL, 3*fsz[i]); /* name can be NULL if the current font name is to be used */
    } else {
      ps_font(f, NULL, -7); /* name can be NULL if the current font name is to be used */
    }
    ps_textc(f, x[i*dim], x[i*dim+1], labels[i], 0);
  }

}

static void plot_ps_edges(psfile_t *f, SparseMatrix A, int dim, real *x){
  int i, *ia, *ja, n, j;
  float xx[2], yy[2];
  n = A->m;
  ia = A->ia;
  ja = A->ja;
  for (i = 0; i < n; i++){
    for (j = ia[i]; j < ia[i+1]; j++){
      if (ja[j] == i) continue;
      xx[0] = x[i*dim];
      xx[1] = x[ja[j]*dim];
      yy[0] = x[i*dim+1];
      yy[1] = x[ja[j]*dim+1];
      ps_polygon(f, 2, xx, yy, TRUE, -1, FALSE);
    }
  }
}
static void plot_triangles(int n, SparseMatrix A, int dim, real *x){
  int i, j, ii, jj;
  printf("Graphics[{Green");
  for (i = 0; i < n; i++) {
    for (j = A->ia[i]; j < A->ia[i+1]; j++){
      ii = i;
      jj = A->ja[j];
      if (j < A->ia[n] - A->ia[0] - 1 && A->ja[j] == A->ja[j+1]) j++;
      printf(",Line[{{%f,%f},{%f,%f}}]", x[ii*dim], x[ii*dim+1], x[jj*dim], x[jj*dim+1]);
    }
  }
  printf("}]");
}

static void poly_bbox(SparseMatrix polys, real *x_poly, real *bbox){
  /* get the bounding box of a polygon. 
     bbox[0]: xmin
     bbox[1]: xmax
     bbox[2]: ymin 
     bbox[3]: ymax
  */
  int i, j, *ia = polys->ia, *ja = polys->ja;
  int npolys = polys->m;

  bbox[0] = bbox[1] = x_poly[2*ja[ia[0]]];
  bbox[2] = bbox[3] = x_poly[2*ja[ia[0]] + 1];
  
  for (i = 0; i < npolys; i++){
    for (j = ia[i]; j < ia[i+1]; j++){
      bbox[0] = MIN(bbox[0], x_poly[2*ja[j]]); 
      bbox[1] = MAX(bbox[1], x_poly[2*ja[j]]); 
      bbox[2] = MIN(bbox[2], x_poly[2*ja[j]+1]);
      bbox[3] = MAX(bbox[3], x_poly[2*ja[j]+1]);
    }
  }
}

void plot_ps_map(int n, int dim, real *x, SparseMatrix polys, SparseMatrix poly_lines, real line_width, real *x_poly, int *polys_groups, char **labels, real *width,
		 float *fsz, float *r, float *g, float *b, char *plot_label, real *bg_color, SparseMatrix A){
 
  psfile_t *f;
  int i;
  real xmin[2], xmax[2], w = 7, h = 10;
  real asp;
  real bbox[4];
  float pad = 20, label_fsz = 10;
  int MAX_GRP;
  int IBG;/*back ground color number */
  int plot_polyQ = TRUE;

  if (!r || !g || !b) plot_polyQ = FALSE;

  xmin[0] = xmax[0] = x[0];
  xmin[1] = xmax[1] = x[1];

  for (i = 0; i < n; i++){
    xmin[0] = MIN(xmin[0], x[i*dim] - width[i*dim]);
    xmax[0] = MAX(xmax[0], x[i*dim] + width[i*dim]);
    xmin[1] = MIN(xmin[1], x[i*dim + 1] - width[i*dim+1]);
    xmax[1] = MAX(xmax[1], x[i*dim + 1] + width[i*dim+1]);
  }

  if (polys){
    poly_bbox(polys, x_poly, bbox);
    xmin[0] = MIN(xmin[0], bbox[0]);
    xmax[0] = MAX(xmax[0], bbox[1]);
    xmin[1] = MIN(xmin[1], bbox[2]);
    xmax[1] = MAX(xmax[1], bbox[3]);
  }

  if (poly_lines){
    poly_bbox(poly_lines, x_poly, bbox);
    xmin[0] = MIN(xmin[0], bbox[0]);
    xmax[0] = MAX(xmax[0], bbox[1]);
    xmin[1] = MIN(xmin[1], bbox[2]);
    xmax[1] = MAX(xmax[1], bbox[3]);
  }

  pad = .08*MIN(xmax[0] - xmin[0], xmax[1] - xmin[1]);
  label_fsz = pad/3;

  xmin[0] -= pad;
  xmax[0] += pad;
  xmin[1] -= pad;
  xmax[1] += pad;


 if ((xmax[1] - xmin[1]) < 0.001*(xmax[0] - xmin[0])){
    asp = 0.001;
  } else {
    asp = (xmax[1] - xmin[1])/(xmax[0] - xmin[0]);
  }
  if (w*asp > h){
    w = h/asp;
  } else {
    h = w*asp;
  }
  fprintf(stderr, "asp = %f, w = %f, h = %f\n",asp,w,h);

  /*  f = ps_create_autoscale("test.ps", "title", 7, 7, 0);*/
  f = ps_create_autoscale(NULL, "title", w, h, 0);
  /*f = ps_create_epsf(NULL, "title", xmin[0], xmin[1], xmax[0] - xmin[0], xmax[1] - xmin[1]);*/

  ps_line_width(f, 0.00001); // line width 1 point

  ps_font(f, NULL, 10); /* name can be NULL if the current font name is to be used */

  MAX_GRP = polys_groups[0];

  for (i = 0; i < polys->m; i++){
    if (r && g && b) ps_define_color_rgb(f, polys_groups[i], r[polys_groups[i]], g[polys_groups[i]], b[polys_groups[i]]);
    MAX_GRP = MAX(MAX_GRP, polys_groups[i]);
    assert(polys_groups[i] < PS_MAX_COLORS && polys_groups[i] >= 0);
  }

  /* set bg color */
  IBG = MAX_GRP + 1;
  assert(MAX_GRP + 1 < PS_MAX_COLORS);
  if (bg_color) ps_define_color_rgb(f, IBG, bg_color[0], bg_color[1], bg_color[2]);

  ps_color_model(f, PS_COLOR_MODEL_PALETTE);
  if (bg_color) ps_rect(f, (float) xmin[0], (float) xmin[1], xmax[0] - xmin[0], xmax[1] - xmin[1], -1, IBG);


  /* labels */
  ps_color_model(f, PS_COLOR_MODEL_RGB);
  ps_font(f, NULL, label_fsz); /* name can be NULL if the current font name is to be used */
  ps_textc(f, (float)(.5*(xmin[0] + xmax[0])), (float) xmin[1] + pad, plot_label, 0);
  
  ps_color_model(f, PS_COLOR_MODEL_PALETTE);

  /*polygons */
  if (plot_polyQ) plot_ps_polygons(f, FALSE, polys, x_poly, polys_groups);
  
  ps_color_model(f, PS_COLOR_MODEL_RGB);
  /* polylines*/
  if (line_width >= 0){
    ps_line_width(f, (float) line_width);
    plot_ps_polygons(f, TRUE, poly_lines, x_poly, polys_groups);
  }

  /* edges*/
  if (A) plot_ps_edges(f, A, dim, x);
  
  if (labels) plot_ps_labels(f, n, dim, x, labels, width, fsz);
  
  ps_done(f);
  
  
}
 
#endif


static void plot_dot_edges(FILE *f, SparseMatrix A, int dim, real *x){
  int i, *ia, *ja, j;

  
  int n = A->m;
  ia = A->ia;
  ja = A->ja;
  for (i = 0; i < n; i++){
    for (j = ia[i]; j < ia[i+1]; j++){
      if (ja[j] == i) continue;
      fprintf(f,"%d -- %d;\n",i,ja[j]);
    }
  }
}
#if 0
static void plot_processing_edges(FILE *f, SparseMatrix A, int dim, real *x){
  int i, *ia, *ja, j;

  //fprintf(f,"stroke(.5);\n");
  fprintf(f, "beginLines\n");
  fprintf(f, "color(125,125,125)\n");
  int n = A->m;
  ia = A->ia;
  ja = A->ja;
  for (i = 0; i < n; i++){
    for (j = ia[i]; j < ia[i+1]; j++){
      if (ja[j] == i) continue;
      //fprintf(f,"line(%f, %f, %f, %f);\n", x[i*dim], x[i*dim+1], x[ja[j]*dim], x[ja[j]*dim+1]);
      fprintf(f,"%f %f %f %f\n", x[i*dim], x[i*dim+1], x[ja[j]*dim], x[ja[j]*dim+1]);
    }
  }
  fprintf(f,"endLines\n");
}
#endif

void plot_dot_labels(FILE *f, int n, int dim, real *x, char **labels, real *width, float *fsz){
  int i;

  for (i = 0; i < n; i++){
    if (fsz){
      fprintf(f, "%d [label=\"%s\", pos=\"%lf,%lf\", fontsize=%f];\n",i, labels[i], x[i*dim], x[i*dim+1], fsz[i]); 
    } else {
      fprintf(f, "%d [label=\"%s\", pos=\"%lf,%lf\"];\n",i, labels[i], x[i*dim], x[i*dim+1]); 
    }
  }

}

#if 0
void plot_processing_labels(FILE *f, int n, int dim, real *x, char **labels, real *width, float *fsz){
  int i;
  //  fprintf(f, "PFont myFont; myFont = createFont(\"FFScala\",8); fill(0,0,0);\n");
  for (i = 0; i < n; i++){
    if (fsz){
      //   fprintf(f, "textFont(myFont, %f); text(\"%s\", %f, %f);\n", fsz[i], labels[i], x[i*dim], x[i*dim+1]);
      fprintf(f, "beginText\nfontSize(%f)\ntext(\"%s\")\nposition(%f, %f)\nendText\n", fsz[i], labels[i], x[i*dim], x[i*dim+1]);
    } else {
      fprintf(f, "beginText\ntext(\"%s\")\nposition(%f, %f)\nendText\n", labels[i], x[i*dim], x[i*dim+1]);
    }
  }

}
#endif

static void dot_polygon(char **sbuff, int *len, int *len_max, int np, float *xp, float *yp, real line_width,  
			int fill, int close, char *cstring){
  int i;
  int ret = 0;
  char buf[10000];
  char swidth[10000];
  int len_swidth;

  if (np > 0){
    /* figure out the size needed */
    if (fill >= 0){/* poly*/
      ret += sprintf(buf, " c %d -%s C %d -%s P %d ",(int) strlen(cstring), cstring, (int) strlen(cstring), cstring, np);
    } else {/* line*/
      assert(line_width >= 0);
      if (line_width > 0){
	sprintf(swidth,"%f",line_width);
	len_swidth = strlen(swidth);
	sprintf(swidth,"S %d -setlinewidth(%f)",len_swidth+14, line_width);
	ret += sprintf(buf, " c %d -%s %s L %d ",(int) strlen(cstring), cstring, swidth, np);
      } else {
	ret += sprintf(buf, " c %d -%s L %d ",(int) strlen(cstring), cstring, np);
      }
    }
    for (i = 0; i < np; i++){
      ret += sprintf(buf, " %f %f",xp[i], yp[i]);
    }

    if (*len + ret > *len_max - 1){
      *len_max = *len_max + MAX(100, 0.2*(*len_max)) + ret;
      *sbuff = REALLOC(*sbuff, *len_max);
    }

    /* do the actual string */
    if (fill >= 0){
      ret = sprintf(&((*sbuff)[*len]), " c %d -%s C %d -%s P %d ",(int) strlen(cstring), cstring, (int) strlen(cstring), cstring, np);
    } else {
      if (line_width > 0){
        sprintf(swidth,"%f",line_width);
        len_swidth = strlen(swidth);
        sprintf(swidth,"S %d -setlinewidth(%f)",len_swidth+14, line_width);
	ret = sprintf(&((*sbuff)[*len]), " c %d -%s %s L %d ",(int) strlen(cstring), cstring, swidth, np);
      } else {
	ret = sprintf(&((*sbuff)[*len]), " c %d -%s L %d ",(int) strlen(cstring), cstring, np);
      }
    }
    (*len) += ret;
    for (i = 0; i < np; i++){
      assert(*len < *len_max);
      ret = sprintf(&((*sbuff)[*len]), " %f %f",xp[i], yp[i]);
      (*len) += ret;
    }


  }
}
static void processing_polygon(FILE *f, int np, float *xp, float *yp, real line_width, int fill, int close, 
			       float rr, float gg, float bb){
  int i;

  if (np > 0){
    if (fill >= 0){
      // fprintf(f, "beginShape();\n noStroke(); fill(%f, %f, %f);\n", 255*rr, 255*gg, 255*bb);
      fprintf(f, "beginPolygons\ncolor(%f, %f, %f)\n", 255*rr, 255*gg, 255*bb);
    } else {
      //fprintf(f, "beginShape();\n");
      fprintf(f, "beginPolylines\n");
      if (line_width > 0){
	fprintf(f, "strokeWeight(%f);\n", line_width);
      }
    }
    for (i = 0; i < np; i++){
      //fprintf(f, "vertex(%f, %f);\n",xp[i], yp[i]);
      fprintf(f, "%f %f\n",xp[i], yp[i]);
    }
    //fprintf(f, "endShape(CLOSE);\n");
    if (fill >= 0){
      fprintf(f, "endPolygons\n");
    } else {
      fprintf(f, "endPolylines\n");
    }

  }
}

void dot_one_poly(char **sbuff, int *len, int *len_max, int use_line, real line_width, int fill, int close, int is_river, int np, float *xp, float *yp, char *cstring){
  if (use_line){
    if (is_river){
      /*river*/
    } else {
    }
    dot_polygon(sbuff, len, len_max, np, xp, yp, line_width, fill, close, cstring);
  } else {
    dot_polygon(sbuff, len, len_max, np, xp, yp, line_width, fill, close, cstring);
  }
}

void processing_one_poly(FILE *f, int use_line, real line_width, int fill, int close, int is_river, int np, float *xp, float *yp, 
		      float rr, float gg, float bb){
  if (use_line){
    if (is_river){
      /*river*/
    } else {
    }
    processing_polygon(f, np, xp, yp, line_width, fill, close, rr, gg, bb);
  } else {
    processing_polygon(f, np, xp, yp, line_width, fill, close, rr, gg, bb);
  }
}


void plot_dot_polygons(char **sbuff, int *len, int *len_max, real line_width, char *line_color, SparseMatrix polys, real *x_poly, int *polys_groups, float *r, float *g, float *b, char *opacity){
  int i, j, *ia = polys->ia, *ja = polys->ja, *a = (int*) polys->a, npolys = polys->m, nverts = polys->n, ipoly,first;
  int np = 0, maxlen = 0;
  float *xp, *yp;
  int fill = -1, close = 1;
  int is_river = FALSE;
  char cstring[] = "#aaaaaaff";
  int use_line = (line_width >= 0);
  
  for (i = 0; i < npolys; i++) maxlen = MAX(maxlen, ia[i+1]-ia[i]);

  xp = MALLOC(sizeof(float)*maxlen);
  yp = MALLOC(sizeof(float)*maxlen);

  if (Verbose) fprintf(stderr,"npolys = %d\n",npolys);
  first = ABS(a[0]); ipoly = first + 1;
  for (i = 0; i < npolys; i++){
    np = 0;
    for (j = ia[i]; j < ia[i+1]; j++){
      assert(ja[j] < nverts && ja[j] >= 0);
      if (ABS(a[j]) != ipoly){/* the first poly, or a hole */
	ipoly = ABS(a[j]);
	is_river = (a[j] < 0);
	if (r && g && b) {
	  rgb2hex(r[polys_groups[i]], g[polys_groups[i]], b[polys_groups[i]], cstring, opacity);
	}
	dot_one_poly(sbuff, len, len_max, use_line, line_width, fill, close, is_river, np, xp, yp, cstring);
	np = 0;/* start a new polygon */
      } 
      xp[np] = x_poly[2*ja[j]]; yp[np++] = x_poly[2*ja[j]+1];
    }
    if (use_line) {
      dot_one_poly(sbuff, len, len_max, use_line, line_width, fill, close, is_river, np, xp, yp, line_color);
    } else {
      /* why set fill to polys_groups[i]?*/
      //dot_one_poly(sbuff, len, len_max, use_line, polys_groups[i], polys_groups[i], close, is_river, np, xp, yp, cstring);
      dot_one_poly(sbuff, len, len_max, use_line, -1, 1, close, is_river, np, xp, yp, cstring);
    }
  }
  FREE(xp);
  FREE(yp);

}

void plot_processing_polygons(FILE *f, real line_width, SparseMatrix polys, real *x_poly, int *polys_groups, float *r, float *g, float *b){
  int i, j, *ia = polys->ia, *ja = polys->ja, *a = (int*) polys->a, npolys = polys->m, nverts = polys->n, ipoly,first;
  int np = 0, maxlen = 0;
  float *xp, *yp;
  int fill = -1, close = 1;
  int is_river = FALSE;
  int use_line = (line_width >= 0);
  float rr = 0, gg = 0, bb = 0;

  for (i = 0; i < npolys; i++) maxlen = MAX(maxlen, ia[i+1]-ia[i]);

  xp = MALLOC(sizeof(float)*maxlen);
  yp = MALLOC(sizeof(float)*maxlen);

  if (Verbose) fprintf(stderr,"npolys = %d\n",npolys);
  first = ABS(a[0]); ipoly = first + 1;
  for (i = 0; i < npolys; i++){
    np = 0;
    for (j = ia[i]; j < ia[i+1]; j++){
      assert(ja[j] < nverts && ja[j] >= 0);
      if (ABS(a[j]) != ipoly){/* the first poly, or a hole */
	ipoly = ABS(a[j]);
	is_river = (a[j] < 0);
	if (r && g && b) {
	  rr = r[polys_groups[i]]; gg = g[polys_groups[i]]; bb = b[polys_groups[i]];
	}
	  processing_one_poly(f, use_line, line_width, fill, close, is_river, np, xp, yp, rr, gg, bb);
	np = 0;/* start a new polygon */
      } 
      xp[np] = x_poly[2*ja[j]]; yp[np++] = x_poly[2*ja[j]+1];
    }
    if (use_line) {
      processing_one_poly(f, use_line, line_width, fill, close, is_river, np, xp, yp, rr, gg, bb);
    } else {
      /* why set fill to polys_groups[i]?*/
      processing_one_poly(f,  use_line, -1, 1, close, is_river, np, xp, yp, rr, gg, bb);
    }
  }
  FREE(xp);
  FREE(yp);

}


void plot_dot_map(Agraph_t* gr, int n, int dim, real *x, SparseMatrix polys, SparseMatrix poly_lines, real line_width, char *line_color, real *x_poly, int *polys_groups, char **labels, real *width,
		  float *fsz, float *r, float *g, float *b, char* opacity, char *plot_label, real *bg_color, SparseMatrix A, FILE* f){
  /* if graph object exist, we just modify some attributes, otherwise we dump the whole graph */
  int plot_polyQ = TRUE;
  char *sbuff;
  int len = 0, len_max = 1000;

  sbuff = N_NEW(len_max,char);

  if (!r || !g || !b) plot_polyQ = FALSE;

  if (!gr) {
    fprintf(f, "graph map {\n node [margin = 0 width=0.0001 height=0.00001 shape=plaintext];\n graph [outputorder=edgesfirst, bgcolor=\"#dae2ff\"]\n edge [color=\"#55555515\",fontname=\"Helvetica-Bold\"]\n");
  } else {
    agattr(gr, AGNODE, "margin", "0"); 
    agattr(gr, AGNODE, "width", "0.0001"); 
    agattr(gr, AGNODE, "height", "0.0001"); 
    agattr(gr, AGNODE, "shape", "plaintext"); 
    agattr(gr, AGNODE, "margin", "0"); 
    agattr(gr, AGNODE, "fontname", "Helvetica-Bold"); 
    agattr(gr, AGRAPH, "outputorder", "edgesfirst");
    agattr(gr, AGRAPH, "bgcolor", "#dae2ff");
    if (!A) agattr(gr, AGEDGE, "style","invis");/* do not plot edges */
    //    agedgeattr(gr, "color", "#55555515"); 
  }

  /*polygons */
  if (plot_polyQ) {
    if (!gr) fprintf(f,"_draw_ = \"");
    plot_dot_polygons(&sbuff, &len, &len_max, -1., NULL, polys, x_poly, polys_groups, r, g, b, opacity);
  }

  /* polylines: line width is set here */
  if (line_width >= 0){
    plot_dot_polygons(&sbuff, &len, &len_max, line_width, line_color, poly_lines, x_poly, polys_groups, NULL, NULL, NULL, NULL);
  }
  if (!gr) {
    fprintf(f,"%s",sbuff);
    fprintf(f,"\"\n");/* close polygons/lines */
  } else {
    agattr(gr, AGRAPH, "_draw_", sbuff);
    agwrite(gr, f);
  }

  /* nodes */
  if (!gr && labels) plot_dot_labels(f, n, dim, x, labels, width, fsz);
  /* edges */
  if (!gr && A) plot_dot_edges(f, A, dim, x);

  /* background color + plot label?*/

  if (!gr) fprintf(f, "}\n");
  
 
}

#if 0
static void get_polygon_bbox(SparseMatrix polys, real *x_poly, real *bbox_x, real *bbox_y){
  /* find bounding box. bbox_x is the xmin and xmax, bbox_y is ymin and ymax */
  int i, j, *ia = polys->ia, *ja = polys->ja, *a = (int*) polys->a, npolys = polys->m, nverts = polys->n, ipoly,first;
  int NEW = TRUE;

  first = ABS(a[0]); ipoly = first + 1;
  for (i = 0; i < npolys; i++){
    for (j = ia[i]; j < ia[i+1]; j++){
      assert(ja[j] < nverts && ja[j] >= 0);
      if (NEW){
	bbox_x[0] = bbox_x[1] =  x_poly[2*ja[j]];
	bbox_y[0] = bbox_y[1] =  x_poly[2*ja[j]+1];
	NEW = FALSE;
      } else {
	bbox_x[0] = MIN(bbox_x[0], x_poly[2*ja[j]]);
	bbox_x[1] = MAX(bbox_x[1], x_poly[2*ja[j]]);
	bbox_y[0] = MIN(bbox_y[0], x_poly[2*ja[j]+1]);
	bbox_y[1] = MAX(bbox_y[1], x_poly[2*ja[j]+1]);
      }
    }
  }
}
#endif

#if 0
void plot_processing_map(Agraph_t* gr, int n, int dim, real *x, SparseMatrix polys, SparseMatrix poly_lines, real line_width, int nverts, real *x_poly, int *polys_groups, char **labels, real *width,
		 float *fsz, float *r, float *g, float *b, char *plot_label, real *bg_color, SparseMatrix A){
  /* if graph object exist, we just modify some attributes, otherwise we dump the whole graph */
  int plot_polyQ = TRUE;
  FILE *f = stdout;
  real xmin[2], xwidth[2], scaling=1, bbox_x[2], bbox_y[2];
  int i;

  /* shift to make all coordinates position */
  get_polygon_bbox(polys, x_poly, bbox_x, bbox_y);
  xmin[0] = bbox_x[0]; xmin[1] = bbox_y[0]; 
  xwidth[0] = (bbox_x[1] - bbox_x[0])*scaling;
  xwidth[1] = (bbox_y[1] - bbox_y[0])*scaling;
  for (i = 0; i < n; i++) {
    xmin[0] = MIN(xmin[0], x[i*dim]);
    xmin[1] = MIN(xmin[1], x[i*dim+1]);
  }

  fprintf(stderr,"xmin={%f, %f}\n", xmin[0], xmin[1]);

  for (i = 0; i < n; i++) {
    x[i*dim] = (x[i*dim] - xmin[0])*scaling;
    x[i*dim+1] = (x[i*dim+1] - xmin[1])*scaling;
    xwidth[0] = MAX(xwidth[0], x[i*dim]);
    xwidth[1] = MAX(xwidth[1], x[i*dim+1]);
  }
  for (i = 0; i < nverts; i++) {
    x_poly[i*dim] = (x_poly[i*dim] - xmin[0])*scaling;
    x_poly[i*dim+1] = (x_poly[i*dim+1] - xmin[1])*scaling;
  }
  /*
  fprintf(f,"//This file is for processing language. Syntax:\n//\n");
  fprintf(f,"//beginPolygons\n//[color(r,g,b)]\n//x y\n...\n//endPolygons\n//\n");
  fprintf(f,"//beginPolylines\n//[color(r,g,b)]\n//...\n//endPolylines\n\n");
  fprintf(f,"//beginLines\n//[color(r,g,b)]\n//x1 y1 x2 y2\n//...\n endLines\n//\n");
  fprintf(f,"//beginText\n//[fontSize(x)]\n//text(\"foo\")\n//position(x,y)\n\\endText\n//\n");
  fprintf(f,"//\n");

  fprintf(f, "size(%d, %d);\n", (int)(xwidth[0]+1), (int) (xwidth[1]+1));
  */

  if (!r || !g || !b) plot_polyQ = FALSE;

  if (!gr) {
    fprintf(f, "background(234, 242, 255)");
  }

  /*polygons */
  if (plot_polyQ) {
    plot_processing_polygons(f, -1., polys, x_poly, polys_groups, r, g, b);
  }

  /* polylines: line width is set here */
  if (line_width >= 0){
    plot_processing_polygons(f, line_width, poly_lines, x_poly, polys_groups, NULL, NULL, NULL);
  }

  /* edges */
  if (A) plot_processing_edges(f, A, dim, x);

  /* nodes */
  if (labels) plot_processing_labels(f, n, dim, x, labels, width, fsz);

 
}
#endif

static void get_tri(int n, int dim, real *x, int *nt, struct Triangle **T, SparseMatrix *E, int *flag) {
   /* always contains a self edge and is symmetric.
     input:
     n: number of points
     dim: dimension, only first2D used
     x: point j is stored x[j*dim]--x[j*dim+dim-1]
     output: 
     nt: number of triangles
     T: triangles
     E: a matrix of size nxn, if two points i > j are connected by an taiangulation edge, and is neighboring two triangles 
     .  t1 and t2, then A[i,j] is both t1 and t2
   */
  int i, j, i0, i1, i2, ntri;
  SparseMatrix A, B;

  int* trilist = get_triangles(x, n, &ntri);
  *flag = 0;


  *T = N_NEW(ntri,struct Triangle);

  A = SparseMatrix_new(n, n, 1, MATRIX_TYPE_INTEGER, FORMAT_COORD);
  for (i = 0; i < ntri; i++) {
    for (j = 0; j < 3; j++) {
      ((*T)[i]).vertices[j] = trilist[i * 3 + j];
    }
    i0 = (*T)[i].vertices[0]; i1 = (*T)[i].vertices[1]; i2 = (*T)[i].vertices[2];

    triangle_center(&x[i0*dim], &x[i1*dim], &x[i2*dim], (*T)[i].center);
    A = matrix_add_entry(A, i0, i1, i);
    A = matrix_add_entry(A, i1, i2, i);
    A = matrix_add_entry(A, i2, i0, i);
  }

  B = SparseMatrix_from_coordinate_format_not_compacted(A, SUM_REPEATED_NONE);
  SparseMatrix_delete(A);
  B = SparseMatrix_sort(B);
  *E = B;

  *nt = ntri;

  FREE(trilist);

  return;
}



void plot_labels(int n, int dim, real *x, char **labels){
  int i, j;

  printf("Graphics[{");

  for (i = 0; i < n; i++){
    printf("Text[\"%s\",{",labels[i]);
    for (j = 0; j < 2; j++) {
      printf("%f",x[i*dim+j]);
      if (j == 0) printf(",");
    }
    printf("}]");
    if (i < n - 1) printf(",\n");
  }
  printf("}]");


}
void plot_points(int n, int dim, real *x){
  int i, j;

  printf("Graphics[{Point[{");
  for (i = 0; i < n; i++){
    printf("{");
    for (j = 0; j < 2; j++) {
      printf("%f",x[i*dim+j]);
      if (j == 0) printf(",");
    }
    printf("}");
    if (i < n - 1) printf(",");
  }
  printf("}]");

  /*
  printf(",");
  for (i = 0; i < n; i++){
    printf("Text[%d,{",i);
    for (j = 0; j < 2; j++) {
      printf("%f",x[i*dim+j]);
      if (j == 0) printf(",");
    }
    printf("}]");
    if (i < n - 1) printf(",");
  }
  */
  printf("}]");

}

void plot_edges(int n, int dim, real *x, SparseMatrix A){
  int i, j, k;
  int *ia, *ja;

  if (!A) {
    printf("Graphics[{}]");
    return;
  }
  ia = A->ia; ja = A->ja;

  printf("Graphics[(* edges of the graph*){");
  for (i = 0; i < n; i++){
    for (j = ia[i]; j < ia[i+1]; j++){
      if (i > 0 && j == ia[i]) printf(",");;
      printf("Line[{{");
      for (k = 0; k < 2; k++) {
	printf("%f",x[i*dim+k]);
	if (k == 0) printf(",");
      }
      printf("},{");
      for (k = 0; k < 2; k++) {
	printf("%f",x[ja[j]*dim+k]);
	if (k == 0) printf(",");
      }
      printf("}}]");
      if (j < ia[i+1] - 1) printf(",");
    }
  }
  printf("}(* end of edges of the graph*)]");

}
static SparseMatrix get_country_graph(int n, SparseMatrix A, int *groups, SparseMatrix *poly_point_map, int GRP_RANDOM, int GRP_BBOX){
  /* form a graph each vertex is a group (a country), and a vertex is connected to another if the two countryes shares borders.
   since the group ID may not be contigous (e.g., only groups 2,3,5, -1), we will return NULL if one of the group has non-positive ID! */
  int *ia, *ja;
  int one = 1, jj, i, j, ig1, ig2;
  SparseMatrix B, BB;
  int min_grp, max_grp;
  
  min_grp = max_grp = groups[0];
  for (i = 0; i < n; i++) {
    max_grp = MAX(groups[i], max_grp);
    min_grp = MIN(groups[i], min_grp);
  }
  if (min_grp <= 0) return NULL;
  B = SparseMatrix_new(max_grp, max_grp, 1, MATRIX_TYPE_INTEGER, FORMAT_COORD);
  ia = A->ia;
  ja = A->ja;
  for (i = 0; i < n; i++){
    ig1 = groups[i]-1;/* add a diagonal entry */
    SparseMatrix_coordinate_form_add_entries(B, 1, &ig1, &ig1, &one);
    for (j = ia[i]; j < ia[i+1]; j++){
      jj = ja[j];
      if (i != jj && groups[i] != groups[jj] && groups[jj] != GRP_RANDOM && groups[jj] != GRP_BBOX){
	ig1 = groups[i]-1; ig2 = groups[jj]-1;
	SparseMatrix_coordinate_form_add_entries(B, 1, &ig1, &ig2, &one);
      }
    }
  }
  BB = SparseMatrix_from_coordinate_format(B);
  SparseMatrix_delete(B);
  return BB;
}

static void conn_comp(int n, SparseMatrix A, int *groups, SparseMatrix *poly_point_map){
  /* form a graph where only vertices that are connected as well as in the same group are connected */
  int *ia, *ja;
  int one = 1, jj, i, j;
  SparseMatrix B, BB;
  int ncomps, *comps = NULL, *comps_ptr = NULL;

  B = SparseMatrix_new(n, n, 1, MATRIX_TYPE_INTEGER, FORMAT_COORD);
  ia = A->ia;
  ja = A->ja;
  for (i = 0; i < n; i++){
    for (j = ia[i]; j < ia[i+1]; j++){
      jj = ja[j];
      if (i != jj && groups[i] == groups[jj]){
	SparseMatrix_coordinate_form_add_entries(B, 1, &i, &jj, &one);
      }
    }
  }
  BB = SparseMatrix_from_coordinate_format(B);

  SparseMatrix_weakly_connected_components(BB, &ncomps, &comps, &comps_ptr);
  SparseMatrix_delete(B);
  SparseMatrix_delete(BB);
  *poly_point_map = SparseMatrix_new(ncomps, n, n, MATRIX_TYPE_PATTERN, FORMAT_CSR);
  FREE((*poly_point_map)->ia);
  FREE((*poly_point_map)->ja);
  (*poly_point_map)->ia = comps_ptr;
  (*poly_point_map)->ja = comps;
  (*poly_point_map)->nz = n;

}


static void get_poly_lines(int exclude_random, int nt, SparseMatrix graph, SparseMatrix E, int ncomps, int *comps_ptr, int *comps, 
			   int *groups, int *mask, SparseMatrix *poly_lines, int **polys_groups,
			   int GRP_RANDOM, int GRP_BBOX){
  /*============================================================

    polygon outlines 

    ============================================================*/
  int i, *tlist, nz, ipoly, ipoly2, nnt, ii, jj, t1, t2, t, cur, next, nn, j, nlink, sta;
  int *elist, edim = 3;/* a list tell which vertex a particular vertex is linked with during poly construction.
		since the surface is a cycle, each can only link with 2 others, the 3rd position is used to record how many links
	      */
  int *ie = E->ia, *je = E->ja, *e = (int*) E->a, n = E->m;
  int *ia = NULL, *ja = NULL;
  SparseMatrix A;
  int *gmask = NULL;


  graph = NULL;/* we disable checking whether a polyline cross an edge for now due to issues with labels */
  if (graph) {
    assert(graph->m == n);
    gmask = malloc(sizeof(int)*n);
    for (i = 0; i < n; i++) gmask[i] = -1;
    ia = graph->ia; ja = graph->ja;
    edim = 5;/* we also store info about whether an edge of a polygon correspondes to a real edge or not. */
  }

  for (i = 0; i < nt; i++) mask[i] = -1;
  /* loop over every point in each connected component */
  elist = MALLOC(sizeof(int)*(nt)*edim);
  tlist = MALLOC(sizeof(int)*nt*2);
  *poly_lines = SparseMatrix_new(ncomps, nt, 1, MATRIX_TYPE_INTEGER, FORMAT_COORD);
  *polys_groups = MALLOC(sizeof(int)*(ncomps));

  for (i = 0; i < nt; i++) elist[i*edim + 2] = 0;
  nz = ie[E->m] - ie[0];

  ipoly = 1;

  for (i = 0; i < ncomps; i++){
    /*fprintf(stderr, "comp %d has %d members\n",i, comps_ptr[i+1]-comps_ptr[i]);*/
    nnt = 0;
    for (j = comps_ptr[i]; j < comps_ptr[i+1]; j++){
      ii = comps[j];

      if (graph){
	for (jj = ia[ii]; jj < ia[ii+1]; jj++) gmask[ja[jj]] = ii;
      }

      (*polys_groups)[i] = groups[ii];/* assign the grouping of each poly */

      /* skip the country formed by random points */
      if (exclude_random && (groups[ii] == GRP_RANDOM || groups[ii] == GRP_BBOX)) continue;

      /* always skip bounding box */
      if (groups[ii] == GRP_BBOX) continue;

      /*fprintf(stderr, "member %d is in group %d, it is\n",ii, groups[ii]);*/
      for (jj = ie[ii]; jj < ie[ii+1]; jj++){
	/*
	fprintf(stderr, "connected with %d in group %d\n",je[jj], groups[je[jj]]);
	fprintf(stderr, "jj=%d nz = %d je[jj]=%d je[jj+1]=%d\n",jj,nz, je[jj],je[jj+1]);
	*/
	if (groups[je[jj]] != groups[ii] && jj < nz - 1  && je[jj] == je[jj+1]){/* an triangle edge neighboring 2 triangles and two ends not in the same groups */
	  t1 = e[jj];
	  t2 = e[jj+1];

	  nlink = elist[t1*edim + 2]%2;
	  elist[t1*edim + nlink] = t2;/* t1->t2*/
	  if (graph){
	    if (gmask[je[jj]] == ii){/* a real edge */
	      elist[t1*edim+nlink+3] = POLY_LINE_REAL_EDGE;
	    } else {
	      fprintf(stderr,"not real!!!\n");
	      elist[t1*edim+nlink+3] = POLY_LINE_NOT_REAL_EDGE;
	    }
	  }
	  elist[t1*edim + 2]++;

	  nlink = elist[t2*edim + 2]%2;
	  elist[t2*edim + nlink] = t1;/* t1->t2*/
	  elist[t2*edim + 2]++;
	  if (graph){
	    if (gmask[je[jj]] == ii){/* a real edge */
	      elist[t2*edim+nlink+3] = POLY_LINE_REAL_EDGE;
	    } else {
	      fprintf(stderr,"not real!!!\n");
	      elist[t2*edim+nlink+3] = POLY_LINE_NOT_REAL_EDGE;
	    }
	  }

	  tlist[nnt++] = t1; tlist[nnt++] = t2;
	  jj++;
	}
      }
    }/* done poly edges for this component i */


    /* debugging*/
#ifdef DEBUG
    /*
    for (j = 0; j < nnt; j++) {
      if (elist[tlist[j]*edim + 2]%2 != 0){
	printf("wrong: elist[%d*edim+2] = %d\n",j,elist[tlist[j]*edim + 2]%);
	assert(0);
      }
    }
    */
#endif

    /* form one or more (if there is a hole) polygon outlines for this component  */
    for (j = 0; j < nnt; j++){
      t = tlist[j];
      if (mask[t] != i){
	cur = sta = t; mask[cur] = i;
	next = neighbor(t, 1, edim, elist);
	nlink = 1;
	if (graph && neighbor(cur, 3, edim, elist) == POLY_LINE_NOT_REAL_EDGE){
	  ipoly2 = - ipoly;
	} else {
	  ipoly2 = ipoly;
	}
	SparseMatrix_coordinate_form_add_entries(*poly_lines, 1, &i, &cur, &ipoly2);
	while (next != sta){
	  mask[next] = i;
	  
	  if (graph && neighbor(cur, nlink + 3, edim, elist) == POLY_LINE_NOT_REAL_EDGE){
	    ipoly2 = -ipoly;
	  } else {
	    ipoly2 = ipoly;
	  }
	  SparseMatrix_coordinate_form_add_entries(*poly_lines, 1, &i, &next, &ipoly2);

	  nn = neighbor(next, 0, edim, elist);
	  nlink = 0;
	  if (nn == cur) {
	    nlink = 1;
	    nn = neighbor(next, 1, edim, elist);
	  }
	  assert(nn != cur);

	  cur = next;
	  next = nn;
	}

	if (graph && neighbor(cur, nlink + 3, edim, elist) == POLY_LINE_NOT_REAL_EDGE){
	  ipoly2 = -ipoly;
	} else {
	  ipoly2 = ipoly;
	}
	SparseMatrix_coordinate_form_add_entries(*poly_lines, 1, &i, &sta, &ipoly2);/* complete a cycle by adding starting point */

	ipoly++;
      }

    }/* found poly_lines for this comp */
  }

  A = SparseMatrix_from_coordinate_format_not_compacted(*poly_lines, SUM_REPEATED_NONE);
  SparseMatrix_delete(*poly_lines);
  *poly_lines = A;

  FREE(tlist);
  FREE(elist);
}

static void plot_cycle(int head, int *cycle, int *edge_table, real *x){
  int cur, next;
  real x1, x2, y1, y2;

  printf("Graphics[{");
  cur = head;
  while ((next = cycle_next(cur)) != head){
    x1 = x[2*edge_head(cur)];
    y1 = x[2*edge_head(cur)+1];
    x2 = x[2*edge_tail(cur)];
    y2 = x[2*edge_tail(cur)+1];
    printf("{Black,Arrow[{{%f,%f},{%f,%f}}],Green,Text[%d, {%f,%f}],Text[%d, {%f,%f}]},",x1,y1,x2,y2,edge_head(cur),x1,y1,edge_tail(cur),x2,y2);
    cur = next;
  }
  x1 = x[2*edge_head(cur)];
  y1 = x[2*edge_head(cur)+1];
  x2 = x[2*edge_tail(cur)];
  y2 = x[2*edge_tail(cur)+1];
  printf("{Black,Arrow[{{%f,%f},{%f,%f}}],Green,Text[%d, {%f,%f}],Text[%d, {%f,%f}]}}]",x1,y1,x2,y2,edge_head(cur),x1,y1,edge_tail(cur),x2,y2);


}

static void cycle_print(int head, int *cycle, int *edge_table){
  int cur, next;

  cur = head;
  fprintf(stderr, "cycle (edges): {");
  while ((next = cycle_next(cur)) != head){
    fprintf(stderr, "%d,",cur);
    cur = next;
  }
  fprintf(stderr, "%d}\n",cur);

  cur = head;
  fprintf(stderr, "cycle (vertices): ");
  while ((next = cycle_next(cur)) != head){
    fprintf(stderr, "%d--",edge_head(cur));
    cur = next;
  }
  fprintf(stderr, "%d--%d\n",edge_head(cur),edge_tail(cur));






}

static int same_edge(int ecur, int elast, int *edge_table){
  return ((edge_head(ecur) == edge_head(elast) && edge_tail(ecur) == edge_tail(elast))
	  || (edge_head(ecur) == edge_tail(elast) && edge_tail(ecur) == edge_head(elast)));
}

static void get_polygon_solids(int exclude_random, int nt, SparseMatrix E, int ncomps, int *comps_ptr, int *comps, 
			       int *groups, int *mask, real *x_poly, SparseMatrix *polys, int **polys_groups,
			       int GRP_RANDOM, int GRP_BBOX){
  /*============================================================

    polygon slids that will be colored

    ============================================================*/
  int *edge_table;/* a table of edges of the triangle graph. If two vertex u and v are connected and are adjencent to two triangles
		     t1 and t2, then from u there are two edges to v, one denoted as t1->t2, and the other t2->t1. They are
		     numbered as e1 and e2. edge_table[e1]={t1,t2} and edge_table[e2]={t2,t1}
		  */
  SparseMatrix half_edges;/* a graph of triangle edges. If two vertex u and v are connected and are adjencent to two triangles
		     t1 and t2, then from u there are two edges to v, one denoted as t1->t2, and the other t2->t1. They are
		     numbered as e1 and e2. Likewise from v to u there are also two edges e1 and e2.
		  */

  int n = E->m, *ie = E->ia, *je = E->ja, *e = (int*) E->a, ne, i, j, t1, t2, jj, ii;
  int *cycle, cycle_head = 0;/* a list of edges that form a cycle that describe the polygon. cycle[e][0] gives the prev edge in the cycle from e,
	       cycle[e][1] gives the next edge
	     */
  int *edge_cycle_map, NOT_ON_CYCLE = -1;/* map an edge e to its position on cycle, unless it does not exist (NOT_ON_CYCLE) */
  int *emask;/* whether an edge is seens this iter */
  enum {NO_DUPLICATE = -1};
  int *elist, edim = 3;/* a list tell which edge a particular vertex is linked with when a voro cell has been visited,
		since the surface is a cycle, each vertex can only link with 2 edges, the 3rd position is used to record how many links
	      */

  int k, duplicate, ee = 0, ecur, enext, eprev, cur, next, nn, nlink, head, elast = 0, etail, tail, ehead, efirst; 

  int DEBUG_CYCLE = 0;
  SparseMatrix B;

  ne = E->nz;
  edge_table = MALLOC(sizeof(int)*ne*2);  

  for (i = 0; i < n; i++) mask[i] = -1;

  half_edges = SparseMatrix_new(n, n, 1, MATRIX_TYPE_INTEGER, FORMAT_COORD);

  ne = 0;
  for (i = 0; i < n; i++){
    for (j = ie[i]; j < ie[i+1]; j++){
      if (j < ie[n] - ie[0] - 1 && i > je[j] && je[j] == je[j+1]){/* an triangle edge neighboring 2 triangles. Since E is symmetric, we only do one edge of E*/
	t1 = e[j];
	t2 = e[j+1];
	jj = je[j];	  
	assert(jj < n);
	edge_table[ne*2] = t1;/*t1->t2*/
	edge_table[ne*2+1] = t2;
	half_edges = SparseMatrix_coordinate_form_add_entries(half_edges, 1, &i, &jj, &ne);
	half_edges = SparseMatrix_coordinate_form_add_entries(half_edges, 1, &jj, &i, &ne);
	ne++;

	edge_table[ne*2] = t2;/*t2->t1*/
	edge_table[ne*2+1] = t1;
	half_edges = SparseMatrix_coordinate_form_add_entries(half_edges, 1, &i, &jj, &ne);
	half_edges = SparseMatrix_coordinate_form_add_entries(half_edges, 1, &jj, &i, &ne);	


	ne++;
	j++;
      }
    }
  }
  assert(E->nz >= ne);

  cycle = MALLOC(sizeof(int)*ne*2);
  B = SparseMatrix_from_coordinate_format_not_compacted(half_edges, SUM_REPEATED_NONE);
  SparseMatrix_delete(half_edges);half_edges = B;

  edge_cycle_map = MALLOC(sizeof(int)*ne);
  emask = MALLOC(sizeof(int)*ne);
  for (i = 0; i < ne; i++) edge_cycle_map[i] = NOT_ON_CYCLE;
  for (i = 0; i < ne; i++) emask[i] = -1;

  ie = half_edges->ia;
  je = half_edges->ja;
  e = (int*) half_edges->a;
  elist = MALLOC(sizeof(int)*(nt)*3);
  for (i = 0; i < nt; i++) elist[i*edim + 2] = 0;

  *polys = SparseMatrix_new(ncomps, nt, 1, MATRIX_TYPE_INTEGER, FORMAT_COORD);

  for (i = 0; i < ncomps; i++){
    if (DEBUG_CYCLE) fprintf(stderr, "\n ============  comp %d has %d members\n",i, comps_ptr[i+1]-comps_ptr[i]);
    for (k = comps_ptr[i]; k < comps_ptr[i+1]; k++){
      ii = comps[k];
      mask[ii] = i;
      duplicate = NO_DUPLICATE;
      if (DEBUG_CYCLE) fprintf(stderr,"member = %d has %d neighbors\n",ii, ie[ii+1]-ie[ii]);
      for (j = ie[ii]; j < ie[ii+1]; j++){
	jj = je[j];
	ee = e[j];
	t1 = edge_head(ee);
	if (DEBUG_CYCLE) fprintf(stderr," linked with %d using half-edge %d, {head,tail} of the edge = {%d, %d}\n",jj, ee, t1, edge_tail(ee));
	nlink = elist[t1*edim + 2]%2;
	elist[t1*edim + nlink] = ee;/* t1->t2*/
	elist[t1*edim + 2]++;

	if (edge_cycle_map[ee] != NOT_ON_CYCLE) duplicate = ee;
	emask[ee] = ii;
      }

      if (duplicate == NO_DUPLICATE){
	/* this must be the first time the cycle is being established, a new voro cell*/
	ecur = ee;
	cycle_head = ecur;
	cycle_next(ecur) = ecur;
	cycle_prev(ecur) = ecur;
	edge_cycle_map[ecur] = 1;
	head = cur = edge_head(ecur);
	next = edge_tail(ecur);
	if (DEBUG_CYCLE) fprintf(stderr, "NEW CYCLE\n starting with edge %d, {head,tail}={%d,%d}\n", ee, head, next);
	while (next != head){
	  enext = neighbor(next, 0, edim, elist);/* two voro edges linked with triangle "next" */
	  if ((edge_head(enext) == cur && edge_tail(enext) == next)
	      || (edge_head(enext) == next && edge_tail(enext) == cur)){/* same edge */
	    enext = neighbor(next, 1, edim, elist);
	  };
	  if (DEBUG_CYCLE) fprintf(stderr, "cur edge = %d, next edge %d, {head,tail}={%d,%d},\n",ecur, enext, edge_head(enext), edge_tail(enext));
	  nn = edge_head(enext);
	  if (nn == next) nn = edge_tail(enext);
	  cycle_next(enext) = cycle_next(ecur);
	  cycle_prev(enext) = ecur;
	  cycle_next(ecur) = enext;
	  cycle_prev(ee) = enext;
	  edge_cycle_map[enext] = 1;

	  ecur = enext;
	  cur = next;
	  next = nn;
	} 
	if (DEBUG_CYCLE) cycle_print(ee, cycle,edge_table);
      } else {
	/* we found a duplicate edge, remove that, and all contiguous neighbors that overlap with the current voro
	 */
	ecur = ee = duplicate;
	while (emask[ecur] == ii){
	  /* contigous overlapping edges, Cycling is not possible
	     since the cycle can not complete surround the new voro cell and yet
	     do not contain any other edges
	  */
	  ecur = cycle_next(ecur);
	}
	if (DEBUG_CYCLE) fprintf(stderr," duplicating edge = %d, starting from the a non-duplicating edge %d, search backwards\n",ee, ecur);

	ecur = cycle_prev(ecur);
	efirst = ecur;
	while (emask[ecur] == ii){
	  if (DEBUG_CYCLE) fprintf(stderr," remove edge %d (%d--%d)\n",ecur, edge_head(ecur), edge_tail(ecur));
	  /* short this duplicating edge */
	  edge_cycle_map[ecur] = NOT_ON_CYCLE;
	  enext = cycle_next(ecur);
	  eprev = cycle_prev(ecur);
	  cycle_next(ecur) = ecur;/* isolate this edge */
	  cycle_prev(ecur) = ecur;
	  cycle_next(eprev) = enext;/* short */
	  cycle_prev(enext) = eprev;
	  elast = ecur;/* record the last removed edge */
	  ecur = eprev;
	}

	if (DEBUG_CYCLE) {
	  fprintf(stderr, "remaining (broken) cycle = ");
	  cycle_print(cycle_next(ecur), cycle,edge_table);
	}

	/* we now have a broken cycle of head = edge_tail(ecur) and tail = edge_head(cycle_next(ecur)) */
	ehead = ecur; etail = cycle_next(ecur);
	cycle_head = ehead;
	head = edge_tail(ehead); 
	tail = edge_head(etail);

	/* pick an edge ev from head in the voro that is a removed edge: since the removed edges form a path starting from
	   efirst, and at elast (head of elast is head), usually we just need to check that ev is not the same as elast,
	   but in the case of a voro filling in a hole, we also need to check that ev is not efirst,
	   since in this case every edge of the voro cell is removed
	 */
	ecur = neighbor(head, 0, edim, elist);
	if (same_edge(ecur, elast, edge_table)){
	  ecur = neighbor(head, 1, edim, elist);
	};

	if (DEBUG_CYCLE) fprintf(stderr, "forwarding now from edge %d = {%d, %d}, try to reach vtx %d, first edge from voro = %d\n",
				 ehead, edge_head(ehead), edge_tail(ehead), tail, ecur);

	/* now go along voro edges till we reach the tail of the broken cycle*/
	cycle_next(ehead) = ecur;
	cycle_prev(ecur) = ehead;
	cycle_prev(etail) = ecur;
	cycle_next(ecur) = etail;
	if (same_edge(ecur, efirst, edge_table)){
	  if (DEBUG_CYCLE) fprintf(stderr, "this voro cell fill in a hole completely!!!!\n");
	} else {
	
	  edge_cycle_map[ecur] = 1;
	  head = cur = edge_head(ecur);
	  next = edge_tail(ecur);
	  if (DEBUG_CYCLE) fprintf(stderr, "starting with edge %d, {head,tail}={%d,%d}\n", ecur, head, next);
	  while (next != tail){
	    enext = neighbor(next, 0, edim, elist);/* two voro edges linked with triangle "next" */
	    if ((edge_head(enext) == cur && edge_tail(enext) == next)
		|| (edge_head(enext) == next && edge_tail(enext) == cur)){/* same edge */
	      enext = neighbor(next, 1, edim, elist);
	    };
	    if (DEBUG_CYCLE) fprintf(stderr, "cur edge = %d, next edge %d, {head,tail}={%d,%d},\n",ecur, enext, edge_head(enext), edge_tail(enext));


	    nn = edge_head(enext);
	    if (nn == next) nn = edge_tail(enext);
	    cycle_next(enext) = cycle_next(ecur);
	    cycle_prev(enext) = ecur;
	    cycle_next(ecur) = enext;
	    cycle_prev(etail) = enext;
	    edge_cycle_map[enext] = 1;
	    
	    ecur = enext;
	    cur = next;
	    next = nn;
	  }
	}
	if (DEBUG_CYCLE && 0) {
	  cycle_print(etail, cycle,edge_table);
	  plot_cycle(etail, cycle,edge_table, x_poly);
	  printf(",");
	}

      }
      
    }
    /* done this component, load to sparse matrix, unset edge_map*/
    ecur = cycle_head;
    while ((enext = cycle_next(ecur)) != cycle_head){
      edge_cycle_map[ecur] = NOT_ON_CYCLE;
      head = edge_head(ecur);
      SparseMatrix_coordinate_form_add_entries(*polys, 1, &i, &head, &i);
      ecur = enext;
    }
    edge_cycle_map[ecur] = NOT_ON_CYCLE;
    head = edge_head(ecur); tail = edge_tail(ecur);
    SparseMatrix_coordinate_form_add_entries(*polys, 1, &i, &head, &i);
    SparseMatrix_coordinate_form_add_entries(*polys, 1, &i, &tail, &i);


    /* unset edge_map */
  }



  B = SparseMatrix_from_coordinate_format_not_compacted(*polys, SUM_REPEATED_NONE);
  SparseMatrix_delete(*polys);
  *polys = B;
  
  SparseMatrix_delete(half_edges);
  FREE(cycle);
  FREE(edge_cycle_map);
  FREE(elist);
  FREE(emask);
  FREE(edge_table);

}
static void get_polygons(int exclude_random, int n, int nrandom, int dim, SparseMatrix graph, real *xcombined, int *grouping, 
			 int nt, struct Triangle *Tp, SparseMatrix E, int *nverts, real **x_poly, 
			 int *npolys, SparseMatrix *poly_lines, SparseMatrix *polys, int **polys_groups, SparseMatrix *poly_point_map, SparseMatrix *country_graph,
			 int *flag){
  int i, j;
  int *mask;
  int *groups;
  int maxgrp;
  int *comps = NULL, *comps_ptr = NULL, ncomps;
  int GRP_RANDOM, GRP_BBOX;
  SparseMatrix B;

  *flag = 0;

  assert(dim == 2);
  *nverts = nt;
 
  groups = MALLOC(sizeof(int)*(n + nrandom));
  maxgrp = grouping[0];
  for (i = 0; i < n; i++) {
    maxgrp = MAX(maxgrp, grouping[i]);
    groups[i] = grouping[i];
  }

  GRP_RANDOM = maxgrp + 1; GRP_BBOX = maxgrp + 2;
  for (i = n; i < n + nrandom - 4; i++) {/* all random points in the same group */
    groups[i] = GRP_RANDOM;
  }
  for (i = n + nrandom - 4; i < n + nrandom; i++) {/* last 4 pts of the expanded bonding box in the same group */
    groups[i] = GRP_BBOX;
  }
  
  /* finding connected componts: vertices that are connected in the triangle graph, as well as in the same group */
  mask = MALLOC(sizeof(int)*MAX(n + nrandom, 2*nt));
  conn_comp(n + nrandom, E, groups, poly_point_map);

  ncomps = (*poly_point_map)->m;
  comps = (*poly_point_map)->ja;
  comps_ptr = (*poly_point_map)->ia;

  if (exclude_random){
    /* connected components are such that  the random points and the bounding box 4 points forms the last
     remaining components */
    for (i = ncomps - 1; i >= 0; i--) {
      if ((groups[comps[comps_ptr[i]]] != GRP_RANDOM) &&
	  (groups[comps[comps_ptr[i]]] != GRP_BBOX)) break;
    }
    ncomps = i + 1;
    if (Verbose) fprintf(stderr," ncomps = %d\n",ncomps);
  } else {/* alwasy exclud bounding box */
    for (i = ncomps - 1; i >= 0; i--) {
      if (groups[comps[comps_ptr[i]]] != GRP_BBOX) break;
    }
    ncomps = i + 1;
    if (Verbose) fprintf(stderr," ncomops = %d\n",ncomps);
  }
  *npolys = ncomps;

  *x_poly = MALLOC(sizeof(real)*dim*nt);
  for (i = 0; i < nt; i++){
    for (j = 0; j < dim; j++){
      (*x_poly)[i*dim+j] = (Tp[i]).center[j];
    }
  }
  

  /*============================================================

    polygon outlines 

    ============================================================*/
  get_poly_lines(exclude_random, nt, graph, E, ncomps, comps_ptr, comps, groups, mask, poly_lines, polys_groups, GRP_RANDOM, GRP_BBOX);

  /*============================================================

    polygon solids

    ============================================================*/
  get_polygon_solids(exclude_random, nt, E, ncomps, comps_ptr, comps, groups, mask, *x_poly, polys, polys_groups, GRP_RANDOM, GRP_BBOX);

  B = get_country_graph(n, E, groups, poly_point_map, GRP_RANDOM, GRP_BBOX);
  *country_graph = B;

  FREE(groups);
  FREE(mask);

}


int make_map_internal(int exclude_random, int include_OK_points,
		      int n, int dim, real *x0, int *grouping0, SparseMatrix graph, real bounding_box_margin[], int *nrandom, int nedgep, 
		      real shore_depth_tol, real edge_bridge_tol, real **xcombined, int *nverts, real **x_poly, 
		      int *npolys, SparseMatrix *poly_lines, SparseMatrix *polys, int **polys_groups, SparseMatrix *poly_point_map,
		      SparseMatrix *country_graph, int highlight_cluster, int *flag){


  real xmax[2], xmin[2], area, *x = x0;
  int i, j;
  QuadTree qt;
  int dim2 = 2, nn = 0;
  int max_qtree_level = 10;
  real ymin[2], min;
  int imin, nz, nzok = 0, nzok0 = 0, nt;
  real *xran, point[2];
  struct Triangle *Tp;
  SparseMatrix E;
  real boxsize[2];
  int INCLUDE_OK_POINTS = include_OK_points;/* OK points are random points inserted and found to be within shore_depth_tol of real/artificial points,
			      including them instead of throwing away increase realism of boundary */
  int *grouping = grouping0;

  int HIGHLIGHT_SET = highlight_cluster;

  *flag = 0;
  for (j = 0; j < dim2; j++) {
    xmax[j] = x[j];
    xmin[j] = x[j];
  }

  for (i = 0; i < n; i++){
    for (j = 0; j < dim2; j++) {
      xmax[j] = MAX(xmax[j], x[i*dim+j]);
      xmin[j] = MIN(xmin[j], x[i*dim+j]);
    }
  }
  boxsize[0] = (xmax[0] - xmin[0]);
  boxsize[1] = (xmax[1] - xmin[1]);
  area = boxsize[0]*boxsize[1];

  if (*nrandom == 0) {
    *nrandom = n;
  } else if (*nrandom < 0){
    *nrandom = -(*nrandom)*n;
  } else if (*nrandom < 4) {/* by default we add 4 point on 4 corners anyway */
    *nrandom = 0;
  } else {
    *nrandom -= 4;
  }
  if (Verbose) fprintf(stderr,"nrandom=%d\n",*nrandom);

  if (Verbose) fprintf(stderr,"nrandom=%d\n",*nrandom);

  if (shore_depth_tol < 0) shore_depth_tol = sqrt(area/(real) n); /* set to average distance for random distribution */


  /* add artificial points along each edge to avoid as much as possible 
     two connected components be separated due to small shore depth */
  {
    int nz;
    real *y;
    int k, t, np=nedgep;
    if (graph && np){
      fprintf(stderr,"add art np = %d\n",np);
      nz = graph->nz;
      y = MALLOC(sizeof(real)*(dim*n + dim*nz*np));
      for (i = 0; i < n*dim; i++) y[i] = x[i];
      grouping = MALLOC(sizeof(int)*(n + nz*np));
      for (i = 0; i < n; i++) grouping[i] = grouping0[i];
      nz = n;
      for (i = 0; i < graph->m; i++){

	for (j = graph->ia[i]; j < graph->ia[i+1]; j++){
	  if (!HIGHLIGHT_SET || (grouping[i] == grouping[graph->ja[j]] && grouping[i] == HIGHLIGHT_SET)){
	    for (t = 0; t < np; t++){
	      for (k = 0; k < dim; k++){
		y[nz*dim+k] = t/((real) np)*x[i*dim+k] + (1-t/((real) np))*x[(graph->ja[j])*dim + k];
	      }
	      assert(n + (nz-n)*np + t < n + nz*np && n + (nz-n)*np + t >= 0);
	      if (t/((real) np) > 0.5){
		grouping[nz] = grouping[i];
	      } else {
		grouping[nz] = grouping[graph->ja[j]];
	      }
	      nz++;
	    }
	  }
	}
      }
      fprintf(stderr, "after adding edge points, n:%d->%d\n",n, nz);
      n = nz;
      x = y;
      qt = QuadTree_new_from_point_list(dim, nz, max_qtree_level, y, NULL);
    } else {
      qt = QuadTree_new_from_point_list(dim, n, max_qtree_level, x, NULL);
    }
  }
  graph = NULL;


  /* generate random points for lake/sea effect */
  if (*nrandom != 0){
    for (i = 0; i < dim2; i++) {
      if (bounding_box_margin[i] > 0){
	xmin[i] -= bounding_box_margin[i];
	xmax[i] += bounding_box_margin[i];
      } else if (bounding_box_margin[i] == 0) {/* auto bounding box */
	xmin[i] -= MAX(boxsize[i]*0.2, 2.*shore_depth_tol);
	xmax[i] += MAX(boxsize[i]*0.2, 2*shore_depth_tol);
      } else {
	xmin[i] -= boxsize[i]*(-bounding_box_margin[i]);
	xmax[i] += boxsize[i]*(-bounding_box_margin[i]);
      }
    }
    if (*nrandom < 0) {
      real n1, n2, area2;
      area2 = (xmax[1] - xmin[1])*(xmax[0] - xmin[0]);
      n1 = (int) area2/(shore_depth_tol*shore_depth_tol);
      n2 = n*((int) area2/area);
      *nrandom = MAX(n1, n2);
    }
    srand(123);
    xran = MALLOC(sizeof(real)*(*nrandom + 4)*dim2);
    nz = 0;
    if (INCLUDE_OK_POINTS){
      int *grouping2;
      nzok0 = nzok = *nrandom - 1;/* points that are within tolerance of real or artificial points */
      grouping2 = MALLOC(sizeof(int)*(n + *nrandom));
      MEMCPY(grouping2, grouping, sizeof(int)*n);
      grouping = grouping2;
    }
    nn = n;

    for (i = 0; i < *nrandom; i++){

      for (j = 0; j < dim2; j++){
	point[j] = xmin[j] + (xmax[j] - xmin[j])*drand();
      }
      
      QuadTree_get_nearest(qt, point, ymin, &imin, &min, flag);
      assert(!(*flag));

      if (min > shore_depth_tol){/* point not too close, accepted */
	for (j = 0; j < dim2; j++){
	  xran[nz*dim2+j] = point[j];
	}
	nz++;
      } else if (INCLUDE_OK_POINTS && min > shore_depth_tol/10){/* avoid duplicate points */
	for (j = 0; j < dim2; j++){
	  xran[nzok*dim2+j] = point[j];
	}
	grouping[nn++] = grouping[imin];
	nzok--;

      }

    }
    *nrandom = nz;
    if (Verbose) fprintf( stderr, "nn nrandom=%d\n",*nrandom);
  } else {
    xran = MALLOC(sizeof(real)*4*dim2);
  }



  /* add 4 corners even if nrandom = 0. The corners should be further away from the other points to avoid skinny triangles */
  for (i = 0; i < dim2; i++) xmin[i] -= 0.2*(xmax[i]-xmin[i]);
  for (i = 0; i < dim2; i++) xmax[i] += 0.2*(xmax[i]-xmin[i]);
  i = *nrandom;
  for (j = 0; j < dim2; j++) xran[i*dim2+j] = xmin[j];
  i++;
  for (j = 0; j < dim2; j++) xran[i*dim2+j] = xmax[j];
  i++;
  xran[i*dim2] = xmin[0]; xran[i*dim2+1] = xmax[1];
  i++;
  xran[i*dim2] = xmax[0]; xran[i*dim2+1] = xmin[1];
  *nrandom += 4;


  if (INCLUDE_OK_POINTS){
    *xcombined = MALLOC(sizeof(real)*(nn+*nrandom)*dim2);
  } else {
    *xcombined = MALLOC(sizeof(real)*(n+*nrandom)*dim2);
  }
  for (i = 0; i < n; i++) {
    for (j = 0; j < dim2; j++) (*xcombined)[i*dim2+j] = x[i*dim+j];
  }
  for (i = 0; i < *nrandom; i++) {
    for (j = 0; j < dim2; j++) (*xcombined)[(i + nn)*dim2+j] = xran[i*dim+j];
  }

  if (INCLUDE_OK_POINTS){
    for (i = 0; i < nn - n; i++) {
      for (j = 0; j < dim2; j++) (*xcombined)[(i + n)*dim2+j] = xran[(nzok0 - i)*dim+j];
    }
    n = nn;
  }


  {
    int nz, nh = 0;/* the set to highlight */
    real *xtemp;
    if (HIGHLIGHT_SET){
      if (Verbose) fprintf(stderr," highlight cluster %d, n = %d\n",HIGHLIGHT_SET, n);
      xtemp = MALLOC(sizeof(real)*n*dim);
      /* shift set to the beginning */
      nz = 0;
      for (i = 0; i < n; i++){
	if (grouping[i] == HIGHLIGHT_SET){
	  nh++;
	  for (j = 0; j < dim; j++){
	    xtemp[nz++] = x[i*dim+j];
	  }
	}
      }
      for (i = 0; i < n; i++){
	if (grouping[i] != HIGHLIGHT_SET){
	  for (j = 0; j < dim; j++){
	    xtemp[nz++] = x[i*dim+j];
	  }
	}
      }
      assert(nz == n*dim);
      for (i = 0; i < nh; i++){
	grouping[i] = 1;
      }
      for (i = nh; i < n; i++){
	grouping[i] = 2;
      }
      MEMCPY(*xcombined, xtemp, n*dim*sizeof(real));
      *nrandom = *nrandom + n - nh;/* count everything except cluster HIGHLIGHT_SET as random */
      n = nh;
      if (Verbose) fprintf(stderr,"nh = %d\n",nh);
      FREE(xtemp);
    }
  }

  get_tri(n + *nrandom, dim2, *xcombined, &nt, &Tp, &E, flag);
  get_polygons(exclude_random, n, *nrandom, dim2, graph, *xcombined, grouping, nt, Tp, E, nverts, x_poly, npolys, poly_lines, polys, polys_groups, 
	       poly_point_map, country_graph, flag);
  /*
  {
    plot_voronoi(n + *nrandom, E, Tp);
    printf("(*voronoi*),");
    
  }
*/




  SparseMatrix_delete(E);
  FREE(Tp);
  FREE(xran);
  if (grouping != grouping0) FREE(grouping);
  if (x != x0) FREE(x);
  return 0;
}


int make_map_from_point_groups(int exclude_random, int include_OK_points,
			       int n, int dim, real *x, int *grouping, SparseMatrix graph, real bounding_box_margin[], int *nrandom,
			       real shore_depth_tol, real edge_bridge_tol, real **xcombined, int *nverts, real **x_poly, 
			       int *npolys, SparseMatrix *poly_lines, SparseMatrix *polys, int **polys_groups, SparseMatrix *poly_point_map,
			       SparseMatrix *country_graph, int *flag){

  /* create a list of polygons from a list of points in 2D. Points belong to groups. Points in the same group that are also close 
     gemetrically will be in the same polygon describing the outline of the group.

     input: 
     exclude_random:
     include_OK_points: OK points are random points inserted and found to be within shore_depth_tol of real/artificial points,
     .                  including them instead of throwing away increase realism of boundary 
     n: number of points
     dim: dimension of the points. If dim > 2, only the first 2D is used.
     x: coordinates
     grouping: which group each of the vertex belongs to
     graph: the link structure between points. If graph == NULL, this is not used. otherwise
     .      it is assumed that matrix is symmetric and the graph is undirected
     bounding_box_margin: margins used to form the bounding box. Dimension 2.
     .      if negative, it is taken as relative. i.e., -0.5 means a margin of 0.5*box_size
     nrandom (inout): number of random points to insert in the bounding box to figure out lakes and seas.
     .        If nrandom = 0, no points are inserted, if nrandom < 0, the number is decided automatically.
     .        On exit, it is the actual number of random points used. The last 4 "random" points is always the
     .        
     shore_depth_tol: nrandom random points are inserted in the bounding box of the points,
     .      such random points are then weeded out if it is within distance of shore_depth_tol from 
     .      real points. If < 0, auto assigned
     edge_bridge_tol: insert points on edges to give an bridge effect.These points will be evenly spaced
     .       along each edge, and be less than a distance of edge_bridge_tol from each other and from the two ends of the edge.
     .       If < 0, -edge_bridge_tol is the average number of points inserted per half edge
     output:
     xcombined: combined points which contains n + ncombined number of points, dimension 2x(n+nrandom)
     npolys: number of polygons generated to reprsent the real points, the edge insertion points, and the sea/lake points.
     nverts: number of vertices in the Voronoi diagram
     x_poly: the 2D coordinates of these polygons
     poly_lines: the sparse matrix representation of the polygon indices, as well as their identity. The matrix is of size
     .       npolygons x nverts. The i-th polygon is formed by linking vertices with index in the i-th row of the sparse matrix.
     .       Each row is of the form {{i,j1,m},...{i,jk,m},{i,j1,m},{i,l1,m+1},...}, where j1--j2--jk--j1 form one loop,
     .       and l1 -- l2 -- ... form another. Each row can have more than 1 loop only when the connected region the polylines represent
     .       has at least 1 holes.
     polys: the sparse matrix representation of the polygon indices, as well as their identity. The matrix is of size
     .       npolygons x nverts. The i-th polygon is formed by linking vertices with index in the i-th row of the sparse matrix.
     .       Unlike poly_lines, here each row represent an one stroke drawing of the SOLID polygon, vertices
     .       along this path may repeat
     polys_groups: the group (color) each polygon belongs to, this include all groups of the real points,
     .       plus the random point group and the bounding box group
     poly_point_map: a matrix of dimension npolys x (n + nrandom), poly_point_map[i,j] != 0 if polygon i contains the point j.
     .  If j < n, it is the original point, otherwise it is 
     country_graph: shows which country is a neighbor of which country.
     .     if country i and country j are neighbor, then the {i,j} entry is the total number of vertices that
     .     belongs to i and j, and share an edge of the triangulation. In addition, {i,i} and {j,j} have values equal 
     .     to the number of vertices in each of the countries. If the input "grouping" has negative or zero value, then
     .     country_graph = NULL. 
     
  */
  int res;
  int nedgep = 0;
  int highlight_cluster = FALSE;
  /* get poly outlines */
  res = make_map_internal(exclude_random, include_OK_points, n, dim, x, grouping, graph, bounding_box_margin, nrandom, nedgep,
		  shore_depth_tol, edge_bridge_tol, xcombined, nverts, x_poly, 
			  npolys, poly_lines, polys, polys_groups, poly_point_map, country_graph, highlight_cluster, flag);

  printf("Show[{");

  plot_points(n + *nrandom, dim, *xcombined);


  printf(",");
  plot_polys(TRUE, *poly_lines, *x_poly, *polys_groups, NULL, NULL, NULL);

  printf("}]\n");

  return res;
}

static void add_point(int *n, int igrp, real **x, int *nmax, real point[], int **groups){

  if (*n >= *nmax){
    *nmax = MAX((int) 0.2*(*n), 20) + *n;
    *x = REALLOC(*x, sizeof(real)*2*(*nmax));
    *groups = REALLOC(*groups, sizeof(int)*(*nmax));
  }

  (*x)[(*n)*2] = point[0];
  (*x)[(*n)*2+1] = point[1];
  (*groups)[*n] = igrp;
  (*n)++;

}

static void get_boundingbox(int n, int dim, real *x, real *width, real *bbox){
  int i;
  bbox[0] = bbox[1] = x[0];
  bbox[2] = bbox[3] = x[1];
  
  for (i = 0; i < n; i++){
    bbox[0] = MIN(bbox[0], x[i*dim] - width[i*dim]);
    bbox[1] = MAX(bbox[1], x[i*dim] + width[i*dim]);
    bbox[2] = MIN(bbox[2], x[i*dim + 1] - width[i*dim+1]);
    bbox[3] = MAX(bbox[3], x[i*dim + 1] + width[i*dim+1]);
  }
}


int make_map_from_rectangle_groups(int exclude_random, int include_OK_points,
				   int n, int dim, real *x, real *sizes, 
				   int *grouping, SparseMatrix graph0, real bounding_box_margin[], int *nrandom, int *nart, int nedgep, 
				   real shore_depth_tol, real edge_bridge_tol,
				   real **xcombined, int *nverts, real **x_poly, 
				   int *npolys, SparseMatrix *poly_lines, SparseMatrix *polys, int **polys_groups, SparseMatrix *poly_point_map, 
				   SparseMatrix *country_graph, int highlight_cluster, int *flag){

  /* create a list of polygons from a list of rectangles in 2D. rectangles belong to groups. rectangles in the same group that are also close 
     gemetrically will be in the same polygon describing the outline of the group. The main difference for this function and
     make_map_from_point_groups is that in this function, the input are points with width/heights, and we try not to place
     "lakes" inside these rectangles. The is achieved approximately by adding artificial points along the perimeter of the rectangles,
     as well as near the center.

     input:
     include_OK_points: OK points are random points inserted and found to be within shore_depth_tol of real/artificial points,
     .                  including them instead of throwing away increase realism of boundary 
     n: number of points
     dim: dimension of the points. If dim > 2, only the first 2D is used.
     x: coordinates
     sizes: width and height
     grouping: which group each of the vertex belongs to
     graph: the link structure between points. If graph == NULL, this is not used. otherwise
     .      it is assumed that matrix is symmetric and the graph is undirected
     bounding_box_margin: margins used to form the bounding box. Dimension 2.
     .      if negative, it is taken as relative. i.e., -0.5 means a margin of 0.5*box_size
     nrandom (inout): number of random points to insert in the bounding box to figure out lakes and seas.
     .        If nrandom = 0, no points are inserted, if nrandom < 0, the number is decided automatically.
     .        On exit, it is the actual number of random points used. The last 4 "random" points is always the
     .        
     nart: on entry, number of artificla points to be added along rach side of a rectangle enclosing the labels. if < 0, auto-selected.
     . On exit, actual number of artificial points added.
     nedgep: number of artificial points are adding along edges to establish as much as possible a bright between nodes 
     .       connected by the edge, and avoid islands that are connected. k = 0 mean no points.
     shore_depth_tol: nrandom random points are inserted in the bounding box of the points,
     .      such random points are then weeded out if it is within distance of shore_depth_tol from 
     .      real points. If < 0, auto assigned
     edge_bridge_tol: insert points on edges to give an bridge effect.These points will be evenly spaced
     .       along each edge, and be less than a distance of edge_bridge_tol from each other and from the two ends of the edge.
     .       If < 0, -edge_bridge_tol is the average number of points inserted per half edge

     output:
     xcombined: combined points which contains n + ncombined number of points, dimension 2x(n+nrandom)
     npolys: number of polygons generated to reprsent the real points, the edge insertion points, and the sea/lake points.
     nverts: number of vertices in the Voronoi diagram
     x_poly: the 2D coordinates of these polygons, dimension nverts*2
     poly_lines: the sparse matrix representation of the polygon indices, as well as their identity. The matrix is of size
     .       npolygons x nverts. The i-th polygon is formed by linking vertices with index in the i-th row of the sparse matrix.
     .       Each row is of the form {{i,j1,m},...{i,jk,m},{i,j1,m},{i,l1,m+1},...}, where j1--j2--jk--j1 form one loop,
     .       and l1 -- l2 -- ... form another. Each row can have more than 1 loop only when the connected region the polylines represent
     .       has at least 1 holes.
     polys: the sparse matrix representation of the polygon indices, as well as their identity. The matrix is of size
     .       npolygons x nverts. The i-th polygon is formed by linking vertices with index in the i-th row of the sparse matrix.
     .       Unlike poly_lines, here each row represent an one stroke drawing of the SOLID polygon, vertices
     .       along this path may repeat
     polys_groups: the group (color) each polygon belongs to, this include all groups of the real points,
     .       plus the random point group and the bounding box group
     poly_point_map: a matrix of dimension npolys x (n + nrandom), poly_point_map[i,j] != 0 if polygon i contains the point j.
     .  If j < n, it is the original point, otherwise it is artificial point (forming the rectangle around a label) or random points.
    country_graph: shows which country is a neighbor of which country.
     .     if country i and country j are neighbor, then the {i,j} entry is the total number of vertices that
     .     belongs to i and j, and share an edge of the triangulation. In addition, {i,i} and {j,j} have values equal 
     .     to the number of vertices in each of the countries. If the input "grouping" has negative or zero value, then
     .     country_graph = NULL. 

     
  */
    real dist, avgdist;
  real *X;
  int N, nmax, i, j, k, igrp;
  int *groups, K = *nart;/* average number of points added per side of rectangle */

  real avgsize[2],  avgsz, h[2], p1, p0;
  real point[2];
  int nadded[2];
  int res;
  real delta[2];
  SparseMatrix graph = graph0;
  real bbox[4];

  if (K < 0){
    K = (int) 10/(1+n/400.);/* 0 if n > 3600*/
  }
  *nart = 0;
  if (Verbose){
    int maxgp = grouping[0];
    int mingp = grouping[0];
    for (i = 0; i < n; i++) {
      maxgp = MAX(maxgp, grouping[i]);
      mingp = MIN(mingp, grouping[i]);
    }
    fprintf(stderr, "max grouping - min grouping + 1 = %d\n",maxgp - mingp + 1); 
  }

  if (!sizes){
    return make_map_internal(exclude_random, include_OK_points, n, dim, x, grouping, graph, bounding_box_margin, nrandom, nedgep, 
			    shore_depth_tol, edge_bridge_tol, xcombined, nverts, x_poly, 
			     npolys, poly_lines, polys, polys_groups, poly_point_map, country_graph, highlight_cluster, flag);
  } else {

    /* add artificial node due to node sizes */
    avgsize[0] = 0;
    avgsize[1] = 0;
    for (i = 0; i < n; i++){
      for (j = 0; j < 2; j++) {
	avgsize[j] += sizes[i*dim+j];
      }
    }
    for (i = 0; i < 2; i++) avgsize[i] /= n;
    avgsz = 0.5*(avgsize[0] + avgsize[1]);
    if (Verbose) fprintf(stderr, "avgsize = {%f, %f}\n",avgsize[0], avgsize[1]);

    nmax = 2*n;
    X = MALLOC(sizeof(real)*dim*(n+nmax));
    groups = MALLOC(sizeof(int)*(n+nmax));
    for (i = 0; i < n; i++) {
      groups[i] = grouping[i];
      for (j = 0; j < 2; j++){
	X[i*2+j] = x[i*dim+j];
      }
    }
    N = n;

    if (shore_depth_tol < 0) {
      shore_depth_tol = -(shore_depth_tol)*avgsz;
    } else if (shore_depth_tol == 0){
      real area;
      get_boundingbox(n, dim, x, sizes, bbox);
      //shore_depth_tol = MIN(bbox[1] - bbox[0], bbox[3] - bbox[2])*0.05;
      area = (bbox[1] - bbox[0])*(bbox[3] - bbox[2]);
      shore_depth_tol = sqrt(area/(real) n); 
      if (Verbose) fprintf(stderr,"setting shore length ======%f\n",shore_depth_tol);
    } else {
      /*
      get_boundingbox(n, dim, x, sizes, bbox);
      shore_depth_tol = MIN(bbox[1] - bbox[0], bbox[3] - bbox[2])*shore_depth_tol;
      */

    }

    /* add artificial points in an anti-clockwise fashion */

    if (K > 0){
      delta[0] = .5*avgsize[0]/K; delta[1] = .5*avgsize[1]/K;/* small pertubation to make boundary between labels looks more fractal */
    } else {
      delta[0] = delta[1] = 0.;
    }
    for (i = 0; i < n; i++){
      igrp = grouping[i];
      for (j = 0; j < 2; j++) {
	if (avgsz == 0){
	  nadded[j] = 0;
	} else {
	  nadded[j] = (int) K*sizes[i*dim+j]/avgsz;
	}
      }

      /*top: left to right */
      if (nadded[0] > 0){
	h[0] = sizes[i*dim]/nadded[0];
	point[0] = x[i*dim] - sizes[i*dim]/2;
	p1 = point[1] = x[i*dim+1] + sizes[i*dim + 1]/2;
	add_point(&N, igrp, &X, &nmax, point, &groups);
	for (k = 0; k < nadded[0] - 1; k++){
	  point[0] += h[0];
	  point[1] = p1 + (0.5-drand())*delta[1];
	  add_point(&N, igrp, &X, &nmax, point, &groups);
	}
	
	/* bot: right to left */
	point[0] = x[i*dim] + sizes[i*dim]/2;
	p1 = point[1] = x[i*dim+1] - sizes[i*dim + 1]/2;
	add_point(&N, igrp, &X, &nmax, point, &groups);
	for (k = 0; k < nadded[0] - 1; k++){
	  point[0] -= h[0];
	  point[1] = p1 + (0.5-drand())*delta[1];
	  add_point(&N, igrp, &X, &nmax, point, &groups);
	}
      }

      if (nadded[1] > 0){	
	/* left: bot to top */
	h[1] = sizes[i*dim + 1]/nadded[1];
	p0 = point[0] = x[i*dim] - sizes[i*dim]/2;
	point[1] = x[i*dim+1] - sizes[i*dim + 1]/2;
	add_point(&N, igrp, &X, &nmax, point, &groups);
	for (k = 0; k < nadded[1] - 1; k++){
	  point[0] = p0 + (0.5-drand())*delta[0];
	  point[1] += h[1];
	  add_point(&N, igrp, &X, &nmax, point, &groups);
	}
	
	/* right: top to bot */
	p0 = point[0] = x[i*dim] + sizes[i*dim]/2;
	point[1] = x[i*dim+1] + sizes[i*dim + 1]/2;
	add_point(&N, igrp, &X, &nmax, point, &groups);
	for (k = 0; k < nadded[1] - 1; k++){
	  point[0] = p0 + (0.5-drand())*delta[0];
	  point[1] -= h[1];
	  add_point(&N, igrp, &X, &nmax, point, &groups);
	}	
      }
      *nart = N - n;

    }/* done adding artificial points due to node size*/


    /* add artificial node due to edges */
    if (graph && edge_bridge_tol != 0){
      int *ia = graph->ia, *ja = graph->ja, nz = 0, jj;
      int KB;

      graph = SparseMatrix_symmetrize(graph, TRUE);
      ia = graph->ia; ja = graph->ja;
      dist=avgdist = 0.;
      for (i = 0; i < n; i++){
	for (j = ia[i]; j < ia[i+1]; j++){
	  jj = ja[j];
	  if (jj <= i) continue;
	  dist = distance(x, dim, i, jj);
	  avgdist += dist;
	  nz++;
	}
      }
      avgdist /= nz;
      if (edge_bridge_tol < 0){
	KB = (int) (-edge_bridge_tol);
      } else {
	KB = (int) (avgdist/edge_bridge_tol);
      }

      assert(avgdist > 0);
      for (i = 0; i < n; i++){
	for (j = ia[i]; j < ia[i+1]; j++){
	  jj = ja[j];
	  if (jj <= i) continue;
	  dist = distance(x, dim, i, jj);
	  nadded[0] = (int) 2*KB*dist/avgdist;

	  /* half the line segment near i */
	  h[0] = 0.5*(x[jj*dim] - x[i*dim])/nadded[0];
	  h[1] = 0.5*(x[jj*dim+1] - x[i*dim+1])/nadded[0];
	  point[0] = x[i*dim];
	  point[1] = x[i*dim+1];
	  for (k = 0; k < nadded[0] - 1; k++){
	    point[0] += h[0];
	    point[1] += h[1];
	    add_point(&N, grouping[i], &X, &nmax, point, &groups);
	  }	

	  /* half the line segment near jj */
	  h[0] = 0.5*(x[i*dim] - x[jj*dim])/nadded[0];
	  h[1] = 0.5*(x[i*dim+1] - x[jj*dim+1])/nadded[0];
	  point[0] = x[jj*dim];
	  point[1] = x[jj*dim+1];
	  for (k = 0; k < nadded[0] - 1; k++){
	    point[0] += h[0];
	    point[1] += h[1];
	    add_point(&N, grouping[jj], &X, &nmax, point, &groups);
	  }	
	}

      }/* done adding artifial points for edges */
    }

    res = make_map_internal(exclude_random, include_OK_points, N, dim, X, groups, graph, bounding_box_margin, nrandom, nedgep, 
			    shore_depth_tol, edge_bridge_tol, xcombined, nverts, x_poly, 
			    npolys, poly_lines, polys, polys_groups, poly_point_map, country_graph, highlight_cluster, flag);
    if (graph != graph0) SparseMatrix_delete(graph);
    FREE(groups); 
    FREE(X);
  }

  return res;

}
