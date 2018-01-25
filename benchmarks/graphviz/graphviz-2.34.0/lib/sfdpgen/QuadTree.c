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

#include "general.h"
#include "geom.h"
#include "arith.h"
#include "math.h"
#include "LinkedList.h"
#include "QuadTree.h"

extern real distance_cropped(real *x, int dim, int i, int j);

struct node_data_struct {
  real node_weight;
  real *coord;
  real id;
  void *data;
};

typedef struct node_data_struct *node_data;


static node_data node_data_new(int dim, real weight, real *coord, int id){
  node_data nd;
  int i;
  nd = MALLOC(sizeof(struct node_data_struct));
  nd->node_weight = weight;
  nd->coord = MALLOC(sizeof(real)*dim);
  nd->id = id;
  for (i = 0; i < dim; i++) nd->coord[i] = coord[i];
  nd->data = NULL;
  return nd;
}

void node_data_delete(void *d){
  node_data nd = (node_data) d;
  FREE(nd->coord);
  /*delete outside   if (nd->data) FREE(nd->data);*/
  FREE(nd);
}

real node_data_get_weight(void *d){
  node_data nd = (node_data) d;
  return nd->node_weight;
}

real* node_data_get_coord(void *d){
  node_data nd = (node_data) d;
  return nd->coord;
}

int node_data_get_id(void *d){
  node_data nd = (node_data) d;
  return nd->id;
}

#define node_data_get_data(d) (((node_data) (d))->data)


void check_or_realloc_arrays(int dim, int *nsuper, int *nsupermax, real **center, real **supernode_wgts, real **distances){
  
  if (*nsuper >= *nsupermax) {
    *nsupermax = *nsuper + MAX(10, (int) 0.2*(*nsuper));
    *center = REALLOC(*center, sizeof(real)*(*nsupermax)*dim);
    *supernode_wgts = REALLOC(*supernode_wgts, sizeof(real)*(*nsupermax));
    *distances = REALLOC(*distances, sizeof(real)*(*nsupermax));
  }
}

void QuadTree_get_supernodes_internal(QuadTree qt, real bh, real *point, int nodeid, int *nsuper, int *nsupermax, real **center, real **supernode_wgts, real **distances, real *counts, int *flag){
  SingleLinkedList l;
  real *coord, dist;
  int dim, i;

  (*counts)++;

  if (!qt) return;
  dim = qt->dim;
  l = qt->l;
  if (l){
    while (l){
      check_or_realloc_arrays(dim, nsuper, nsupermax, center, supernode_wgts, distances);
      if (node_data_get_id(SingleLinkedList_get_data(l)) != nodeid){
	coord = node_data_get_coord(SingleLinkedList_get_data(l));
	for (i = 0; i < dim; i++){
	  (*center)[dim*(*nsuper)+i] = coord[i];
	}
	(*supernode_wgts)[*nsuper] = node_data_get_weight(SingleLinkedList_get_data(l));
	(*distances)[*nsuper] = point_distance(point, coord, dim);
	(*nsuper)++;
      }
      l = SingleLinkedList_get_next(l);
    }
  }

  if (qt->qts){
    dist = point_distance(qt->center, point, dim); 
    if (qt->width < bh*dist){
      check_or_realloc_arrays(dim, nsuper, nsupermax, center, supernode_wgts, distances);
      for (i = 0; i < dim; i++){
        (*center)[dim*(*nsuper)+i] = qt->average[i];
      }
      (*supernode_wgts)[*nsuper] = qt->total_weight;
      (*distances)[*nsuper] = point_distance(qt->average, point, dim); 
      (*nsuper)++;
    } else {
      for (i = 0; i < 1<<dim; i++){
	QuadTree_get_supernodes_internal(qt->qts[i], bh, point, nodeid, nsuper, nsupermax, center, 
					 supernode_wgts, distances, counts, flag);
      }
    }
  }

}

