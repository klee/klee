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
#ifdef _WIN32
#include <windows.h>
#include <io.h>
#else
#include "unistd.h"
#endif
#include "compat.h"
#include "viewport.h"
#include "draw.h"
#include "color.h"
#include <glade/glade.h>
#include "gui.h"
#include "menucallbacks.h"
#include "string.h"
#include "glcompui.h"
/* #include "topview.h" */
#include "gltemplate.h"
#include "colorprocs.h"
#include "memory.h"
#include "topviewsettings.h"
#include "md5.h"
#include "arcball.h"
#include "hotkeymap.h"
#include "topviewfuncs.h"


  /* Forward declarations */
#ifdef UNUSED
static int init_object_custom_data(Agraph_t * graph, void *obj);
static void refresh_borders(Agraph_t * g);
#endif

static colorschemaset *create_color_theme(int themeid);
static md5_byte_t *get_md5_key(Agraph_t * graph);


#define countof( array ) ( sizeof( array )/sizeof( array[0] ) )

ViewInfo *view;
/* these two global variables should be wrapped in something else */
GtkMessageDialog *Dlg;
int respond;
#ifdef UNUSED
static int mapbool(char *p)
{
    if (p == NULL)
	return FALSE;
    if (!strcasecmp(p, "false"))
	return FALSE;
    if (!strcasecmp(p, "true"))
	return TRUE;
    return atoi(p);
}
static Dtdisc_t qDisc = {
    offsetof(xdot, ops),
    sizeof(xdot_op *),
    -1,
    NIL(Dtmake_f),
    NIL(Dtfree_f),
    NIL(Dtcompar_f),
    NIL(Dthash_f),
    NIL(Dtmemory_f),
    NIL(Dtevent_f)
};
#endif


static void clear_viewport(ViewInfo * view)
{
    /*free topview if there is one */
    if (view->activeGraph >= 0)
    {
	freeSmGraph(view->g[view->activeGraph],view->Topview);
    }
    if (view->graphCount)
	agclose(view->g[view->activeGraph]);
//      init_viewport(view);
}
static void *get_glut_font(int ind)
{

    switch (ind) {
    case 0:
	return GLUT_BITMAP_9_BY_15;
	break;
    case 1:
	return GLUT_BITMAP_8_BY_13;
	break;
    case 2:
	return GLUT_BITMAP_TIMES_ROMAN_10;
	break;
    case 3:
	return GLUT_BITMAP_HELVETICA_10;
	break;
    case 4:
	return GLUT_BITMAP_HELVETICA_12;
	break;
    case 5:
	return GLUT_BITMAP_HELVETICA_18;
	break;
    default:
	return GLUT_BITMAP_TIMES_ROMAN_10;
    }

}
static void fill_key(md5_byte_t * b, md5_byte_t * data)
{
    int ind = 0;
    for (ind = 0; ind < 16; ind++) {
	b[ind] = data[ind];
    }

}

#if TEST_FOR_CHANGE
static int compare_keys(md5_byte_t * b1, md5_byte_t * b2)
{
    /*1 keys are equal */
    /*0 not equal */

    int ind = 0;
    int eq = 1;
    for (ind = 0; ind < 16; ind++) {
	if (b1[ind] != b2[ind]) {
	    eq = 0;
	}
    }
    return eq;
}
#endif

int close_graph(ViewInfo * view, int graphid)
{
    if (view->activeGraph < 0)
	return 1;
#if TEST_FOR_CHANGE
    /* This test should only be done if the user has something significant, not just
     * setting some smyrna control, which then becomes a changed attribute. In addition,
     * it might be good to allow a user to express a preference for the service.
     */
    fill_key(view->final_key, get_md5_key(view->g[graphid]));
    if (!compare_keys(view->final_key, view->orig_key))
	view->Topview->Graphdata.Modified = 1;
    if (view->Topview->Graphdata.Modified) {
	switch (show_close_nosavedlg()) {
	case 0:		/*save and close */
	    save_graph();
	    clear_viewport(view);
	    return 1;
	    break;
	case 1:		/*dont save but close */
	    clear_viewport(view);
	    return 1;
	    break;
	case 2:		/*cancel do nothing */
	    return 0;
	    break;
	default:
	    break;
	}
    }
#endif
    clear_viewport(view);
    return 1;

}

char *get_attribute_value(char *attr, ViewInfo * view, Agraph_t * g)
{
    char *buf;
    buf = agget(g, attr);
    if ((!buf) || (*buf == '\0'))
	buf = agget(view->systemGraphs.def_attrs, attr);
    return buf;
}

