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
#include "topfisheyeview.h"

//#include "glcomptext.h"
//#include "glcomptextpng.h"
#include "math.h"
#include "memory.h"
#include "viewport.h"
#include "viewportcamera.h"
#include "draw.h"
#include "smyrna_utils.h"

#include "assert.h"
#include "hier.h"
#include "topfisheyeview.h"
#include <string.h>
#include "color.h"
#include "colorprocs.h"

#ifdef MSWIN32
#include <wincrypt.h>
#endif

static int get_temp_coords(topview * t, int level, int v, double *coord_x,
			   double *coord_y);
#ifdef UNUSED
static int get_temp_coords2(topview * t, int level, int v, double *coord_x,
			    double *coord_y, float *R, float *G, float *B);
#endif

static void color_interpolation(glCompColor srcColor, glCompColor tarColor,
				glCompColor * color, int levelcount,
				int level)
{
    if (levelcount <= 0)
	return;


    color->R =
	((float) level * tarColor.R - (float) level * srcColor.R +
	 (float) levelcount * srcColor.R) / (float) levelcount;
    color->G =
	((float) level * tarColor.G - (float) level * srcColor.G +
	 (float) levelcount * srcColor.G) / (float) levelcount;
    color->B =
	((float) level * tarColor.B - (float) level * srcColor.B +
	 (float) levelcount * srcColor.B) / (float) levelcount;
}

#ifdef UNUSED
static double dist(double x1, double y1, double x2, double y2)
{
    return sqrt((x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2));
}
static double dist3d(double x1, double y1, double z1, double x2, double y2,
		     double z2)
{
    return sqrt((x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2) +
		(z1 - z2) * (z1 - z2));
}
#endif

#ifdef UNUSED
static void drawtopologicalfisheyestatic(topview * t);
#endif

#ifdef UNUSED

The code below now longer works. First, t->Nodes == NULL, since
we now rely on the libgraph graph for data. Second, the code that
drew the distorted view was left out of the move from topview.c
to topviewfuncs.c. The magnifiers were cute but not particularly
useful, so we leave the feature out for now.

static double G(double x)
{
    // distortion function for fisheye display
    return (view->fmg.fisheye_distortion_fac +
	    1) * x / (view->fmg.fisheye_distortion_fac * x + 1);
}