void QuadTree_get_supernodes(QuadTree qt, real bh, real *point, int nodeid, int *nsuper, 
			     int *nsupermax, real **center, real **supernode_wgts, real **distances, double *counts, int *flag){
  int dim = qt->dim;

  (*counts) = 0;

  *nsuper = 0;

  *flag = 0;
  *nsupermax = 10;
  if (!*center) *center = MALLOC(sizeof(real)*(*nsupermax)*dim);
  if (!*supernode_wgts) *supernode_wgts = MALLOC(sizeof(real)*(*nsupermax));
  if (!*distances) *distances = MALLOC(sizeof(real)*(*nsupermax));
  QuadTree_get_supernodes_internal(qt, bh, point, nodeid, nsuper, nsupermax, center, supernode_wgts, distances, counts, flag);

}


static real *get_or_assign_node_force(real *force, int i, SingleLinkedList l, int dim){

  real *f = (real*) node_data_get_data(SingleLinkedList_get_data(l));

  if (!f){
    node_data_get_data(SingleLinkedList_get_data(l)) = &(force[i*dim]);
    f = (real*) node_data_get_data(SingleLinkedList_get_data(l));
  }
  return f;
}
static real *get_or_alloc_force_qt(QuadTree qt, int dim){
  int i;
  real *force = (real*) qt->data;
  if (!force){
    qt->data = MALLOC(sizeof(real)*dim);
    force = (real*) qt->data;
    for (i = 0; i < dim; i++) force[i] = 0.;
  }
  return force;
}

static void QuadTree_repulsive_force_interact(QuadTree qt1, QuadTree qt2, real *x, real *force, real bh, real p, real KP, real *counts){
  /* calculate the all to all reopulsive force and accumulate on each node of the quadtree if an interaction is possible.
     force[i*dim+j], j=1,...,dim is teh force on node i 
   */
  SingleLinkedList l1, l2;
  real *x1, *x2, dist, wgt1, wgt2, f, *f1, *f2, w1, w2;
  int dim, i, j, i1, i2, k;
  QuadTree qt11, qt12; 

  if (!qt1 || !qt2) return;
  assert(qt1->n > 0 && qt2->n > 0);
  dim = qt1->dim;

  l1 = qt1->l;
  l2 = qt2->l;

  /* far enough, calculate repulsive force */
  dist = point_distance(qt1->average, qt2->average, dim); 
  if (qt1->width + qt2->width < bh*dist){
    counts[0]++;
    x1 = qt1->average;
    w1 = qt1->total_weight;
    f1 = get_or_alloc_force_qt(qt1, dim);
    x2 = qt2->average;
    w2 = qt2->total_weight;
    f2 = get_or_alloc_force_qt(qt2, dim);
    assert(dist > 0);
    for (k = 0; k < dim; k++){
      if (p == -1){
	f = w1*w2*KP*(x1[k] - x2[k])/(dist*dist);
      } else {
	f = w1*w2*KP*(x1[k] - x2[k])/pow(dist, 1.- p);
      }
      f1[k] += f;
      f2[k] -= f;
    }
    return;
  }


  /* both at leaves, calculate repulsive force */
  if (l1 && l2){
    while (l1){
      x1 = node_data_get_coord(SingleLinkedList_get_data(l1));
      wgt1 = node_data_get_weight(SingleLinkedList_get_data(l1));
      i1 = node_data_get_id(SingleLinkedList_get_data(l1));
      f1 = get_or_assign_node_force(force, i1, l1, dim);
      l2 = qt2->l;
      while (l2){
	x2 = node_data_get_coord(SingleLinkedList_get_data(l2));
	wgt2 = node_data_get_weight(SingleLinkedList_get_data(l2));
	i2 = node_data_get_id(SingleLinkedList_get_data(l2));
	f2 = get_or_assign_node_force(force, i2, l2, dim);
	if ((qt1 == qt2 && i2 < i1) || i1 == i2) {
	  l2 = SingleLinkedList_get_next(l2);
	  continue;
	}
	counts[1]++;
	dist = distance_cropped(x, dim, i1, i2);
	for (k = 0; k < dim; k++){
	  if (p == -1){
	    f = wgt1*wgt2*KP*(x1[k] - x2[k])/(dist*dist);
	  } else {
	    f = wgt1*wgt2*KP*(x1[k] - x2[k])/pow(dist, 1.- p);
	  }
	  f1[k] += f;
	  f2[k] -= f;
	}
	l2 = SingleLinkedList_get_next(l2);
      }
      l1 = SingleLinkedList_get_next(l1);
    }
    return;
  }


  /* identical, split one */
  if (qt1 == qt2){
      for (i = 0; i < 1<<dim; i++){
	qt11 = qt1->qts[i];
	for (j = i; j < 1<<dim; j++){
	  qt12 = qt1->qts[j];
	  QuadTree_repulsive_force_interact(qt11, qt12, x, force, bh, p, KP, counts);
	}
      }
  } else {
    /* split the one with bigger box, or one not at the last level */
    if (qt1->width > qt2->width && !l1){
      for (i = 0; i < 1<<dim; i++){
	qt11 = qt1->qts[i];
	QuadTree_repulsive_force_interact(qt11, qt2, x, force, bh, p, KP, counts);
      }
    } else if (qt2->width > qt1->width && !l2){
      for (i = 0; i < 1<<dim; i++){
	qt11 = qt2->qts[i];
	QuadTree_repulsive_force_interact(qt11, qt1, x, force, bh, p, KP, counts);
      }
    } else if (!l1){/* pick one that is not at the last level */
      for (i = 0; i < 1<<dim; i++){
	qt11 = qt1->qts[i];
	QuadTree_repulsive_force_interact(qt11, qt2, x, force, bh, p, KP, counts);
      }
    } else if (!l2){
      for (i = 0; i < 1<<dim; i++){
	qt11 = qt2->qts[i];
	QuadTree_repulsive_force_interact(qt11, qt1, x, force, bh, p, KP, counts);
      }
    } else {
      assert(0); /* can be both at the leaf level since that should be catched at the beginning of this func. */
    }
  }
}