void set_viewport_settings_from_template(ViewInfo * view, Agraph_t * g)
{
    gvcolor_t cl;
    char *buf;
    colorxlate(get_attribute_value("bordercolor", view, g), &cl,
	       RGBA_DOUBLE);
    /* glEnable(GL_POINT_SMOOTH); */
    view->borderColor.R = (float) cl.u.RGBA[0];
    view->borderColor.G = (float) cl.u.RGBA[1];
    view->borderColor.B = (float) cl.u.RGBA[2];

    view->borderColor.A =
	(float) atof(get_attribute_value("bordercoloralpha", view, g));

    view->bdVisible = atoi(get_attribute_value("bordervisible", view, g));

    buf = get_attribute_value("gridcolor", view, g);
    colorxlate(buf, &cl, RGBA_DOUBLE);
    view->gridColor.R = (float) cl.u.RGBA[0];
    view->gridColor.G = (float) cl.u.RGBA[1];
    view->gridColor.B = (float) cl.u.RGBA[2];
    view->gridColor.A =
	(float) atof(get_attribute_value("gridcoloralpha", view, g));

    view->gridSize = (float) atof(buf =
				  get_attribute_value("gridsize", view,
						      g));

    view->defaultnodeshape = atoi(buf =
				  get_attribute_value("defaultnodeshape",
						      view, g));
    /* view->Selection.PickingType=atoi(buf=get_attribute_value("defaultselectionmethod", view,g)); */



    view->gridVisible = atoi(get_attribute_value("gridvisible", view, g));

    //mouse mode=pan

    //background color , default white
    colorxlate(get_attribute_value("bgcolor", view, g), &cl, RGBA_DOUBLE);

    view->bgColor.R = (float) cl.u.RGBA[0];
    view->bgColor.G = (float) cl.u.RGBA[1];
    view->bgColor.B = (float) cl.u.RGBA[2];
    view->bgColor.A = (float) 1;

    //selected nodes are drawn with this color
    colorxlate(get_attribute_value("selectednodecolor", view, g), &cl,
	       RGBA_DOUBLE);
    view->selectedNodeColor.R = (float) cl.u.RGBA[0];
    view->selectedNodeColor.G = (float) cl.u.RGBA[1];
    view->selectedNodeColor.B = (float) cl.u.RGBA[2];
    view->selectedNodeColor.A = (float)
	atof(get_attribute_value("selectednodecoloralpha", view, g));
    //selected edge are drawn with this color
    colorxlate(get_attribute_value("selectededgecolor", view, g), &cl,
	       RGBA_DOUBLE);
    view->selectedEdgeColor.R = (float) cl.u.RGBA[0];
    view->selectedEdgeColor.G = (float) cl.u.RGBA[1];
    view->selectedEdgeColor.B = (float) cl.u.RGBA[2];
    view->selectedEdgeColor.A = (float)
	atof(get_attribute_value("selectededgecoloralpha", view, g));

    colorxlate(get_attribute_value("highlightednodecolor", view, g), &cl,
	       RGBA_DOUBLE);
    view->highlightedNodeColor.R = (float) cl.u.RGBA[0];
    view->highlightedNodeColor.G = (float) cl.u.RGBA[1];
    view->highlightedNodeColor.B = (float) cl.u.RGBA[2];
    view->highlightedNodeColor.A = (float)
	atof(get_attribute_value("highlightednodecoloralpha", view, g));

    buf = agget(g, "highlightededgecolor");
    colorxlate(get_attribute_value("highlightededgecolor", view, g), &cl,
	       RGBA_DOUBLE);
    view->highlightedEdgeColor.R = (float) cl.u.RGBA[0];
    view->highlightedEdgeColor.G = (float) cl.u.RGBA[1];
    view->highlightedEdgeColor.B = (float) cl.u.RGBA[2];
    view->highlightedEdgeColor.A = (float)
	atof(get_attribute_value("highlightededgecoloralpha", view, g));
    view->defaultnodealpha = (float)
	atof(get_attribute_value("defaultnodealpha", view, g));

    view->defaultedgealpha = (float)
	atof(get_attribute_value("defaultedgealpha", view, g));



    /*default line width */
    view->LineWidth =
	(float) atof(get_attribute_value("defaultlinewidth", view, g));
    view->FontSize =
	(float) atof(get_attribute_value("defaultfontsize", view, g));

    view->topviewusermode = atoi(get_attribute_value("usermode", view, g));
    get_attribute_value("defaultmagnifierwidth", view, g);
    view->mg.width =
	atoi(get_attribute_value("defaultmagnifierwidth", view, g));
    view->mg.height =
	atoi(get_attribute_value("defaultmagnifierheight", view, g));

    view->mg.kts =
	(float) atof(get_attribute_value("defaultmagnifierkts", view, g));

    view->fmg.constantR =
	atoi(get_attribute_value
	     ("defaultfisheyemagnifierradius", view, g));

    view->fmg.fisheye_distortion_fac =
	atoi(get_attribute_value
	     ("defaultfisheyemagnifierdistort", view, g));
    view->drawnodes = atoi(get_attribute_value("drawnodes", view, g));
    view->drawedges = atoi(get_attribute_value("drawedges", view, g));
    view->drawnodelabels=atoi(get_attribute_value("labelshownodes", view, g));
    view->drawedgelabels=atoi(get_attribute_value("labelshowedges", view, g));
    view->nodeScale=atof(get_attribute_value("nodesize", view, g))*.30;

    view->FontSizeConst = 0;	//this will be calculated later in topview.c while calculating optimum font size

    view->glutfont =
	get_glut_font(atoi(get_attribute_value("labelglutfont", view, g)));
    colorxlate(get_attribute_value("nodelabelcolor", view, g), &cl,
	       RGBA_DOUBLE);
    view->nodelabelcolor.R = (float) cl.u.RGBA[0];
    view->nodelabelcolor.G = (float) cl.u.RGBA[1];
    view->nodelabelcolor.B = (float) cl.u.RGBA[2];
    view->nodelabelcolor.A =
	(float) atof(get_attribute_value("defaultnodealpha", view, g));
    colorxlate(get_attribute_value("edgelabelcolor", view, g), &cl,
	       RGBA_DOUBLE);
    view->edgelabelcolor.R = (float) cl.u.RGBA[0];
    view->edgelabelcolor.G = (float) cl.u.RGBA[1];
    view->edgelabelcolor.B = (float) cl.u.RGBA[2];
    view->edgelabelcolor.A =
	(float) atof(get_attribute_value("defaultedgealpha", view, g));
    view->labelwithdegree =
	atoi(get_attribute_value("labelwithdegree", view, g));
    view->labelnumberofnodes =
	atof(get_attribute_value("labelnumberofnodes", view, g));
    view->labelshownodes =
	atoi(get_attribute_value("labelshownodes", view, g));
    view->labelshowedges =
	atoi(get_attribute_value("labelshowedges", view, g));
    view->colschms =
	create_color_theme(atoi
			   (get_attribute_value("colortheme", view, g)));
    view->edgerendertype=atoi(get_attribute_value("edgerender", view, g));


    if (view->graphCount > 0)
	glClearColor(view->bgColor.R, view->bgColor.G, view->bgColor.B, view->bgColor.A);	//background color
}