void fisheye_polar(double x_focus, double y_focus, topview * t)
{
    int i;
    double distance, distorted_distance, ratio, range;

    range = 0;
    for (i = 1; i < t->Nodecount; i++) {
	if (point_within_ellips_with_coords
	    ((float) x_focus, (float) y_focus, (float) view->fmg.R,
	     (float) view->fmg.R, t->Nodes[i].x, t->Nodes[i].y)) {
	    range = MAX(range,
			dist(t->Nodes[i].x, t->Nodes[i].y, x_focus,
			     y_focus));
	}
    }

    for (i = 1; i < t->Nodecount; i++) {

	if (point_within_ellips_with_coords
	    ((float) x_focus, (float) y_focus, (float) view->fmg.R,
	     (float) view->fmg.R, t->Nodes[i].x, t->Nodes[i].y)) {
	    distance =
		dist(t->Nodes[i].x, t->Nodes[i].y, x_focus, y_focus);
	    distorted_distance = G(distance / range) * range;
	    if (distance != 0) {
		ratio = distorted_distance / distance;
	    } else {
		ratio = 0;
	    }
	    t->Nodes[i].distorted_x =
		(float) x_focus + (t->Nodes[i].x -
				   (float) x_focus) * (float) ratio;
	    t->Nodes[i].distorted_y =
		(float) y_focus + (t->Nodes[i].y -
				   (float) y_focus) * (float) ratio;
	    t->Nodes[i].zoom_factor =
		(float) 1 *(float) distorted_distance / (float) distance;
	} else {
	    t->Nodes[i].distorted_x = t->Nodes[i].x;
	    t->Nodes[i].distorted_y = t->Nodes[i].y;
	    t->Nodes[i].zoom_factor = 1;
	}
    }
}
void fisheye_spherical(double x_focus, double y_focus, double z_focus,
		       topview * t)
{
    int i;
    double distance, distorted_distance, ratio, range;

    range = 0;
    for (i = 1; i < t->Nodecount; i++) {
	if (point_within_sphere_with_coords
	    ((float) x_focus, (float) y_focus, (float) z_focus,
	     (float) view->fmg.R, t->Nodes[i].x, t->Nodes[i].y,
	     t->Nodes[i].z)) {


	    range =
		MAX(range,
		    dist3d(t->Nodes[i].x, t->Nodes[i].y, t->Nodes[i].z,
			   x_focus, y_focus, z_focus));
	}
    }

    for (i = 1; i < t->Nodecount; i++) {


	if (point_within_sphere_with_coords
	    ((float) x_focus, (float) y_focus, (float) z_focus,
	     (float) view->fmg.R, t->Nodes[i].x, t->Nodes[i].y,
	     t->Nodes[i].z)) {
	    distance =
		dist3d(t->Nodes[i].x, t->Nodes[i].y, t->Nodes[i].z,
		       x_focus, y_focus, z_focus);
	    distorted_distance = G(distance / range) * range;
	    if (distance != 0) {
		ratio = distorted_distance / distance;
	    } else {
		ratio = 0;
	    }
	    t->Nodes[i].distorted_x =
		(float) x_focus + (t->Nodes[i].x -
				   (float) x_focus) * (float) ratio;
	    t->Nodes[i].distorted_y =
		(float) y_focus + (t->Nodes[i].y -
				   (float) y_focus) * (float) ratio;
	    t->Nodes[i].distorted_z =
		(float) z_focus + (t->Nodes[i].z -
				   (float) z_focus) * (float) ratio;
	    t->Nodes[i].zoom_factor =
		(float) 1 *(float) distorted_distance / (float) distance;
	} else {
	    t->Nodes[i].distorted_x = t->Nodes[i].x;
	    t->Nodes[i].distorted_y = t->Nodes[i].y;
	    t->Nodes[i].distorted_z = t->Nodes[i].z;
	    t->Nodes[i].zoom_factor = 1;
	}
    }
}
#endif

static v_data *makeGraph(Agraph_t* gg, int *nedges)
{
    int i;
    int ne = agnedges(gg);
    int nv = agnnodes(gg);
    v_data *graph = N_NEW(nv, v_data);
    int *edges = N_NEW(2 * ne + nv, int);	/* reserve space for self loops */
    float *ewgts = N_NEW(2 * ne + nv, float);
    Agnode_t *np;
    Agedge_t *ep;
    Agraph_t *g = NULL;
    int i_nedges;
    ne = 0;
    i=0;
//    for (i = 0; i < nv; i++) {
    for (np = agfstnode(gg); np; np = agnxtnode(gg, np)) 
    {
	graph[i].edges = edges++;	/* reserve space for the self loop */
	graph[i].ewgts = ewgts++;
#ifdef STYLES
	graph[i].styles = NULL;
#endif
	i_nedges = 1;		/* one for the self */

	if (!g)
	    g = agraphof(np);
	for (ep = agfstedge(g, np); ep; ep = agnxtedge(g, ep, np)) 
	{
	    Agnode_t *vp;
	    Agnode_t *tp = agtail(ep);
	    Agnode_t *hp = aghead(ep);
	    assert(hp != tp);
	    /* FIX: handle multiedges */
	    vp = (tp == np ? hp : tp);
	    ne++;
	    i_nedges++;
//	    *edges++ = ((temp_node_record *) AGDATA(vp))->TVref;
	    *edges++ = ND_TVref(vp);
	    *ewgts++ = 1;

	}

	graph[i].nedges = i_nedges;
	graph[i].edges[0] = i;
	graph[i].ewgts[0] = 1 - i_nedges;
	i++;
    }
    ne /= 2;			/* each edge counted twice */
    *nedges = ne;
    return graph;
}