static void QuadTree_repulsive_force_accumulate(QuadTree qt, real *force, real *counts){
  /* push down forces on cells into the node level */
  real wgt, wgt2;
  real *f, *f2;
  SingleLinkedList l = qt->l;
  int i, k, dim;
  QuadTree qt2;

  dim = qt->dim;
  wgt = qt->total_weight;
  f = get_or_alloc_force_qt(qt, dim);
  assert(wgt > 0);
  counts[2]++;

  if (l){
    while (l){
      i = node_data_get_id(SingleLinkedList_get_data(l));
      f2 = get_or_assign_node_force(force, i, l, dim);
      wgt2 = node_data_get_weight(SingleLinkedList_get_data(l));
      wgt2 = wgt2/wgt;
      for (k = 0; k < dim; k++) f2[k] += wgt2*f[k];
      l = SingleLinkedList_get_next(l);
    }
    return;
  }

  for (i = 0; i < 1<<dim; i++){
    qt2 = qt->qts[i];
    if (!qt2) continue;
    assert(qt2->n > 0);
    f2 = get_or_alloc_force_qt(qt2, dim);
    wgt2 = qt2->total_weight;
    wgt2 = wgt2/wgt;
    for (k = 0; k < dim; k++) f2[k] += wgt2*f[k];
    QuadTree_repulsive_force_accumulate(qt2, force, counts);
  }

}