static gboolean gl_main_expose(gpointer data)
{
    if (view->activeGraph >= 0) {
	if (view->Topview->fisheyeParams.animate == 1)
	    expose_event(view->drawing_area, NULL, NULL);
	return 1;
    }
    return 1;
}

static void get_data_dir(void)
{
    if (view->template_file) {
	free(view->template_file);
	free(view->glade_file);
	free(view->attr_file);
    }

    view->template_file = strdup(smyrnaPath("template.dot"));
    view->glade_file = strdup(smyrnaPath("smyrna.glade"));
    view->attr_file = strdup(smyrnaPath("attrs.txt"));
}

void init_viewport(ViewInfo * view)
{
    FILE *input_file = NULL;
    FILE *input_file2 = NULL;
    static char* path;
    get_data_dir();
    input_file = fopen(view->template_file, "rb");
    if (!input_file) {
	fprintf(stderr,
		"default attributes template graph file \"%s\" not found\n",
		view->template_file);
	exit(-1);
    } 
    view->systemGraphs.def_attrs = agread(input_file, 0);
    fclose (input_file);

    if (!view->systemGraphs.def_attrs) {
	fprintf(stderr,
		"could not load default attributes template graph file \"%s\"\n",
		view->template_file);
	exit(-1);
    }
    if (!path)
	path = smyrnaPath("attr_widgets.dot");
//    printf ("%s\n", path);
    input_file2 = fopen(path, "rb");
    if (!input_file2) {
	fprintf(stderr,	"default attributes template graph file \"%s\" not found\n",smyrnaPath("attr_widgets.dot"));
	exit(-1);

    }
    view->systemGraphs.attrs_widgets = agread(input_file2, 0);
    fclose (input_file2);
    if (!(view->systemGraphs.attrs_widgets )) {
	fprintf(stderr,"could not load default attribute widgets graph file \"%s\"\n",smyrnaPath("attr_widgets.dot"));
	exit(-1);
    }
    //init graphs
    view->g = NULL;		//no graph, gl screen should check it
    view->graphCount = 0;	//and disable interactivity if count is zero

    view->bdxLeft = 0;
    view->bdxRight = 500;
    view->bdyBottom = 0;
    view->bdyTop = 500;
    view->bdzBottom = 0;
    view->bdzTop = 0;

    view->borderColor.R = 1;
    view->borderColor.G = 0;
    view->borderColor.B = 0;
    view->borderColor.A = 1;

    view->bdVisible = 1;	//show borders red

    view->gridSize = 10;
    view->gridColor.R = 0.5;
    view->gridColor.G = 0.5;
    view->gridColor.B = 0.5;
    view->gridColor.A = 1;
    view->gridVisible = 0;	//show grids in light gray

    //mouse mode=pan
    //pen color
    view->penColor.R = 0;
    view->penColor.G = 0;
    view->penColor.B = 0;
    view->penColor.A = 1;

    view->fillColor.R = 1;
    view->fillColor.G = 0;
    view->fillColor.B = 0;
    view->fillColor.A = 1;
    //background color , default white
    view->bgColor.R = 1;
    view->bgColor.G = 1;
    view->bgColor.B = 1;
    view->bgColor.A = 1;

    //selected objets are drawn with this color
    view->selectedNodeColor.R = 1;
    view->selectedNodeColor.G = 0;
    view->selectedNodeColor.B = 0;
    view->selectedNodeColor.A = 1;

    //default line width;
    view->LineWidth = 1;

    //default view settings , camera is not active
    view->GLDepth = 1;		//should be set before GetFixedOGLPos(int x, int y,float kts) funtion is used!!!!
    view->panx = 0;
    view->pany = 0;
    view->panz = 0;


    view->zoom = -20;
    view->texture = 1;
    view->FontSize = 52;

    view->topviewusermode = TOP_VIEW_USER_NOVICE_MODE;	//for demo
    view->mg.active = 0;
    view->mg.x = 0;
    view->mg.y = 0;
    view->mg.width = DEFAULT_MAGNIFIER_WIDTH;
    view->mg.height = DEFAULT_MAGNIFIER_HEIGHT;
    view->mg.kts = DEFAULT_MAGNIFIER_KTS;
    view->fmg.constantR = DEFAULT_FISHEYE_MAGNIFIER_RADIUS;
    view->fmg.active = 0;
    view->mouse.down = 0;
    view->activeGraph = -1;
    view->SignalBlock = 0;
    view->Topview = GNEW(topview);
    view->Topview->fisheyeParams.fs = 0;
    view->Topview->xDot=NULL;

    /* init topfish parameters */
    view->Topview->fisheyeParams.level.num_fine_nodes = 10;
    view->Topview->fisheyeParams.level.coarsening_rate = 2.5;
    view->Topview->fisheyeParams.hier.dist2_limit = 1;
    view->Topview->fisheyeParams.hier.min_nvtxs = 20;
    view->Topview->fisheyeParams.repos.rescale = Polar;
    view->Topview->fisheyeParams.repos.width =
	(int) (view->bdxRight - view->bdxLeft);
    view->Topview->fisheyeParams.repos.height =
	(int) (view->bdyTop - view->bdyBottom);
    view->Topview->fisheyeParams.repos.margin = 0;
    view->Topview->fisheyeParams.repos.graphSize = 100;
    view->Topview->fisheyeParams.repos.distortion = 1.0;
    /*create timer */
    view->timer = g_timer_new();
    view->timer2 = g_timer_new();
    view->timer3 = g_timer_new();

    g_timer_stop(view->timer);
    view->active_frame = 0;
    view->total_frames = 1500;
    view->frame_length = 1;
    /*add a call back to the main() */
    g_timeout_add_full((gint) G_PRIORITY_DEFAULT, (guint) 100,
		       gl_main_expose, NULL, NULL);
    view->cameras = '\0';;
    view->camera_count = 0;
    view->active_camera = -1;
    set_viewport_settings_from_template(view, view->systemGraphs.def_attrs);
    view->dfltViewType = VT_NONE;
    view->dfltEngine = GVK_NONE;
    view->Topview->Graphdata.GraphFileName = (char *) 0;
    view->Topview->Graphdata.Modified = 0;
    view->colschms = NULL;
    view->flush = 1;
    view->arcball = NEW(ArcBall_t);
    view->keymap.down=0;
    load_mouse_actions (NULL,view);
    view->refresh.color=1;
    view->refresh.pos=1;
    view->refresh.selection=1;
    view->refresh.visibility=1;
    view->refresh.nodesize=1;
    view->edgerendertype=0;
    if(view->guiMode!=GUI_FULLSCREEN)
	view->guiMode=GUI_WINDOWED;

    /*create glcomp menu system */
    view->widgets = glcreate_gl_topview_menu();

}