#if 0
static v_data *makeGraph_old(topview * tv, int *nedges)
{
    int i;
    int ne = tv->Edgecount;	/* upper bound */
    int nv = tv->Nodecount;
    v_data *graph = N_NEW(nv, v_data);
    int *edges = N_NEW(2 * ne + nv, int);	/* reserve space for self loops */
    float *ewgts = N_NEW(2 * ne + nv, float);
    Agnode_t *np;
    Agedge_t *ep;
    Agraph_t *g = NULL;
    int i_nedges;

    ne = 0;
    for (i = 0; i < nv; i++) {
	graph[i].edges = edges++;	/* reserve space for the self loop */
	graph[i].ewgts = ewgts++;
#ifdef STYLES
	graph[i].styles = NULL;
#endif
	i_nedges = 1;		/* one for the self */

	np = tv->Nodes[i].Node;
	if (!g)
	    g = agraphof(np);
	for (ep = agfstedge(g, np); ep; ep = agnxtedge(g, ep, np)) 
	{
	    Agnode_t *vp;
	    Agnode_t *tp = agtail(ep);
	    Agnode_t *hp = aghead(ep);
	    assert(hp != tp);
	    /* FIX: handle multiedges */
	    vp = (tp == np ? hp : tp);
	    ne++;
	    i_nedges++;
	    *edges++ = ((temp_node_record *) AGDATA(vp))->TVref;
	    *ewgts++ = 1;
	}

	graph[i].nedges = i_nedges;
	graph[i].edges[0] = i;
	graph[i].ewgts[0] = 1 - i_nedges;
    }
    ne /= 2;			/* each edge counted twice */
    *nedges = ne;
    return graph;
}
#endif

static void refresh_old_values(topview * t)
{
    int level, v;
    Hierarchy *hp = t->fisheyeParams.h;
    for (level = 0; level < hp->nlevels; level++) {
	for (v = 0; v < hp->nvtxs[level]; v++) {
	    ex_vtx_data *gg = hp->geom_graphs[level];
	    /* v_data *g = hp->graphs[level]; */
	    /* double x0,y0; */
	    gg[v].old_physical_x_coord = gg[v].physical_x_coord;
	    gg[v].old_physical_y_coord = gg[v].physical_y_coord;
	    gg[v].old_active_level = gg[v].active_level;
	}
    }

}

/* To use:
 * double* x_coords; // initial x coordinates
 * double* y_coords; // initial y coordinates
 * focus_t* fs;
 * int ne;
 * v_data* graph = makeGraph (topview*, &ne);
 * hierarchy = makeHier(topview->NodeCount, ne, graph, x_coords, y_coords);
 * freeGraph (graph);
 * fs = initFocus (topview->Nodecount); // create focus set
 */