void QuadTree_get_repulsive_force(QuadTree qt, real *force, real *x, real bh, real p, real KP, real *counts, int *flag){
  /* get repulsice force by a more efficient algortihm: we consider two cells, if they are well separated, we
     calculate the overall repulsive force on the cell level, if not well separated, we divide one of the cell.
     If both cells are at the leaf level, we calcuaulate repulsicve force among individual nodes. Finally
     we accumulate forces at the cell levels to teh node level
     qt: the quadtree
     x: current coordinates for node i is x[i*dim+j], j = 0, ..., dim-1
     force: the repulsice force, an array of length dim*nnodes, the force for node i is at force[i*dim+j], j = 0, ..., dim - 1
     bh: Barnes-Hut coefficient. If width_cell1+width_cell2 < bh*dist_between_cells, we treat each cell as a supernode.
     p: the repulsive force power
     KP: pow(K, 1 - p)
     counts: array of size 4. 
     .  counts[0]: number of cell-cell interaction
     .  counts[1]: number of cell-node interaction
     .  counts[2]: number of total cells in the quadtree
     . Al normalized by dividing by number of nodes
  */
  int n = qt->n, dim = qt->dim, i;

  for (i = 0; i < 4; i++) counts[i] = 0;

  *flag = 0;

  for (i = 0; i < dim*n; i++) force[i] = 0;

  QuadTree_repulsive_force_interact(qt, qt, x, force, bh, p, KP, counts);
  QuadTree_repulsive_force_accumulate(qt, force, counts);
  for (i = 0; i < 4; i++) counts[i] /= n;

}
QuadTree QuadTree_new_from_point_list(int dim, int n, int max_level, real *coord, real *weight){
  /* form a new QuadTree data structure from a list of coordinates of n points
     coord: of length n*dim, point i sits at [i*dim, i*dim+dim - 1]
     weight: node weight of lentgth n. If NULL, unit weight assumed.
   */
  real *xmin, *xmax, *center, width;
  QuadTree qt = NULL;
  int i, k;

  xmin = MALLOC(sizeof(real)*dim);
  xmax = MALLOC(sizeof(real)*dim);
  center = MALLOC(sizeof(real)*dim);
  if (!xmin || !xmax || !center) return NULL;

  for (i = 0; i < dim; i++) xmin[i] = coord[i];
  for (i = 0; i < dim; i++) xmax[i] = coord[i];

  for (i = 1; i < n; i++){
    for (k = 0; k < dim; k++){
      xmin[k] = MIN(xmin[k], coord[i*dim+k]);
      xmax[k] = MAX(xmax[k], coord[i*dim+k]);
    }
  }

  width = xmax[0] - xmin[0];
  for (i = 0; i < dim; i++) {
    center[i] = (xmin[i] + xmax[i])*0.5;
    width = MAX(width, xmax[i] - xmin[i]);
  }
  width *= 0.52;
  qt = QuadTree_new(dim, center, width, max_level);

  if (weight){
    for (i = 0; i < n; i++){
      qt = QuadTree_add(qt, &(coord[i*dim]), weight[i], i);
    }
  } else {
    for (i = 0; i < n; i++){
      qt = QuadTree_add(qt, &(coord[i*dim]), 1, i);
    }
  }


  FREE(xmin);
  FREE(xmax);
  FREE(center);
  return qt;
}

QuadTree QuadTree_new(int dim, real *center, real width, int max_level){
  QuadTree q;
  int i;
  q = MALLOC(sizeof(struct QuadTree_struct));
  q->dim = dim;
  q->n = 0;
  q->center = MALLOC(sizeof(real)*dim);
  for (i = 0; i < dim; i++) q->center[i] = center[i];
  assert(width > 0);
  q->width = width;
  q->total_weight = 0;
  q->average = NULL;
  q->qts = NULL;
  q->l = NULL;
  q->max_level = max_level;
  q->data = NULL;
  return q;
}

void QuadTree_delete(QuadTree q){
  int i, dim;
  if (!q) return;
  dim = q->dim;
  FREE(q->center);
  FREE(q->average);
  if (q->data) FREE(q->data);
  if (q->qts){
    for (i = 0; i < 1<<dim; i++){
      QuadTree_delete(q->qts[i]);
    }
    FREE(q->qts);
  }
  SingleLinkedList_delete(q->l, node_data_delete);
  FREE(q);
}