/* update_graph_params:
 * adds gledit params
 * assumes custom_graph_data has been attached to the graph.
 */
static void update_graph_params(Agraph_t * graph)
{
    agattr(graph, AGRAPH, "GraphFileName",
	   view->Topview->Graphdata.GraphFileName);
}

#ifdef UNUSED
/* clear_object_xdot:
 * clear single object's xdot info
 */
static int clear_object_xdot(void *obj)
{
    if (obj) {
	if (agattrsym(obj, "_draw_"))
	    agset(obj, "_draw_", "");
	if (agattrsym(obj, "_ldraw_"))
	    agset(obj, "_ldraw_", "");
	if (agattrsym(obj, "_hdraw_"))
	    agset(obj, "_hdraw_", "");
	if (agattrsym(obj, "_tdraw_"))
	    agset(obj, "_tdraw_", "");
	if (agattrsym(obj, "_hldraw_"))
	    agset(obj, "_hldraw_", "");
	if (agattrsym(obj, "_tldraw_"))
	    agset(obj, "_tldraw_", "");
	return 1;
    }
    return 0;
}

/* clear_graph_xdot:
 * clears all xdot  attributes, used especially before layout change
 */
static int clear_graph_xdot(Agraph_t * graph)
{
    Agnode_t *n;
    Agedge_t *e;
    Agraph_t *s;

    clear_object_xdot(graph);
    n = agfstnode(graph);

    for (s = agfstsubg(graph); s; s = agnxtsubg(s))
	clear_object_xdot(s);

    for (n = agfstnode(graph); n; n = agnxtnode(graph, n)) {
	clear_object_xdot(n);
	for (e = agfstout(graph, n); e; e = agnxtout(graph, e)) {
	    clear_object_xdot(e);
	}
    }
    return 1;
}

/* clear_graph:
 * clears custom data binded
 * FIXME: memory leak - free allocated storage
 */
static void clear_graph(Agraph_t * graph)
{

}