void prepare_topological_fisheye(Agraph_t* g,topview * t)
{
    double *x_coords = N_NEW(t->Nodecount, double);	// initial x coordinates
    double *y_coords = N_NEW(t->Nodecount, double);	// initial y coordinates
    focus_t *fs;
    int ne;
    int i;
    int closest_fine_node;
    int cur_level = 0;
    Hierarchy *hp;
    ex_vtx_data *gg;
    gvcolor_t cl;
    Agnode_t *np;

    v_data *graph = makeGraph(g, &ne);

//      t->fisheyeParams.animate=1;   //turn the animation on
    i=0;
    for (np = agfstnode(g); np; np = agnxtnode(g, np)) 
    {
	x_coords[i]=ND_A(np).x;
	y_coords[i]=ND_A(np).y;
	i++;
    }
    hp = t->fisheyeParams.h =
	makeHier(agnnodes(g), ne, graph, x_coords, y_coords,
		 &(t->fisheyeParams.hier));
    freeGraph(graph);
    free(x_coords);
    free(y_coords);

    fs = t->fisheyeParams.fs = initFocus(agnnodes(g));	// create focus set
    gg = hp->geom_graphs[0];

    closest_fine_node = 0;	/* first node */
    fs->num_foci = 1;
    fs->foci_nodes[0] = closest_fine_node;
    fs->x_foci[0] = hp->geom_graphs[cur_level][closest_fine_node].x_coord;
    fs->y_foci[0] = hp->geom_graphs[cur_level][closest_fine_node].y_coord;

    view->Topview->fisheyeParams.repos.width =
	(int) (view->bdxRight - view->bdxLeft);
    view->Topview->fisheyeParams.repos.height =
	(int) (view->bdyTop - view->bdyBottom);
    view->Topview->fisheyeParams.repos.rescale = Polar;

    //topological fisheye 

    colorxlate(get_attribute_value
	       ("topologicalfisheyefinestcolor", view,
		view->g[view->activeGraph]), &cl, RGBA_DOUBLE);
    view->Topview->fisheyeParams.srcColor.R = (float) cl.u.RGBA[0];
    view->Topview->fisheyeParams.srcColor.G = (float) cl.u.RGBA[1];
    view->Topview->fisheyeParams.srcColor.B = (float) cl.u.RGBA[2];
    colorxlate(get_attribute_value
	       ("topologicalfisheyecoarsestcolor", view,
		view->g[view->activeGraph]), &cl, RGBA_DOUBLE);
    view->Topview->fisheyeParams.tarColor.R = (float) cl.u.RGBA[0];
    view->Topview->fisheyeParams.tarColor.G = (float) cl.u.RGBA[1];
    view->Topview->fisheyeParams.tarColor.B = (float) cl.u.RGBA[2];


    sscanf(agget
	   (view->g[view->activeGraph],
	    "topologicalfisheyedistortionfactor"), "%lf",
	   &view->Topview->fisheyeParams.repos.distortion);
    sscanf(agget
	   (view->g[view->activeGraph], "topologicalfisheyefinenodes"),
	   "%d", &view->Topview->fisheyeParams.level.num_fine_nodes);
    sscanf(agget
	   (view->g[view->activeGraph],
	    "topologicalfisheyecoarseningfactor"), "%lf",
	   &view->Topview->fisheyeParams.level.coarsening_rate);
    sscanf(agget
	   (view->g[view->activeGraph], "topologicalfisheyedist2limit"),
	   "%d", &view->Topview->fisheyeParams.hier.dist2_limit);
    sscanf(agget(view->g[view->activeGraph], "topologicalfisheyeanimate"),
	   "%d", &view->Topview->fisheyeParams.animate);

    set_active_levels(hp, fs->foci_nodes, fs->num_foci, &(t->fisheyeParams.level));
    positionAllItems(hp, fs, &(t->fisheyeParams.repos));
    refresh_old_values(t);

/* fprintf (stderr, "No. of active nodes = %d\n", count_active_nodes(hp)); */

}

#if 0
/*
	draws all level 0 nodes and edges, during animation
*/
void printalllevels(topview * t)
{
    int level, v;
    Hierarchy *hp = t->fisheyeParams.h;
    glPointSize(5);
    glBegin(GL_POINTS);
    for (level = 0; level < hp->nlevels; level++) {
	for (v = 0; v < hp->nvtxs[level]; v++) {
	    ex_vtx_data *gg = hp->geom_graphs[level];
	    if (gg[v].active_level == level) {
		double x0, y0;
		get_temp_coords(t, level, v, &x0, &y0);
		glColor3f((GLfloat) (hp->nlevels - level) /
			  (GLfloat) hp->nlevels,
			  (GLfloat) level / (GLfloat) hp->nlevels, 0);
		glVertex3f((GLfloat) x0, (GLfloat) y0, (GLfloat) 0);
	    }
	}
    }
    glEnd();
}
#endif