static int QuadTree_get_quadrant(int dim, real *center, real *coord){
  /* find the quadrant that a point of coordinates coord is going into with reference to the center.
     if coord - center == {+,-,+,+} = {1,0,1,1}, then it will sit in the i-quadrant where
     i's binary representation is 1011 (that is, decimal 11).
   */
  int d = 0, i;

  for (i = dim - 1; i >= 0; i--){
    if (coord[i] - center[i] < 0){
      d = 2*d;
    } else {
      d = 2*d+1;
    }
  }
  return d;
}

static QuadTree QuadTree_new_in_quadrant(int dim, real *center, real width, int max_level, int i){
  /* a new quadtree in quadrant i of the original cell. The original cell is centered at 'center".
     The new cell have width "width".
   */
  QuadTree qt;
  int k;

  qt = QuadTree_new(dim, center, width, max_level);
  center = qt->center;/* right now this has the center for the parent */
  for (k = 0; k < dim; k++){/* decompose child id into binary, if {1,0}, say, then
				     add {width/2, -width/2} to the parents' center
				     to get the child's center. */
    if (i%2 == 0){
      center[k]  -= width;
    } else {
      center[k] += width;
    }
    i = (i - i%2)/2;
  }
  return qt;

}
static QuadTree QuadTree_add_internal(QuadTree q, real *coord, real weight, int id, int level){
  int i, dim = q->dim, ii;
  node_data nd = NULL;

  int max_level = q->max_level;
  int idd;

  /* Make sure that coord is within bounding box */
  for (i = 0; i < q->dim; i++) {
    if (coord[i] < q->center[i] - q->width - 1.e5*MACHINEACC*q->width || coord[i] > q->center[i] + q->width + 1.e5*MACHINEACC*q->width) {
#ifdef DEBUG_PRINT
      fprintf(stderr,"coordinate %f is outside of the box:{%f, %f}, \n(q->center[i] - q->width) - coord[i] =%g, coord[i]-(q->center[i] + q->width) = %g\n",coord[i], (q->center[i] - q->width), (q->center[i] + q->width),
	      (q->center[i] - q->width) - coord[i],  coord[i]-(q->center[i] + q->width));
#endif
      //return NULL;
    }
  }

  if (q->n == 0){
    /* if this node is empty right now */
    q->n = 1;
    q->total_weight = weight;
    q->average = MALLOC(sizeof(real)*dim);
    for (i = 0; i < q->dim; i++) q->average[i] = coord[i];
    nd = node_data_new(q->dim, weight, coord, id);
    assert(!(q->l));
    q->l = SingleLinkedList_new(nd);
  } else if (level < max_level){
    /* otherwise open up into 2^dim quadtrees unless the level is too high */
    q->total_weight += weight;
    for (i = 0; i < q->dim; i++) q->average[i] = ((q->average[i])*q->n + coord[i])/(q->n + 1);
    if (!q->qts){
      q->qts = MALLOC(sizeof(QuadTree)*(1<<dim));
      for (i = 0; i < 1<<dim; i++) q->qts[i] = NULL;
    }/* done adding new quadtree, now add points to them */
    
    /* insert the old node (if exist) and the current node into the appropriate child quadtree */
    ii = QuadTree_get_quadrant(dim, q->center, coord);
    assert(ii < 1<<dim && ii >= 0);
    if (q->qts[ii] == NULL) q->qts[ii] = QuadTree_new_in_quadrant(q->dim, q->center, (q->width)/2, max_level, ii);
    
    q->qts[ii] = QuadTree_add_internal(q->qts[ii], coord, weight, id, level + 1);
    assert(q->qts[ii]);

    if (q->l){
      idd = node_data_get_id(SingleLinkedList_get_data(q->l));
      assert(q->n == 1);
      coord = node_data_get_coord(SingleLinkedList_get_data(q->l));
      weight = node_data_get_weight(SingleLinkedList_get_data(q->l));
      ii = QuadTree_get_quadrant(dim, q->center, coord);
      assert(ii < 1<<dim && ii >= 0);

      if (q->qts[ii] == NULL) q->qts[ii] = QuadTree_new_in_quadrant(q->dim, q->center, (q->width)/2, max_level, ii);

      q->qts[ii] = QuadTree_add_internal(q->qts[ii], coord, weight, idd, level + 1);
      assert(q->qts[ii]);
      
      /* delete the old node data on parent */
      SingleLinkedList_delete(q->l, node_data_delete);
      q->l = NULL;
    }
    
    (q->n)++;
  } else {
    assert(!(q->qts));
    /* level is too high, append data in the linked list */
    (q->n)++;
    q->total_weight += weight;
    for (i = 0; i < q->dim; i++) q->average[i] = ((q->average[i])*q->n + coord[i])/(q->n + 1);
    nd = node_data_new(q->dim, weight, coord, id);
    assert(q->l);
    q->l = SingleLinkedList_prepend(q->l, nd);
  }
  return q;
}