#endif

static Agraph_t *loadGraph(char *filename)
{
    Agraph_t *g;
    FILE *input_file;
    /* char* bf; */
    /* char buf[512]; */
    if (!(input_file = fopen(filename, "r"))) {
	g_print("Cannot open %s\n", filename);
	return 0;
    }
    g = agread(input_file, NIL(Agdisc_t *));
    fclose (input_file);
    if (!g) {
	g_print("Cannot read graph in  %s\n", filename);
	return 0;
    }

    /* If no position info, run layout with -Txdot
     */
    if (!agattr(g, AGNODE, "pos", NULL)) {
	g_print("There is no position info in graph %s in %s\n", agnameof(g), filename);
	agclose (g);
	return 0;
    }
//    free(view->Topview->Graphdata.GraphFileName);
    view->Topview->Graphdata.GraphFileName = strdup(filename);
    return g;
}

#ifdef UNUSED
static void refresh_borders(Agraph_t * g)
{
    sscanf(agget(g, "bb"), "%f,%f,%f,%f", &(view->bdxLeft),
	   &(view->bdyBottom), &(view->bdxRight), &(view->bdyTop));
}
#endif



/* add_graph_to_viewport_from_file:
 * returns 1 if successful else 0
 */
int add_graph_to_viewport_from_file(char *fileName)
{
    Agraph_t *graph = loadGraph(fileName);

    return add_graph_to_viewport(graph, fileName);
}

/* updateRecord:
 * Update fields which may be added dynamically.
 */
void updateRecord (Agraph_t* g)
{
    GN_size(g) = agattr (g, AGNODE, "size", 0);
    GN_visible(g) = agattr (g, AGNODE, "visible", 0);
    GN_selected(g) = agattr (g, AGNODE, "selected", 0);
    GN_labelattribute(g) = agattr (g, AGNODE, "nodelabelattribute", 0);

    GE_pos(g)=agattr(g,AGEDGE,"pos",0);
    GE_visible(g) = agattr (g, AGEDGE, "visible", 0);
    GE_selected(g) = agattr (g, AGEDGE, "selected", 0);
    GE_labelattribute(g) = agattr (g, AGEDGE, "edgelabelattribute", 0);
}

/* graphRecord:
 * add graphRec to graph if necessary.
 * update fields of graphRec.
 * We assume the graph has attributes nodelabelattribute, edgelabelattribute,
 * nodelabelcolor and edgelabelcolor from template.dot.
 * We assume nodes have pos attributes. 
 * Only size, visible, selected and edge pos may or may not be defined.
 */
static void
graphRecord (Agraph_t* g)
{
    agbindrec(g, "graphRec", sizeof(graphRec), 1);

    GG_nodelabelcolor(g) = agattr (g, AGRAPH, "nodelabelcolor", 0);
    GG_edgelabelcolor(g) = agattr (g, AGRAPH, "edgelabelcolor", 0);
    GG_labelattribute(g) = agattr (g, AGRAPH, "nodelabelattribute", 0);
    GG_elabelattribute(g) = agattr (g, AGRAPH, "edgelabelattribute", 0);

    GN_pos(g) = agattr (g, AGNODE, "pos", 0);


    updateRecord (g);
}

void refreshViewport(int doClear)
{
    Agraph_t *graph = view->g[view->activeGraph];
    view->refresh.color=1;
    view->refresh.nodesize=1;
    view->refresh.pos=1;
    view->refresh.selection=1;
    view->refresh.visibility=1;
    load_settings_from_graph(graph);

    if(view->guiMode!=GUI_FULLSCREEN)
	update_graph_from_settings(graph);
    set_viewport_settings_from_template(view, graph);
    graphRecord(graph);
    initSmGraph(graph,view->Topview);

//    update_topview(graph, view->Topview, 1);
    fill_key(view->orig_key, get_md5_key(graph));
    expose_event(view->drawing_area, NULL, NULL);
}

static void activate(int id, int doClear)
{
    view->activeGraph = id;
    refreshViewport(doClear);
}

int add_graph_to_viewport(Agraph_t * graph, char *id)
{
    if (graph) {
	view->graphCount = view->graphCount + 1;
	view->g =
	    (Agraph_t **) realloc(view->g,
				  sizeof(Agraph_t *) * view->graphCount);
	view->g[view->graphCount - 1] = graph;

	gtk_combo_box_append_text(view->graphComboBox, id);
	gtk_combo_box_set_active(view->graphComboBox,view->graphCount-1);
	activate(view->graphCount - 1, 0);
	return 1;
    } else {
	return 0;
    }

}
void switch_graph(int graphId)
{
    if ((graphId >= view->graphCount) || (graphId < 0))
	return;			/*wrong entry */
    else
	activate(graphId, 0);
}

#ifdef UNUSED
/* add_new_graph_to_viewport:
 * returns graph index , otherwise -1
 */
int add_new_graph_to_viewport(void)
{
    //returns graph index , otherwise -1
    Agraph_t *graph;
    graph = (Agraph_t *) malloc(sizeof(Agraph_t));
    if (graph) {
	view->graphCount = view->graphCount + 1;
	view->g[view->graphCount - 1] = graph;
	return (view->graphCount - 1);
    } else
	return -1;
}
#endif
static md5_byte_t md5_digest[16];
static md5_state_t pms;