static void drawtopfishnodes(topview * t)
{
    glCompColor srcColor;
    glCompColor tarColor;
    glCompColor color;
    int level, v;
    Hierarchy *hp = t->fisheyeParams.h;
    static int max_visible_level = 0;
    srcColor.R = view->Topview->fisheyeParams.srcColor.R;
    srcColor.G = view->Topview->fisheyeParams.srcColor.G;
    srcColor.B = view->Topview->fisheyeParams.srcColor.B;
    tarColor.R = view->Topview->fisheyeParams.tarColor.R;
    tarColor.G = view->Topview->fisheyeParams.tarColor.G;
    tarColor.B = view->Topview->fisheyeParams.tarColor.B;


    //draw focused node little bigger than others
/*		ex_vtx_data *gg = hp->geom_graphs[0];
		if ((gg[v].active_level == 0) &&(v==t->fisheyeParams.fs->foci_nodes[0]))*/


    //drawing nodes
    glPointSize(7);
    level = 0;
    glBegin(GL_POINTS);
    for (level = 0; level < hp->nlevels; level++) {
	for (v = 0; v < hp->nvtxs[level]; v++) {
	    double x0, y0;
	    if (get_temp_coords(t, level, v, &x0, &y0)) {

		if (!(((-x0 / view->zoom > view->clipX1)
		       && (-x0 / view->zoom < view->clipX2)
		       && (-y0 / view->zoom > view->clipY1)
		       && (-y0 / view->zoom < view->clipY2))))
		    continue;

//                              if (level !=0)
//                                      glColor4f((GLfloat) (hp->nlevels - level)*(GLfloat)0.5 /  (GLfloat) hp->nlevels,
//                                (GLfloat) level / (GLfloat) hp->nlevels, (GLfloat)0,(GLfloat)view->defaultnodealpha);
		//                      else
//                              glColor4f((GLfloat) 1,
//                                (GLfloat) level / (GLfloat) hp->nlevels*2, 0,view->defaultnodealpha);

		if (max_visible_level < level)
		    max_visible_level = level;
		color_interpolation(srcColor, tarColor, &color,
				    max_visible_level, level);
		glColor4f(color.R, color.G, color.B,
			  (GLfloat) view->defaultnodealpha);

/*								glColor3f((GLfloat) (hp->nlevels - level)*0.5 /  (GLfloat) hp->nlevels,
				  (GLfloat) level / (GLfloat) hp->nlevels, 0);*/

		glVertex3f((GLfloat) x0, (GLfloat) y0, (GLfloat) 0);
	    }
	}
    }
    glEnd();

}