QuadTree QuadTree_add(QuadTree q, real *coord, real weight, int id){
  if (!q) return q;
  return QuadTree_add_internal(q, coord, weight, id, 0);
  
}

static void draw_polygon(FILE *fp, int dim, real *center, real width){
  /* pliot the enclosing square */
  if (dim < 2 || dim > 3) return;
  fprintf(fp,"(*in c*){Line[{");

  if (dim == 2){
    fprintf(fp, "{%f, %f}", center[0] + width, center[1] + width);
    fprintf(fp, ",{%f, %f}", center[0] - width, center[1] + width);
    fprintf(fp, ",{%f, %f}", center[0] - width, center[1] - width);
    fprintf(fp, ",{%f, %f}", center[0] + width, center[1] - width);
    fprintf(fp, ",{%f, %f}", center[0] + width, center[1] + width);
  } else if (dim == 3){
    /* top */
    fprintf(fp,"{");
    fprintf(fp, "{%f, %f, %f}", center[0] + width, center[1] + width, center[2] + width);
    fprintf(fp, ",{%f, %f, %f}", center[0] - width, center[1] + width, center[2] + width);
    fprintf(fp, ",{%f, %f, %f}", center[0] - width, center[1] - width, center[2] + width);
    fprintf(fp, ",{%f, %f, %f}", center[0] + width, center[1] - width, center[2] + width);
    fprintf(fp, ",{%f, %f, %f}", center[0] + width, center[1] + width, center[2] + width);
    fprintf(fp,"},");
    /* bot */
    fprintf(fp,"{");
    fprintf(fp, "{%f, %f, %f}", center[0] + width, center[1] + width, center[2] - width);
    fprintf(fp, ",{%f, %f, %f}", center[0] - width, center[1] + width, center[2] - width);
    fprintf(fp, ",{%f, %f, %f}", center[0] - width, center[1] - width, center[2] - width);
    fprintf(fp, ",{%f, %f, %f}", center[0] + width, center[1] - width, center[2] - width);
    fprintf(fp, ",{%f, %f, %f}", center[0] + width, center[1] + width, center[2] - width);
    fprintf(fp,"},");
    /* for sides */
    fprintf(fp,"{");
    fprintf(fp, "{%f, %f, %f}", center[0] + width, center[1] + width, center[2] - width);
    fprintf(fp, ",{%f, %f, %f}", center[0] + width, center[1] + width, center[2] + width);
    fprintf(fp,"},");
    
    fprintf(fp,"{");
    fprintf(fp, "{%f, %f, %f}", center[0] - width, center[1] + width, center[2] - width);
    fprintf(fp, ",{%f, %f, %f}", center[0] - width, center[1] + width, center[2] + width);
    fprintf(fp,"},");
    
    fprintf(fp,"{");
    fprintf(fp, "{%f, %f, %f}", center[0] + width, center[1] - width, center[2] - width);
    fprintf(fp, ",{%f, %f, %f}", center[0] + width, center[1] - width, center[2] + width);
    fprintf(fp,"},");
    
    fprintf(fp,"{");
    fprintf(fp, "{%f, %f, %f}", center[0] - width, center[1] - width, center[2] - width);
    fprintf(fp, ",{%f, %f, %f}", center[0] - width, center[1] - width, center[2] + width);
    fprintf(fp,"}");
  }


  fprintf(fp, "}]}(*end C*)");


}
static void QuadTree_print_internal(FILE *fp, QuadTree q, int level){
  /* dump a quad tree in Mathematica format. */
  SingleLinkedList l, l0;
  real *coord;
  int i, dim;

  if (!q) return;

  draw_polygon(fp, q->dim, q->center, q->width);
  dim = q->dim;
  
  l0 = l = q->l;
  if (l){
    printf(",(*a*) {Red,");
    while (l){
      if (l != l0) printf(",");
      coord = node_data_get_coord(SingleLinkedList_get_data(l));
      fprintf(fp, "(*node %d*) Point[{",  node_data_get_id(SingleLinkedList_get_data(l)));
      for (i = 0; i < dim; i++){
	if (i != 0) printf(",");
	fprintf(fp, "%f",coord[i]);
      }
      fprintf(fp, "}]");
      l = SingleLinkedList_get_next(l);
    }
    fprintf(fp, "}");
  }

  if (q->qts){
    for (i = 0; i < 1<<dim; i++){
      fprintf(fp, ",(*b*){");
      QuadTree_print_internal(fp, q->qts[i], level + 1); 
      fprintf(fp, "}");
    }
  }


}