static int append_to_md5(void *chan, const char *str)
{
    md5_append(&pms, (unsigned char *) str, (int) strlen(str));
    return 1;

}
static int flush_md5(void *chan)
{
    md5_finish(&pms, md5_digest);
    return 1;
}


static md5_byte_t *get_md5_key(Agraph_t * graph)
{
    Agiodisc_t *xio;
    Agiodisc_t a;
    xio = graph->clos->disc.io;
    a.afread = graph->clos->disc.io->afread;
    a.putstr = append_to_md5;
    a.flush = flush_md5;
    graph->clos->disc.io = &a;
    md5_init(&pms);
    agwrite(graph, NULL);
    graph->clos->disc.io = xio;
    return md5_digest;
}

/* save_graph_with_file_name:
 * saves graph with file name; if file name is NULL save as is
 */
int save_graph_with_file_name(Agraph_t * graph, char *fileName)
{
    int ret;
    FILE *output_file;
    update_graph_params(graph);
    if (fileName)
	output_file = fopen(fileName, "w");
    else if (view->Topview->Graphdata.GraphFileName)
	output_file = fopen(view->Topview->Graphdata.GraphFileName, "w");
    else {
	g_print("there is no file name to save! Programmer error\n");
	return 0;
    }
    if (output_file == NULL) {
	g_print("Cannot create file \n");
	return 0;
    }

    ret = agwrite(graph, (void *) output_file);
    fclose (output_file);
    if (ret) {
	g_print("%s successfully saved \n", fileName);
	return 1;
    }
    return 0;
}

/* save_graph:
 * save without prompt
 */
int save_graph(void)
{
    //check if there is an active graph
    if (view->activeGraph > -1) 
    {
	//check if active graph has a file name
	if (view->Topview->Graphdata.GraphFileName) {
	    return save_graph_with_file_name(view->g[view->activeGraph],
					     view->Topview->Graphdata.
					     GraphFileName);
	} else
	    return save_as_graph();
        fill_key(view->orig_key, get_md5_key(view->g[view->activeGraph]));
    }
    return 1;

}

/* save_as_graph:
 * save with prompt
 */
int save_as_graph(void)
{
    //check if there is an active graph
    if (view->activeGraph > -1) {
	GtkWidget *dialog;
	dialog = gtk_file_chooser_dialog_new("Save File",
					     NULL,
					     GTK_FILE_CHOOSER_ACTION_SAVE,
					     GTK_STOCK_CANCEL,
					     GTK_RESPONSE_CANCEL,
					     GTK_STOCK_SAVE,
					     GTK_RESPONSE_ACCEPT, NULL);
	gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER
						       (dialog), TRUE);
	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
	    char *filename;
	    filename =
		gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
	    save_graph_with_file_name(view->g[view->activeGraph],
				      filename);
	    g_free(filename);
	    gtk_widget_destroy(dialog);

	    return 1;
	} else {
	    gtk_widget_destroy(dialog);
	    return 0;
	}
    }
    return 0;
}


#ifdef UNUSED
/* move_node:
 */
static void movenode(void *obj, float dx, float dy)
{
    char buf[512];
    double x, y;
    Agsym_t *pos;

    if ((AGTYPE(obj) == AGNODE) && (pos = agattrsym(obj, "pos"))) {
	sscanf(agxget(obj, pos), "%lf,%lf", &x, &y);
	sprintf(buf, "%lf,%lf", x - dx, y - dy);
	agxset(obj, pos, buf);
    }
}

static char *move_xdot(void *obj, xdot * x, int dx, int dy, int dz)
{
    int i = 0;
    int j = 0;
    /* int a=0; */
    /* char* pch; */
    /* int pos[MAXIMUM_POS_COUNT];  //maximum pos count hopefully does not exceed 100 */
    if (!x)
	return "\0";

    for (i = 0; i < x->cnt; i++) {
	switch (x->ops[i].kind) {
	case xd_filled_polygon:
	case xd_unfilled_polygon:
	case xd_filled_bezier:
	case xd_unfilled_bezier:
	case xd_polyline:
	    for (j = 0; j < x->ops[i].u.polygon.cnt; j++) {
		x->ops[i].u.polygon.pts[j].x =
		    x->ops[i].u.polygon.pts[j].x - dx;
		x->ops[i].u.polygon.pts[j].y =
		    x->ops[i].u.polygon.pts[j].y - dy;
		x->ops[i].u.polygon.pts[j].z =
		    x->ops[i].u.polygon.pts[j].z - dz;
	    }
	    break;
	case xd_filled_ellipse:
	case xd_unfilled_ellipse:
	    x->ops[i].u.ellipse.x = x->ops[i].u.ellipse.x - dx;
	    x->ops[i].u.ellipse.y = x->ops[i].u.ellipse.y - dy;
	    //                      x->ops[i].u.ellipse.z=x->ops[i].u.ellipse.z-dz;
	    break;
	case xd_text:
	    x->ops[i].u.text.x = x->ops[i].u.text.x - dx;
	    x->ops[i].u.text.y = x->ops[i].u.text.y - dy;
	    //                      x->ops[i].u.text.z=x->ops[i].u.text.z-dz;
	    break;
	case xd_image:
	    x->ops[i].u.image.pos.x = x->ops[i].u.image.pos.x - dx;
	    x->ops[i].u.image.pos.y = x->ops[i].u.image.pos.y - dy;
	    //                      x->ops[i].u.image.pos.z=x->ops[i].u.image.pos.z-dz;
	    break;
	default:
	    break;
	}
    }
    view->GLx = view->GLx2;
    view->GLy = view->GLy2;
    return sprintXDot(x);


}