#if 0
static void drawtopfishnodelabels(topview * t)
{
    int v, finenodes, focusnodes;
    char buf[512];
    char *str;
    Hierarchy *hp = t->fisheyeParams.h;
    finenodes = focusnodes = 0;
    str =
	agget(view->g[view->activeGraph],
	      "topologicalfisheyelabelfinenodes");
    if (strcmp(str, "1") == 0) {
	finenodes = 1;
    }
    str =
	agget(view->g[view->activeGraph], "topologicalfisheyelabelfocus");
    if (strcmp(str, "1") == 0) {
	focusnodes = 1;
    }
    if ((finenodes) || (focusnodes)) {
	for (v = 0; v < hp->nvtxs[0]; v++) {
	    ex_vtx_data *gg = hp->geom_graphs[0];
	    if (gg[v].active_level == 0) {
		if (view->Topview->Nodes[v].Label)
		    strcpy(buf, view->Topview->Nodes[v].Label);
		else
		    sprintf(buf, "%d", v);

		if ((v == t->fisheyeParams.fs->foci_nodes[0]) && (focusnodes)) {
		    glColor4f((float) 0, (float) 0, (float) 1, (float) 1);
		    glprintfglut(GLUT_BITMAP_HELVETICA_18,
				 gg[v].physical_x_coord,
				 gg[v].physical_y_coord, 0, buf);
		} else if (finenodes) {
		    glColor4f(0, 0, 0, 1);
		    glprintfglut(GLUT_BITMAP_HELVETICA_10,
				 gg[v].physical_x_coord,
				 gg[v].physical_y_coord, 0, buf);
		}
	    }

	}
    }

}
#endif
static void drawtopfishedges(topview * t)
{
    glCompColor srcColor;
    glCompColor tarColor;
    glCompColor color;

    int level, v, i, n;
    Hierarchy *hp = t->fisheyeParams.h;
    static int max_visible_level = 0;
    srcColor.R = view->Topview->fisheyeParams.srcColor.R;
    srcColor.G = view->Topview->fisheyeParams.srcColor.G;
    srcColor.B = view->Topview->fisheyeParams.srcColor.B;
    tarColor.R = view->Topview->fisheyeParams.tarColor.R;
    tarColor.G = view->Topview->fisheyeParams.tarColor.G;
    tarColor.B = view->Topview->fisheyeParams.tarColor.B;

    //and edges
    glBegin(GL_LINES);
    for (level = 0; level < hp->nlevels; level++) {
	for (v = 0; v < hp->nvtxs[level]; v++) {
	    v_data *g = hp->graphs[level];
	    double x0, y0;
	    if (get_temp_coords(t, level, v, &x0, &y0)) {
		for (i = 1; i < g[v].nedges; i++) {
		    double x, y;
		    n = g[v].edges[i];


		    if (max_visible_level < level)
			max_visible_level = level;
		    color_interpolation(srcColor, tarColor, &color,
					max_visible_level, level);
		    glColor4f(color.R, color.G, color.B,
			      (GLfloat) view->defaultnodealpha);


		    if (get_temp_coords(t, level, n, &x, &y)) {
			glVertex3f((GLfloat) x0, (GLfloat) y0,
				   (GLfloat) 0);
			glVertex3f((GLfloat) x, (GLfloat) y, (GLfloat) 0);
		    } else	// if (gg[n].active_level > level) 
		    {
			int levell, nodee;
			find_active_ancestor_info(hp, level, n, &levell,
						  &nodee);
//                                              find_physical_coords(hp, level, n, &x, &y);
			if (get_temp_coords(t, levell, nodee, &x, &y)) {

			    if ((!(((-x0 / view->zoom > view->clipX1)
				    && (-x0 / view->zoom < view->clipX2)
				    && (-y0 / view->zoom > view->clipY1)
				    && (-y0 / view->zoom < view->clipY2))))
				&& (!(((-x / view->zoom > view->clipX1)
				       && (-x / view->zoom < view->clipX2)
				       && (-y / view->zoom > view->clipY1)
				       && (-y / view->zoom <
					   view->clipY2)))))

				continue;


			    glVertex3f((GLfloat) x0, (GLfloat) y0,
				       (GLfloat) 0);
			    glVertex3f((GLfloat) x, (GLfloat) y,
				       (GLfloat) 0);
			}
		    }
		}
	    }
	}
    }
    glEnd();

}

static int get_active_frame(topview * t)
{
    gulong microseconds;
    gdouble seconds;
    int fr;
    seconds = g_timer_elapsed(view->timer, &microseconds);
    fr = (int) (seconds / ((double) view->frame_length / (double) 1000));
    if (fr < view->total_frames) {

	if (fr == view->active_frame)
	    return 0;
	else {
	    view->active_frame = fr;
	    return 1;
	}
    } else {
	g_timer_stop(view->timer);
	view->Topview->fisheyeParams.animate = 0;
	return 0;
    }

}

void drawtopologicalfisheye(topview * t)
{
    get_active_frame(t);
    drawtopfishnodes(t);
    drawtopfishedges(t);
/*    if (!t->fisheyeParams.animate)
	drawtopfishnodelabels(t);*/

}

static void get_interpolated_coords(double x0, double y0, double x1, double y1,
			     int fr, int total_fr, double *x, double *y)
{
    *x = x0 + (x1 - x0) / (double) total_fr *(double) (fr + 1);
    *y = y0 + (y1 - y0) / (double) total_fr *(double) (fr + 1);
}

