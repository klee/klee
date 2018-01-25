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

#ifndef SMYRNADEFS_H
#define SMYRNADEFS_H

#ifdef WIN32
#ifndef NO_WIN_HEADER
#include "windows.h"
#endif
#endif
#ifdef	WIN32			//this  is needed on WIN32 to get libglade see the callback
#define _BB  __declspec(dllexport)
#else
#define _BB
#endif

#include "xdot.h"
#include <gtk/gtk.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <gtk/gtkgl.h>
#include "cgraph.h"
#include "glcompset.h"
#include "hier.h"
#include "md5.h"
#include "glutils.h"

#ifdef __cplusplus
extern "C" {
#endif

#define WITH_CGRAPH 1


#ifdef	WIN32			//this is needed on WIN32 to get libglade see the callback
#define _BB  __declspec(dllexport)
#else
#define _BB
#endif

typedef struct _ArcBall_t ArcBall_t;

#define IS_TEST_MODE_ON							0
#define	DEFAULT_MAGNIFIER_WIDTH					300
#define	DEFAULT_MAGNIFIER_HEIGHT				225
#define DEFAULT_MAGNIFIER_KTS					3	//x3
#define DEFAULT_FISHEYE_MAGNIFIER_RADIUS		250
#define TOP_VIEW_USER_ADVANCED_MODE				0
#define TOP_VIEW_USER_NOVICE_MODE				1
#define GL_VIEWPORT_FACTOR						100
//mouse modes
#define MM_PAN					0
#define MM_ZOOM					1
#define MM_ROTATE				2
#define MM_SINGLE_SELECT		3
#define MM_RECTANGULAR_SELECT	4
#define MM_RECTANGULAR_X_SELECT	5
#define MM_MOVE					10
#define MM_MAGNIFIER			20
#define MM_FISHEYE_MAGNIFIER	21
#define MM_FISHEYE_PICK		22  /*fisheye select foci point*/
#define MM_POLYGON_SELECT   30



#define GLOBAL_Z_OFFSET			0.001

#define MAX_ZOOM	500
#define MIN_ZOOM	0.005
#define ZOOM_STEPS	100

#define ZOOM_STEP	0.5
#define DEG2RAD  G_PI/180
#define RAD2DEG	  1/0.017453292519943

#define UNHIGHLIGHTED_ALPHA	0.3
#define Z_FORWARD_PLANE -0.00201
#define Z_MIDDLE_PLANE 0.0000
#define Z_BACK_PLANE -0.00199

#define NODE_ZOOM_LIMIT	-25.3
#define NODE_CIRCLE_LIMIT	-7.3

#define GL_DOTSIZE_CONSTANT -18
#define DOUBLE_IT 10.00


#define SPHERE_SLICE_COUNT 6
#define DOT_SIZE_CORRECTION_FAC 0.3


#define EXPAND_CAPACITY_VALUE 50
#define DEFAULT_ATTR_LIST_CAPACITY 100
#define MAX_FILTERED_ATTR_COUNT 50

typedef enum {attr_alpha,attr_float,attr_int,attr_bool,attr_drowdown,attr_color} attr_data_type;
typedef enum {smyrna_all,smyrna_2D,smyrna_3D,smyrna_fisheye,smyrna_all_but_fisheye} smyrna_view_mode;


typedef struct{
    int keyVal;
    int down;
}keymap_t;



typedef struct {
	int index;
	char* name;
	char* value;
	char* defValG;
	char* defValN;
	char* defValE;
	attr_data_type type;
	int objType[3];
	GtkWidget* widget;
	int propagate;
}attr_t;

typedef struct
{
	int attr_count;
	int capacity;
	attr_t** attributes;
	GtkLabel* fLabels[MAX_FILTERED_ATTR_COUNT];
	int with_widgets;
}attr_list;


    typedef enum { nodshapedot, nodeshapecircle } node_shape;
    typedef enum { leftmousebutton, rightmousebutton,
	    thirdmousebutton } clicked_mouse_button;
    typedef enum { MOUSE_ROTATE_X, MOUSE_ROTATE_Y, MOUSE_ROTATE_XY,
	    MOUSE_ROTATE_Z } mouse_rotate_axis;


    typedef struct
    {
	unsigned char *data;
	int w;
	int h;
    }image_data;
    typedef struct 
    {
	xdot_op op;
	void *obj;
	glCompFont* font;
	int size;
	int layer;
	int listId;/*opengl list id*/
	glCompImage* img;
	/* image_data iData; */
    } sdot_op;	
	
	

#define MAX_BTN_CNT 50
    typedef struct {
	float perc;
	glCompColor c;

    } colorschema;

    typedef enum { gvpr_no_arg,gvpr_obj_arg,gvpr_string_arg,gvpr_sel_node_arg,gvpr_sel_edge_arg} gvpr_arg_type;

    typedef struct {
	char* def;
	char *script;
	char *args;
	char *attr_name;	/*attribute name to identify script in the graph */
	void* obj;
	gvpr_arg_type arg_type;
    } gvprscript;
    //_on_click="(gvpr_no_arg)N{node.color="red"){N.color="blue"}";


    typedef struct {
	int schemacount;       /* number of colors */
	int smooth;            /* if true, interpolate */
	colorschema *s;
    } colorschemaset;

    typedef enum {
	VT_NONE,
	VT_XDOT,
	VT_TOPVIEW,
	VT_TOPFISH,
    } viewtype_t;


    typedef enum {
	GVE_NONE = -1,
	GVE_GRAPH,
	GVE_CLUSTER,
	GVE_NODE,
	GVE_EDGE		/* keep last */
    } gve_element;

    typedef enum {
	GVK_NONE = -1,
	GVK_DOT,
	GVK_NEATO,
	GVK_TWOPI,
	GVK_CIRCO,
	GVK_FDP,
	GVK_SFDP		/* keep last */
    } gvk_layout;

    typedef struct {
	int anglex;
	int angley;
	int anglez;
    } rotation;

    typedef struct {
	int anglex;
	int angley;
	int anglez;
    } gl3DNav;


    typedef struct {

	    int a;
    } topviewdata;
    typedef struct
    {
	int node_id;
	int edge_id;
	int selnode_id;
	int seledge_id;
	int nodelabel_id;
	int edgelabel_id;
    }topviewcache;

    typedef struct xdot_set xdot_set;
    typedef enum { GEpixels, GEinches, GEmm } GEunit;
    typedef struct {
	int color;
	int pos;
	int selection;
	int visibility;
	int nodesize;


    }refresh_filter;


    typedef struct 
    {
	int index;
	int action;
	int hotkey;
	glMouseButtonType type;
	int drag;
	smyrna_view_mode mode;
    }mouse_action_t;

    typedef struct _object_data	//has to be attached to every Node, Edge, Graph and cluster
    {
	Agrec_t h;
	int ID;
	char *ObjName;
	int ObjType;
	int Layer;
	int Visible;
	int Selected;
	int NumDataCount;
	float *NumData;
	int StrDataCount;
	char **StrData;
	int param;		//generic purpose param
	int TVRef;		//Topview reference
	int edgeid;		/*for only edges,  > 0  multiedges */

    } element_data;
    typedef struct _temp_node_record	//helper record to identofy head and tail of edges
    {
	Agrec_t h;
	int ID;
	int TVref;		//topview data structure reference
    } temp_node_record;

#define OD_Visible(p) (p.data.Visible)
#define OD_Locked(p) (p.data.Locked)
#define OD_Highlighted(p) (p.data.Highlighted)



    typedef enum { CAM_PERSPECTIVE, CAM_ORTHO } cam_t;

    typedef struct _viewport_camera {
	float x;
	float y;
	float z;

	float targetx;
	float targety;
	float targetz;
	int index;
	float anglexy;
	float anglexyz;

	float anglex;
	float angley;
	float anglez;


	float camera_vectorx;
	float camera_vectory;
	float camera_vectorz;
	float r;


	cam_t type;		//
    } viewport_camera;



    typedef struct 
	{
		Agnode_t *Node;
		/*original coordinates */
		float x;
		float y;
		float z;
		/*coordinates to draw */
		float distorted_x;
		float distorted_y;
		float distorted_z;
		float zoom_factor;
		int in_fish_eye;	//boolean value if to apply fisheye
		glCompColor Color;
		char *Label;
		char *Label2;
		int degree;
		float node_alpha;
		int valid;
		element_data data;
		float size;
		xdot* xDot;
    } topview_node;

    typedef struct 
	{
		Agedge_t *Edge;		//edge itself
		float x1;
		float y1;
		float z1;
		float x2;
		float y2;
		float z2;
		float length;
		topview_node *Node1;	//Tail
		topview_node *Node2;	//Head
		glCompColor Color;
		element_data data;
		xdot* xDot;
    } topview_edge;

    typedef struct _graph_data {
	Agrec_t h;
	char *GraphFileName;
	//graph's location, change these to move the whole graph
	int Modified;		//if graph has been modified after loading
	float offsetx;
	float offsety;
	float offsetz;
    } graph_data;



    typedef struct{
        Agrec_t h;
        glCompPoint A;
	float size;
	int selected;
	int visible;
	int printLabel;
	int TVref;
    }nodeRec;
#define NREC(n) ((nodeRec*)(aggetrec(n,"nodeRec",0)))
#define ND_visible(n) (NREC(n)->visible)
#define ND_selected(n) (NREC(n)->selected)
#define ND_printLabel(n) (NREC(n)->printLabel)
#define ND_A(n) (NREC(n)->A)
#define ND_size(n) (NREC(n)->size)
#define ND_TVref(n) (NREC(n)->TVref)

    typedef struct{
	Agrec_t h;
	glCompPoint posTail;
	glCompPoint posHead;
	int selected;
	int visible;
	int printLabel;
    }edgeRec;
#define EREC(e) ((edgeRec*)(aggetrec(e,"edgeRec",0)))
#define ED_visible(e) (EREC(e)->visible)
#define ED_selected(e) (EREC(e)->selected)
#define ED_printLabel(e) (EREC(e)->printLabel)
#define ED_posTail(e) (EREC(e)->posTail)
#define ED_posHead(e) (EREC(e)->posHead)

    typedef struct{
	Agrec_t h;
	Agsym_t* N_pos;
	Agsym_t* N_size;
	Agsym_t* N_visible;
	Agsym_t* N_selected;
	Agsym_t* G_nodelabelcolor;
	Agsym_t* GN_labelattribute;
	Agsym_t* N_labelattribute;
	Agsym_t* E_visible;
	Agsym_t* E_selected;
	Agsym_t* E_pos;
	Agsym_t* G_edgelabelcolor;
	Agsym_t* E_labelattribute;
	Agsym_t* GE_labelattribute;
    } graphRec;
#define GREC(g) ((graphRec*)(AGDATA(g)))
#define GN_pos(g) (GREC(g)->N_pos)
#define GN_size(g) (GREC(g)->N_size)
#define GN_visible(g) (GREC(g)->N_visible)
#define GN_selected(g) (GREC(g)->N_selected)
#define GG_nodelabelcolor(g) (GREC(g)->G_nodelabelcolor)
#define GN_labelattribute(g) (GREC(g)->N_labelattribute)
#define GG_labelattribute(g) (GREC(g)->GN_labelattribute)
#define GE_pos(g) (GREC(g)->E_pos)
#define GE_visible(g) (GREC(g)->E_visible)
#define GE_selected(g) (GREC(g)->E_selected)
#define GG_edgelabelcolor(g) (GREC(g)->G_edgelabelcolor)
#define GE_labelattribute(g) (GREC(g)->E_labelattribute)
#define GG_elabelattribute(g) (GREC(g)->GE_labelattribute)

#define GUI_WINDOWED   0
#define GUI_FULLSCREEN    1

    typedef struct _selection {
	glCompPoly selPoly;
	int selectNodes;
	int selectEdges;
    } selection;

    typedef struct {
	topview_node *Nodes;
	topview_edge *Edges;
	int Nodecount;
	int Edgecount;
	topviewdata *TopviewData;
	void *customptr;
	struct {
	    int active;	//1 draw hierarchy 0 draw regular topview
	    reposition_t repos;
	    levelparms_t level;
	    hierparms_t hier;
	    Hierarchy *h;
	    int animate;
	    glCompColor srcColor;	//fine node colors of topfisheye
	    glCompColor tarColor;	//supernode colors of fisheye
	    focus_t *fs;
    	} fisheyeParams;

	topview_node **picked_nodes;
	int picked_node_count;
	topview_edge **picked_edges;
	int picked_edge_count;

	graph_data Graphdata;
	int maxnodedegree;
	float maxedgelen;
	float minedgelen;
	float avgedgelength;
	float init_zoom;
	float fitin_zoom;
	xdot* xDot;
	float global_z;
	attr_list* attributes;/*attribute list*/
	attr_list* filtered_attr_list;

	topviewcache cache;
	int xdotId;
	selection sel;
	
    } topview;



    enum {
	COL_NAME = 0,
	COL_FILENAME,
	NUM_COLS
    };


/*    typedef struct _mouse_attr {
	int mouse_down;
	int mouse_mode;
	int pick;
	float mouse_X;
	float mouse_Y;
	float begin_x;
	float begin_y;
	float dx;
	float dy;
	float GLX;	
	float GLpos.y;
	float GLpos.z;
	mouse_rotate_axis rotate_axis;
	clicked_mouse_button button;
    } mouse_attr;*/



    typedef struct _attribute {
	char Type;
	char *Name;
	char *Default;
	char ApplyTo[GVE_EDGE + 1];
	char Engine[GVK_FDP + 1];
	char **ComboValues;
	int ComboValuesCount;
	GtkWidget *attrWidget;

    } attribute;

    typedef struct _magnifier {
	float x, y;
	float kts;		//zoom X
	float GLwidth, GLheight;
	int width, height;	//how big is the magnifier referenced from windows
	int active;
    } magnifier;

    typedef struct _fisheye_magnifier {
	float x, y;		//center coords of active circle
	float distortion_factor;	//distortion factor ,default 1
	int R;			//radius of  the magnifier 
	int constantR;		//radius of  the magnifier referenced from windows
	int active;
	int fisheye_distortion_fac;
    } fisheye_magnifier;
    typedef struct{
    Agraph_t *def_attrs;
    Agraph_t *attrs_widgets;
    }systemgraphs;


    typedef struct _ViewInfo {
	systemgraphs systemGraphs;
	/*view variables */
	float panx;
	float pany;
	float panz;
	float zoom;

	/*clipping coordinates, to avoid unnecesarry rendering */
	float clipX1, clipX2, clipY1, clipY2, clipZ1, clipZ2;

	/*background color */
	glCompColor bgColor;
	/*default pen color */
	glCompColor penColor;
	/*default fill color */
	glCompColor fillColor;
	/*highlighted Node Color */
	glCompColor highlightedNodeColor;
	/*highlighted Edge Color */
	glCompColor highlightedEdgeColor;
	/*grid color */
	glCompColor gridColor;	//grid color
	/*border color */
	glCompColor borderColor;
	/*selected node color */
	glCompColor selectedNodeColor;
	/*selected edge color */
	glCompColor selectedEdgeColor;
	/*default node alpha */
	float defaultnodealpha;
	/*default edge alpha */
	float defaultedgealpha;
	/*default node shape */
	int defaultnodeshape;

	/*default line width */
	float LineWidth;

	/*grid is drawn if this variable is 1 */
	int gridVisible;
	/*grid cell size in gl coords system */
	float gridSize;		//grid cell size

	/*draws a border in border colors if it is 1 */
	int bdVisible;		//if borders are visible (boundries of the drawing,
	/*border coordinates, needs to be calculated for each graph */

	/*randomize node colors or use default node color */
	int rndNodeColor;

	/*randomize edge colors or use default edge color */
	int rndEdgeColor;
	/*Font Size */
	float FontSize;


	float bdxLeft, bdyTop, bdzTop;
	float bdxRight, bdyBottom, bdzBottom;

	/*reserved , not being used yet */
	GEunit unit;		//default pixels :0  

	/*screen window size in 2d */
	int w, h;
	/*graph pointer to hold loaded graphs */
	Agraph_t **g;
	/*number of graphs loaded */
	int graphCount;
	/*active graph */
	int activeGraph;

	/*texture data */
	int texture;		/*1 texturing enabled, 0 disabled */
	/*opengl depth value to convert mouse to GL coords */
	float GLDepth;

	/*stores the info about status of mouse,pressed? what button ? where? */
//	mouse_attr mouse;
	glCompMouse mouse;

	/*selection object,refer to smyrnadefs.h for more info */
//	selection Selection;

	/*rectangular magnifier object */
	magnifier mg;
	/*fisheye magnifier object */
	fisheye_magnifier fmg;

	viewport_camera **cameras;
	int camera_count;	//number of cameras
	int active_camera;
	viewport_camera *selected_camera;	//selected camera does not have to nec. be active one 

	/*data attributes are read from graph's attributes DataAttribute1 and DataAttribute2 */
	char *node_data_attribute1;	/*for topview graphs this is the node data attribute to put as label */
	char *node_data_attribute2;	/*for topview graphs this is the node data attribute to be stored and used for something else */

	/*0 advanced users with editing options 1 nonice users just navigate (glmenu system) */
	int topviewusermode;
	/*this should not be confused with graphviz node shapes, it can be dot or circles (dots are rendered mych faster, circle looks handsome, if graph is ulta large go with dot */
//      node_shape nodeshape;
	/*if true and nodeshape is nodeshapecircle , radius of nodes changes with degree */
	int nodesizewithdegree;

	/*open gl canvas, used to be a globa variable before looks better wrapped in viewinfo */
	GtkWidget *drawing_area;

	/*some boolean variable for variety hacks used in the software */
	int SignalBlock;

	/*Topview data structure, refer topview.h for more info */
	topview *Topview;
	/*timer for animations */
	GTimer *timer;
	/*this timer is session timer and always active */
	GTimer *timer2;
	/*general purpose timer */
	GTimer *timer3;
	int active_frame;
	int total_frames;
	int frame_length;
	/*lately added */
	int drawnodes;
	int drawedges;
	int drawnodelabels;
	int drawedgelabels;

	/*labelling properties */
	void *glutfont;
	glCompColor nodelabelcolor;
	glCompColor edgelabelcolor;
	int labelwithdegree;
	int labelnumberofnodes;
	int labelshownodes;
	int labelshowedges;

	viewtype_t dfltViewType;
	gvk_layout dfltEngine;
	GtkTextBuffer *consoleText;
	float FontSizeConst;
	glCompSet *widgets;	//for novice user open gl menu
	int visiblenodecount;	/*helper variable to know the number of the nodes being rendered, good data to optimize speed */
	md5_byte_t orig_key[16];	/*md5 result for original graph */
	md5_byte_t final_key[16];	/*md5 result right before graph is saved */
	char *initFileName;	//file name from command line
	int initFile;
	int drawSplines;
	colorschemaset *colschms;
	char *glade_file;
	char* temp;
	char *template_file;
	char *attr_file;
	int flush;
	line interpol;
	gvprscript *scripts;
	int script_count;	/*# of scripts */
	GtkComboBox *graphComboBox;	/*pointer to graph combo box at top right */
	ArcBall_t *arcball;
	keymap_t keymap;
	mouse_action_t* mouse_actions;	/*customizable moouse interraction list*/
	int mouse_action_count;
	refresh_filter refresh;
	int edgerendertype;
	float nodeScale;
	int guiMode;
	char* optArg;

    } ViewInfo;
/*rotation steps*/

    extern ViewInfo *view;
    extern GtkMessageDialog *Dlg;
    extern int respond;
    extern char *smyrnaPath(char *suffix);
    extern char *smyrnaGlade;

    extern void glexpose(void);

    extern char *layout2s(gvk_layout gvkl);
    extern gvk_layout s2layout(char *s);
    extern char *element2s(gve_element);
    extern unsigned char SmrynaVerbose;

#ifdef __cplusplus
}				/* end extern "C" */
#endif
#endif