static char *offset_spline(xdot * x, float dx, float dy, float headx,
			   float heady)
{
    int i = 0;
    Agnode_t *headn, tailn;
    Agedge_t *e;
    e = x->obj;			//assume they are all edges, check function name
    headn = aghead(e);
    tailn = agtail(e);

    for (i = 0; i < x->cnt; i++)	//more than 1 splines ,possible
    {
	switch (x->ops[i].kind) {
	case xd_filled_polygon:
	case xd_unfilled_polygon:
	case xd_filled_bezier:
	case xd_unfilled_bezier:
	case xd_polyline:
	    if (OD_Selected((headn)->obj) && OD_Selected((tailn)->obj)) {
		for (j = 0; j < x->ops[i].u.polygon.cnt; j++) {
		    x->ops[i].u.polygon.pts[j].x =
			x->ops[i].u.polygon.pts[j].x + dx;
		    x->ops[i].u.polygon.pts[j].y =
			x->ops[i].u.polygon.pts[j].y + dy;
		    x->ops[i].u.polygon.pts[j].z =
			x->ops[i].u.polygon.pts[j].z + dz;
		}
	    }
	    break;
	}
    }
    return 0;
}
#endif

/* move_nodes:
 * move selected nodes 
 */
#ifdef UNUSED
void move_nodes(Agraph_t * g)
{
    Agnode_t *obj;

    float dx, dy;
    xdot *bf;
    int i = 0;
    dx = view->GLx - view->GLx2;
    dy = view->GLy - view->GLy2;

    if (GD_TopView(view->g[view->activeGraph]) == 0) {
	for (i = 0; i < GD_selectedNodesCount(g); i++) {
	    obj = GD_selectedNodes(g)[i];
	    bf = parseXDot(agget(obj, "_draw_"));
	    agset(obj, "_draw_",
		  move_xdot(obj, bf, (int) dx, (int) dy, 0));
	    free(bf);
	    bf = parseXDot(agget(obj, "_ldraw_"));
	    agset(obj, "_ldraw_",
		  move_xdot(obj, bf, (int) dx, (int) dy, 0));
	    free(bf);
	    movenode(obj, dx, dy);
	    //iterate edges
	    /*for (e = agfstout(g,obj) ; e ; e = agnxtout (g,e))
	       {
	       bf=parseXDot (agget(e,"_tdraw_"));
	       agset(e,"_tdraw_",move_xdot(e,bf,(int)dx,(int)dy,0.00));
	       free(bf);
	       bf=parseXDot (agget(e,"_tldraw_"));
	       agset(e,"_tldraw_",move_xdot(e,bf,(int)dx,(int)dy,0.00));
	       free(bf);
	       bf=parseXDot (agget(e,"_draw_"));
	       agset(e,"_draw_",offset_spline(bf,(int)dx,(int)dy,0.00,0.00,0.00));
	       free(bf);
	       bf=parseXDot (agget(e,"_ldraw_"));
	       agset(e,"_ldraw_",offset_spline(bf,(int)dx,(int)dy,0.00,0.00,0.00));
	       free (bf);
	       } */
	    /*              for (e = agfstin(g,obj) ; e ; e = agnxtin (g,e))
	       {
	       free(bf);
	       bf=parseXDot (agget(e,"_hdraw_"));
	       agset(e,"_hdraw_",move_xdot(e,bf,(int)dx,(int)dy,0.00));
	       free(bf);
	       bf=parseXDot (agget(e,"_hldraw_"));
	       agset(e,"_hldraw_",move_xdot(e,bf,(int)dx,(int)dy,0.00));
	       free(bf);
	       bf=parseXDot (agget(e,"_draw_"));
	       agset(e,"_draw_",offset_spline(e,bf,(int)dx,(int)dy,0.00,0.00,0.00));
	       free(bf);
	       bf=parseXDot (agget(e,"_ldraw_"));
	       agset(e,"_ldraw_",offset_spline(e,bf,(int)dx,(int)dy,0.00,0.00,0.00));
	       } */
	}
    }
}
#endif
int setGdkColor(GdkColor * c, char *color)
{
    gvcolor_t cl;
    if (color != '\0') {
	colorxlate(color, &cl, RGBA_DOUBLE);
	c->red = (int) (cl.u.RGBA[0] * 65535.0);
	c->green = (int) (cl.u.RGBA[1] * 65535.0);
	c->blue = (int) (cl.u.RGBA[2] * 65535.0);
	return 1;
    } else
	return 0;

}