int get_temp_coords(topview * t, int level, int v, double *coord_x,
		    double *coord_y)
{
    Hierarchy *hp = t->fisheyeParams.h;
    ex_vtx_data *gg = hp->geom_graphs[level];
    /* v_data *g = hp->graphs[level]; */

    if (!t->fisheyeParams.animate) {
	if (gg[v].active_level != level)
	    return 0;

	*coord_x = (double) gg[v].physical_x_coord;
	*coord_y = (double) gg[v].physical_y_coord;
    } else {


	double x0, y0, x1, y1;
	int OAL, AL;

	x0 = 0;
	y0 = 0;
	x1 = 0;
	y1 = 0;
	AL = gg[v].active_level;
	OAL = gg[v].old_active_level;

	if ((OAL < level) || (AL < level))	//no draw 
	    return 0;
	if ((OAL >= level) || (AL >= level))	//draw the node
	{
	    if ((OAL == level) && (AL == level))	//draw as is from old coords to new)
	    {
		x0 = (double) gg[v].old_physical_x_coord;
		y0 = (double) gg[v].old_physical_y_coord;
		x1 = (double) gg[v].physical_x_coord;
		y1 = (double) gg[v].physical_y_coord;
	    }
	    if ((OAL > level) && (AL == level))	//draw as  from ancs  to new)
	    {
		find_old_physical_coords(t->fisheyeParams.h, level, v, &x0, &y0);
		x1 = (double) gg[v].physical_x_coord;
		y1 = (double) gg[v].physical_y_coord;
	    }
	    if ((OAL == level) && (AL > level))	//draw as  from ancs  to new)
	    {
		find_physical_coords(t->fisheyeParams.h, level, v, &x1, &y1);
		x0 = (double) gg[v].old_physical_x_coord;
		y0 = (double) gg[v].old_physical_y_coord;
	    }
	    get_interpolated_coords(x0, y0, x1, y1, view->active_frame,
				    view->total_frames, coord_x, coord_y);
	    if ((x0 == 0) || (x1 == 0))
		return 0;

	}
    }
    return 1;
}



/*  In loop,
 *  update fs.
 *    For example, if user clicks mouse at (p.x,p.y) to pick a single new focus,
 *      int closest_fine_node;
 *      find_closest_active_node(hierarchy, p.x, p.y, &closest_fine_node);
 *      fs->num_foci = 1;
 *      fs->foci_nodes[0] = closest_fine_node;
 *      fs->x_foci[0] = hierarchy->geom_graphs[cur_level][closest_fine_node].x_coord;
 *      fs->y_foci[0] = hierarchy->geom_graphs[cur_level][closest_fine_node].y_coord;
 *  set_active_levels(hierarchy, fs->foci_nodes, fs->num_foci);
 *  positionAllItems(hierarchy, fs, parms)
 */

#if 0
void infotopfisheye(topview * t, float *x, float *y, float *z)
{

    Hierarchy *hp = t->fisheyeParams.h;
    int closest_fine_node;
    find_closest_active_node(hp, *x, *y, &closest_fine_node);




/*		hp->geom_graphs[0][closest_fine_node].x_coord;
	    hp->geom_graphs[0][closest_fine_node].y_coord;*/
}
#endif