void QuadTree_print(FILE *fp, QuadTree q){
  if (!fp) return;
  if (q->dim == 2){
    fprintf(fp, "Graphics[{");
  } else if (q->dim == 3){
    fprintf(fp, "Graphics3D[{");
  } else {
    return;
  }
  QuadTree_print_internal(fp, q, 0);
  if (q->dim == 2){
    fprintf(fp, "}, PlotRange -> All, Frame -> True, FrameTicks -> True]\n");
  } else {
    fprintf(fp, "}, PlotRange -> All]\n");
  }
}




static void QuadTree_get_nearest_internal(QuadTree qt, real *x, real *y, real *min, int *imin, int tentative, int *flag){
  /* get the narest point years to {x[0], ..., x[dim]} and store in y.*/
  SingleLinkedList l;
  real *coord, dist;
  int dim, i, iq = -1;
  real qmin;
  real *point = x;

  *flag = 0;
  if (!qt) return;
  dim = qt->dim;
  l = qt->l;
  if (l){
    while (l){
      coord = node_data_get_coord(SingleLinkedList_get_data(l));
      dist = point_distance(point, coord, dim);
      if(*min < 0 || dist < *min) {
	*min = dist;
	*imin = node_data_get_id(SingleLinkedList_get_data(l));
	for (i = 0; i < dim; i++) y[i] = coord[i];
      }
      l = SingleLinkedList_get_next(l);
    }
  }
  
  if (qt->qts){
    dist = point_distance(qt->center, point, dim); 
    if (*min >= 0 && (dist - sqrt((real) dim) * qt->width > *min)){
      return;
    } else {
      if (tentative){/* quick first approximation*/
	qmin = -1;
	for (i = 0; i < 1<<dim; i++){
	  if (qt->qts[i]){
	    dist = point_distance(qt->qts[i]->average, point, dim); 
	    if (dist < qmin || qmin < 0){
	      qmin = dist; iq = i;
	    }
	  }
	}
	assert(iq >= 0);
	QuadTree_get_nearest_internal(qt->qts[iq], x, y, min, imin, tentative, flag);
      } else {
	for (i = 0; i < 1<<dim; i++){
	  QuadTree_get_nearest_internal(qt->qts[i], x, y, min, imin, tentative, flag);
	}
      }
    }
  }

}


void QuadTree_get_nearest(QuadTree qt, real *x, real *ymin, int *imin, real *min, int *flag){

  *flag = 0;
  *min = -1;

  QuadTree_get_nearest_internal(qt, x, ymin, min, imin, TRUE, flag);

  QuadTree_get_nearest_internal(qt, x, ymin, min, imin, FALSE, flag);


}