void glexpose(void)
{
    expose_event(view->drawing_area, NULL, NULL);
}

#if 0
/*following code does not do what i like it to do*/
/*I liked to have a please wait window on the screen all i got was the outer borders of the window
GTK requires a custom widget expose function 
*/
void please_wait(void)
{
    gtk_widget_hide(glade_xml_get_widget(xml, "frmWait"));
    gtk_widget_show(glade_xml_get_widget(xml, "frmWait"));
    gtk_window_set_keep_above((GtkWindow *)
			      glade_xml_get_widget(xml, "frmWait"), 1);

}
void please_dont_wait(void)
{
    gtk_widget_hide(glade_xml_get_widget(xml, "frmWait"));
}
#endif

float interpol(float minv, float maxv, float minc, float maxc, float x)
{
    return ((x - minv) * (maxc - minc) / (maxv - minv) + minc);
}

void getcolorfromschema(colorschemaset * sc, float l, float maxl,
			glCompColor * c)
{
    int ind;
    float percl = l / maxl;

    if (sc->smooth) {
	/* For smooth schemas, s[0].perc = 0, so we start with ind=1 */
	for (ind = 1; ind < sc->schemacount-1; ind++) {
	    if (percl < sc->s[ind].perc)
		break;
	}
	c->R =
	    interpol(sc->s[ind - 1].perc, sc->s[ind].perc,
		     sc->s[ind - 1].c.R, sc->s[ind].c.R, percl);
	c->G =
	    interpol(sc->s[ind - 1].perc, sc->s[ind].perc,
		     sc->s[ind - 1].c.G, sc->s[ind].c.G, percl);
	c->B =
	    interpol(sc->s[ind - 1].perc, sc->s[ind].perc,
		     sc->s[ind - 1].c.B, sc->s[ind].c.B, percl);
    }
    else {
	for (ind = 0; ind < sc->schemacount-1; ind++) {
	    if (percl < sc->s[ind].perc)
		break;
	}
	c->R = sc->s[ind].c.R;
	c->G = sc->s[ind].c.G;
	c->B = sc->s[ind].c.B;
    }

    c->A = 1;
}

/* set_color_theme_color:
 * Convert colors as strings to RGB
 */
static void set_color_theme_color(colorschemaset * sc, char **colorstr, int smooth)
{
    int ind;
    int colorcnt = sc->schemacount;
    gvcolor_t cl;
    float av_perc;

    sc->smooth = smooth;
    if (smooth) {
	av_perc = 1.0 / (float) (colorcnt-1);
	for (ind = 0; ind < colorcnt; ind++) {
	    colorxlate(colorstr[ind], &cl, RGBA_DOUBLE);
	    sc->s[ind].c.R = cl.u.RGBA[0];
	    sc->s[ind].c.G = cl.u.RGBA[1];
	    sc->s[ind].c.B = cl.u.RGBA[2];
	    sc->s[ind].c.A = cl.u.RGBA[3];
	    sc->s[ind].perc = ind * av_perc;
	}
    }
    else {
	av_perc = 1.0 / (float) (colorcnt);
	for (ind = 0; ind < colorcnt; ind++) {
	    colorxlate(colorstr[ind], &cl, RGBA_DOUBLE);
	    sc->s[ind].c.R = cl.u.RGBA[0];
	    sc->s[ind].c.G = cl.u.RGBA[1];
	    sc->s[ind].c.B = cl.u.RGBA[2];
	    sc->s[ind].c.A = cl.u.RGBA[3];
	    sc->s[ind].perc = (ind+1) * av_perc;
	}
    }
}

static void clear_color_theme(colorschemaset * cs)
{
    free(cs->s);
    free(cs);
}

static char *deep_blue[] = {
    "#C8CBED", "#9297D3", "#0000FF", "#2C2E41"
};
static char *pastel[] = {
    "#EBBE29", "#D58C4A", "#74AE09", "#893C49"
};
static char *magma[] = {
    "#E0061E", "#F0F143", "#95192B", "#EB712F"
};
static char *rain_forest[] = {
    "#1E6A10", "#2ABE0E", "#AEDD39", "#5EE88B"
};
#define CSZ(x) (sizeof(x)/sizeof(char*))
typedef struct {
    int cnt;
    char **colors;
} colordata;
static colordata palette[] = {
    {CSZ(deep_blue), deep_blue},
    {CSZ(pastel), pastel},
    {CSZ(magma), magma},
    {CSZ(rain_forest), rain_forest},
};
#define NUM_SCHEMES (sizeof(palette)/sizeof(colordata))

static colorschemaset *create_color_theme(int themeid)
{
    colorschemaset *s;

    if ((themeid < 0) || (NUM_SCHEMES <= themeid)) {
	fprintf (stderr, "colorschemaset: illegal themeid %d\n", themeid);
	return view->colschms;
    }

    s = NEW(colorschemaset);
    if (view->colschms)
	clear_color_theme(view->colschms);

    s->schemacount = palette[themeid].cnt;
    s->s = N_NEW(s->schemacount,colorschema);
    set_color_theme_color(s, palette[themeid].colors, 1);

    return s;
}