void changetopfishfocus(topview * t, float *x, float *y,
			float *z, int num_foci)
{

    gvcolor_t cl;
    focus_t *fs = t->fisheyeParams.fs;
    int i;
    int closest_fine_node;
    int cur_level = 0;

    Hierarchy *hp = t->fisheyeParams.h;
    refresh_old_values(t);
    fs->num_foci = num_foci;
    for (i = 0; i < num_foci; i++) {
	find_closest_active_node(hp, x[i], y[i], &closest_fine_node);
	fs->foci_nodes[i] = closest_fine_node;
	fs->x_foci[i] =
	    hp->geom_graphs[cur_level][closest_fine_node].x_coord;
	fs->y_foci[i] =
	    hp->geom_graphs[cur_level][closest_fine_node].y_coord;
    }


    view->Topview->fisheyeParams.repos.width =
	(int) (view->bdxRight - view->bdxLeft);
    view->Topview->fisheyeParams.repos.height =
	(int) (view->bdyTop - view->bdyBottom);

    colorxlate(get_attribute_value
	       ("topologicalfisheyefinestcolor", view,
		view->g[view->activeGraph]), &cl, RGBA_DOUBLE);
    view->Topview->fisheyeParams.srcColor.R = (float) cl.u.RGBA[0];
    view->Topview->fisheyeParams.srcColor.G = (float) cl.u.RGBA[1];
    view->Topview->fisheyeParams.srcColor.B = (float) cl.u.RGBA[2];
    colorxlate(get_attribute_value
	       ("topologicalfisheyecoarsestcolor", view,
		view->g[view->activeGraph]), &cl, RGBA_DOUBLE);
    view->Topview->fisheyeParams.tarColor.R = (float) cl.u.RGBA[0];
    view->Topview->fisheyeParams.tarColor.G = (float) cl.u.RGBA[1];
    view->Topview->fisheyeParams.tarColor.B = (float) cl.u.RGBA[2];



    sscanf(agget
	   (view->g[view->activeGraph],
	    "topologicalfisheyedistortionfactor"), "%lf",
	   &view->Topview->fisheyeParams.repos.distortion);
    sscanf(agget
	   (view->g[view->activeGraph], "topologicalfisheyefinenodes"),
	   "%d", &view->Topview->fisheyeParams.level.num_fine_nodes);
    sscanf(agget
	   (view->g[view->activeGraph],
	    "topologicalfisheyecoarseningfactor"), "%lf",
	   &view->Topview->fisheyeParams.level.coarsening_rate);
    sscanf(agget
	   (view->g[view->activeGraph], "topologicalfisheyedist2limit"),
	   "%d", &view->Topview->fisheyeParams.hier.dist2_limit);
    sscanf(agget(view->g[view->activeGraph], "topologicalfisheyeanimate"),
	   "%d", &view->Topview->fisheyeParams.animate);




    set_active_levels(hp, fs->foci_nodes, fs->num_foci, &(t->fisheyeParams.level));


    positionAllItems(hp, fs, &(t->fisheyeParams.repos));

    view->Topview->fisheyeParams.animate = 1;

    if (t->fisheyeParams.animate) {
	view->active_frame = 0;
	g_timer_start(view->timer);
    }

}



#ifdef UNUSED
void drawtopologicalfisheyestatic(topview * t)
{
    int level, v;
    Hierarchy *hp = t->fisheyeParams.h;

    glPointSize(15);
    glBegin(GL_POINTS);
    for (level = 0; level < hp->nlevels; level++) {
	for (v = 0; v < hp->nvtxs[level]; v++) {
	    ex_vtx_data *gg = hp->geom_graphs[level];
	    if (gg[v].active_level == level) {
		double x0 = gg[v].physical_x_coord;
		double y0 = gg[v].physical_y_coord;
//              glColor3f((GLfloat)(hp->nlevels-level)/(GLfloat)hp->nlevels,(GLfloat)level/(GLfloat)hp->nlevels,0);
		glColor4f((GLfloat) 0, (GLfloat) 1, (GLfloat) 0,
			  (GLfloat) 0.2);
		glVertex3f((GLfloat) x0, (GLfloat) y0, (GLfloat) 0);
	    }
	}
    }
    glEnd();




/*	glBegin(GL_LINES);
    for (level=0;level < hp->nlevels;level++) {
	for (v=0;v < hp->nvtxs[level]; v++) {
	    ex_vtx_data* gg = hp->geom_graphs[level];
	    vtx_data* g = hp->graphs[level];
	    if(gg[v].active_level==level) {
		double x0 = gg[v].physical_x_coord;
		double y0 = gg[v].physical_y_coord;

		for (i=1;i < g[v].nedges;i++) {
		    double x,y;
			n = g[v].edges[i];
			glColor3f((GLfloat)(hp->nlevels-level)/(GLfloat)hp->nlevels,(GLfloat)level/(GLfloat)hp->nlevels,0);
			if (gg[n].active_level == level) {
			if (v < n) {
			    x = gg[n].physical_x_coord;
			    y = gg[n].physical_y_coord;
			    glVertex3f((GLfloat)x0,(GLfloat)y0,(GLfloat)0);
			    glVertex3f((GLfloat)x,(GLfloat)y,(GLfloat)0);
			}
			}
		    else if (gg[n].active_level > level) {
			find_physical_coords(hp, level, n, &x, &y);
			glVertex3f((GLfloat)x0,(GLfloat)y0,(GLfloat)0);
			glVertex3f((GLfloat)x,(GLfloat)y,(GLfloat)0);
		    }
		}
	    }
	}
    }
    glEnd();*/
}
#endif
