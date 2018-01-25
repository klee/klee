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

#include <stdio.h>

#include "compat.h"
#include <stdlib.h>
#include "gui.h"
/* #include "abstring.h" */
#include <glade/glade.h>
#include <gdk/gdkkeysyms.h>
#include <gdk/gdk.h>
#include "viewport.h"
#include "memory.h"
#include "frmobjectui.h"
#include <assert.h>
#include "sfstr.h"
#include "gvprpipe.h"



#ifdef WIN32
#define STRCASECMP stricmp
#else
#include <strings.h>
#define STRCASECMP strcasecmp
#endif

static attr_t *binarySearch(attr_list * l, char *searchKey);
static int sel_node;
static int sel_edge;
static int sel_graph;

static char *safestrdup(char *src)
{
    if (!src)
	return NULL;
    else
	return strdup(src);
}
static int get_object_type(void)
{
    if (gtk_toggle_button_get_active
	((GtkToggleButton *) glade_xml_get_widget(xml, "attrRB0")))
	return AGRAPH;
    if (gtk_toggle_button_get_active
	((GtkToggleButton *) glade_xml_get_widget(xml, "attrRB1")))
	return AGNODE;
    if (gtk_toggle_button_get_active
	((GtkToggleButton *) glade_xml_get_widget(xml, "attrRB2")))
	return AGEDGE;
    return -1;
}

void free_attr(attr_t * at)
{
    free(at->defValG);
    free(at->defValN);
    free(at->defValE);

    free(at->name);
    free(at);
}


attr_t *new_attr(void)
{
    attr_t *attr = malloc(sizeof(attr_t));
    attr->defValG = (char *) 0;
    attr->defValN = (char *) 0;
    attr->defValE = (char *) 0;
    attr->name = (char *) 0;
    attr->value = (char *) 0;
    attr->propagate = 0;
    attr->objType[0] = 0;
    attr->objType[1] = 0;
    attr->objType[2] = 0;
    return attr;
}


attr_t *new_attr_with_ref(Agsym_t * sym)
{
    attr_t *attr = new_attr();
    attr->name = safestrdup(sym->name);
    switch (sym->kind) {
    case AGRAPH:
	attr->objType[0] = 1;
	if (sym->defval)
	    attr->defValG = safestrdup(sym->defval);
	break;
    case AGNODE:
	attr->objType[1] = 1;
	if (sym->defval)
	    attr->defValN = safestrdup(sym->defval);
	break;
    case AGEDGE:
	attr->objType[2] = 1;
	if (sym->defval)
	    attr->defValE = safestrdup(sym->defval);
	break;
    }
    return attr;
}

attr_t *new_attr_ref(attr_t * refAttr)
{
    attr_t *attr = malloc(sizeof(attr_t));
    *attr = *refAttr;
    attr->defValG = safestrdup(refAttr->defValG);
    attr->defValN = safestrdup(refAttr->defValN);
    attr->defValE = safestrdup(refAttr->defValE);
    attr->name = safestrdup(refAttr->name);
    if (refAttr->value)
	attr->value = safestrdup(refAttr->value);
    return attr;
}

static void reset_attr_list_widgets(attr_list * l)
{
    int id;
    for (id = 0; id < MAX_FILTERED_ATTR_COUNT; id++) {
	gtk_label_set_text(l->fLabels[id], "");
    }
}



static void free_attr_list_widgets(attr_list * l)
{
    int id;
    for (id = 0; id < MAX_FILTERED_ATTR_COUNT; id++) {
	gtk_object_destroy((GtkObject *) l->fLabels[id]);
    }
}


void free_attr_list(attr_list * l)
{
    int id;
    for (id = 0; id < l->attr_count; id++) {
	free_attr(l->attributes[id]);
    }
    if (l->with_widgets)
	free_attr_list_widgets(l);
    free(l);
}
attr_list *attr_list_new(Agraph_t * g, int with_widgets)
{
    int id;
    attr_list *l = malloc(sizeof(attr_list));
    l->attr_count = 0;
    l->capacity = DEFAULT_ATTR_LIST_CAPACITY;
    l->attributes = malloc(DEFAULT_ATTR_LIST_CAPACITY * sizeof(attr_t *));
    l->with_widgets = with_widgets;
    /*create filter widgets */

/*			gtk_widget_add_events(glade_xml_get_widget(xml, "frmObject"),
			//  GDK_BUTTON_MOTION_MASK      = 1 << 4,
			  GDK_BUTTON_MOTION_MASK |
			  GDK_POINTER_MOTION_MASK |
			  GDK_BUTTON_PRESS_MASK | GDK_KEY_PRESS |
			  GDK_BUTTON_RELEASE_MASK |
			  GDK_SCROLL | GDK_VISIBILITY_NOTIFY_MASK);

    g_signal_connect((gpointer)glade_xml_get_widget(xml, "frmObject"), "motion_notify_event",  G_CALLBACK(attr_label_motion), NULL);*/
    if (with_widgets) {
	for (id = 0; id < MAX_FILTERED_ATTR_COUNT; id++) {
	    l->fLabels[id] = (GtkLabel *) gtk_label_new("");

	    gtk_widget_add_events((GtkWidget *) l->fLabels[id],
				  //  GDK_BUTTON_MOTION_MASK      = 1 << 4,
				  GDK_BUTTON_MOTION_MASK |
				  GDK_POINTER_MOTION_MASK |
				  GDK_BUTTON_PRESS_MASK | GDK_KEY_PRESS |
				  GDK_BUTTON_RELEASE_MASK |
				  GDK_SCROLL | GDK_VISIBILITY_NOTIFY_MASK);

	    gtk_widget_show((GtkWidget *) l->fLabels[id]);
	    Color_Widget_bg("blue", (GtkWidget *) l->fLabels[id]);
	    gtk_fixed_put((GtkFixed *) glade_xml_get_widget(xml, "fixed6"),
			  (GtkWidget *) l->fLabels[id], 10, 110 + id * 13);
	}
    }
    return l;
}

void print_attr_list(attr_list * l)
{
    int id;
    for (id = 0; id < l->attr_count; id++) {
	printf("%d  %s (%d %d %d) \n", l->attributes[id]->index,
	       l->attributes[id]->name, l->attributes[id]->objType[0],
	       l->attributes[id]->objType[1],
	       l->attributes[id]->objType[2]);
	printf("defG:%s defN:%s defE:%s\n", l->attributes[id]->defValG,
	       l->attributes[id]->defValN, l->attributes[id]->defValE);
    }
}

int attr_compare(const void *a, const void *b)
{
    attr_t *a1 = *(attr_t **) a;
    attr_t *a2 = *(attr_t **) b;
    return STRCASECMP(a1->name, a2->name);
}

static void attr_list_sort(attr_list * l)
{
    qsort(l->attributes, l->attr_count, sizeof(attr_t *), attr_compare);

}

void attr_list_add(attr_list * l, attr_t * a)
{
    int id;
    if ((!l) || (!a))
	return;
    l->attr_count++;
    if (l->attr_count == l->capacity) {
	l->capacity = l->capacity + EXPAND_CAPACITY_VALUE;
	l->attributes =
	    realloc(l->attributes, l->capacity * sizeof(attr_t *));
    }
    l->attributes[l->attr_count - 1] = a;
    if (l->attr_count > 1)
	attr_list_sort(l);
    /*update indices */
    for (id = 0; id < l->attr_count; id++)
	l->attributes[id]->index = id;



}
static attr_data_type get_attr_data_type(char c)
{
//typedef enum {attr_alpha,attr_float,attr_int,attr_bool,attr_drowdown,attr_color} attr_data_type;
    switch (c) {
    case 'A':
	return attr_alpha;
	break;
    case 'F':
	return attr_float;
	break;
    case 'B':
	return attr_bool;
	break;
    case 'I':
	return attr_int;
	break;
    }
    return attr_alpha;

}
static void object_type_helper(char *a, int *t)
{
    if (strcmp(a, "GRAPH") == 0)
	t[0] = 1;
    if (strcmp(a, "CLUSTER") == 0)
	t[0] = 1;
    if (strcmp(a, "NODE") == 0)
	t[1] = 1;
    if (strcmp(a, "EDGE") == 0)
	t[2] = 1;
    if (strcmp(a, "ANY_ELEMENT") == 0) {
	t[0] = 1;
	t[1] = 1;
	t[2] = 1;
    }
}

static void set_attr_object_type(char *str, int *t)
{

    char *a;
    a = strtok(str, " ");
    object_type_helper(a, t);
    while ((a = strtok(NULL, " or ")))
	object_type_helper(a, t);

}

static attr_t *binarySearch(attr_list * l, char *searchKey)
{
    int middle, low, high, res;
    low = 0;
    high = l->attr_count - 1;

    while (low <= high) {
	middle = (low + high) / 2;
	res = STRCASECMP(searchKey, l->attributes[middle]->name);
	if (res == 0) {
	    return l->attributes[middle];
	} else if (res < 0) {
	    high = middle - 1;
	} else {
	    low = middle + 1;
	}
    }
    return NULL;
}






static attr_t *pBinarySearch(attr_list * l, char *searchKey)
{
    char buf[512];
    int middle, low, high, res;
    low = 0;
    high = l->attr_count - 1;

    while (low <= high) {
	middle = (low + high) / 2;
	strncpy(buf, l->attributes[middle]->name, strlen(searchKey));
	buf[strlen(searchKey)] = '\0';
	res = STRCASECMP(searchKey, buf);
	if (res == 0) {
	    return l->attributes[middle];
	}
//        else if               ( searchKey < b[ middle ] ) {
	else if (res < 0) {
	    high = middle - 1;
	} else {
	    low = middle + 1;
	}
    }
    return NULL;

}






void create_filtered_list(char *prefix, attr_list * sl, attr_list * tl)
{
    int res;
    char buf[512];
    attr_t *at;
    int objKind = get_object_type();

    if (strlen(prefix) == 0)
	return;
    /*locate first occurance */
    at = pBinarySearch(sl, prefix);
    if (!at)
	return;

    res = 0;
    /*go backward to get the first */
    while ((at->index > 0) && (res == 0)) {
	at = sl->attributes[at->index - 1];
	strncpy(buf, at->name, strlen(prefix));
	buf[strlen(prefix)] = '\0';;
	res = STRCASECMP(prefix, buf);
    }
    res = 0;
    while ((at->index < sl->attr_count) && (res == 0)) {
	at = sl->attributes[at->index + 1];
	strncpy(buf, at->name, strlen(prefix));
	buf[strlen(prefix)] = '\0';
	res = STRCASECMP(prefix, buf);
	if ((res == 0) && (at->objType[objKind] == 1))
	    attr_list_add(tl, new_attr_ref(at));
    }
}
void filter_attributes(char *prefix, topview * t)
//void filter_attributes(char* prefix, attr_list* l)
{

    int ind;
    int tmp;

    attr_list *l = t->attributes;
    attr_list *fl = t->filtered_attr_list;
    int objKind = get_object_type();

    if (fl)
	free_attr_list(fl);
    fl = attr_list_new(NULL, 0);
    reset_attr_list_widgets(l);
    create_filtered_list(prefix, l, fl);
    for (ind = 0; ind < fl->attr_count; ind++) {
	gtk_label_set_text(l->fLabels[ind], fl->attributes[ind]->name);
    }

    Color_Widget_bg("white", glade_xml_get_widget(xml, "txtAttr"));
    if (fl->attr_count == 0)
	Color_Widget_bg("red", glade_xml_get_widget(xml, "txtAttr"));
    /*a new attribute can be entered */

    gtk_widget_show(glade_xml_get_widget(xml, "txtValue"));
    gtk_widget_show(glade_xml_get_widget(xml, "txtDefValue"));

    gtk_entry_set_text((GtkEntry *)
		       glade_xml_get_widget(xml, "txtDefValue"), "");
    gtk_entry_set_text((GtkEntry *) glade_xml_get_widget(xml, "txtValue"),
		       "");
    gtk_widget_set_sensitive(glade_xml_get_widget(xml, "txtDefValue"), 1);
    gtk_widget_show(glade_xml_get_widget(xml, "attrAddBtn"));
    gtk_widget_hide(glade_xml_get_widget(xml, "attrApplyBtn"));
    gtk_widget_hide(glade_xml_get_widget(xml, "attrApplyAllBtn"));
    gtk_widget_hide(glade_xml_get_widget(xml, "attrSearchBtn"));
    gtk_toggle_button_set_active((GtkToggleButton *)
				 glade_xml_get_widget(xml, "attrProg"), 0);




    if (strlen(prefix) == 0) {
	gtk_widget_hide(glade_xml_get_widget(xml, "attrAddBtn"));
	gtk_widget_hide(glade_xml_get_widget(xml, "attrApplyBtn"));
	gtk_widget_hide(glade_xml_get_widget(xml, "attrApplyAllBtn"));
	gtk_widget_hide(glade_xml_get_widget(xml, "attrSearchBtn"));
	gtk_widget_hide(glade_xml_get_widget(xml, "attrAddBtn"));
	gtk_widget_hide(glade_xml_get_widget(xml, "txtValue"));
	gtk_widget_hide(glade_xml_get_widget(xml, "txtDefValue"));
	Color_Widget_bg("white", glade_xml_get_widget(xml, "txtAttr"));
    }


    for (ind = 0; ind < fl->attr_count; ind++) {
	if (STRCASECMP(prefix, fl->attributes[ind]->name) == 0) {	/*an existing attribute */

	    Color_Widget_bg("green", glade_xml_get_widget(xml, "txtAttr"));

	    if (get_object_type() == AGRAPH)
		gtk_entry_set_text((GtkEntry *)
				   glade_xml_get_widget(xml,
							"txtDefValue"),
				   fl->attributes[0]->defValG);
	    if (get_object_type() == AGNODE)
		gtk_entry_set_text((GtkEntry *)
				   glade_xml_get_widget(xml,
							"txtDefValue"),
				   fl->attributes[0]->defValN);
	    if (get_object_type() == AGEDGE)
		gtk_entry_set_text((GtkEntry *)
				   glade_xml_get_widget(xml,
							"txtDefValue"),
				   fl->attributes[0]->defValE);
//                      gtk_entry_set_text((GtkEntry*)glade_xml_get_widget(xml, "txtValue"),agget(view->g[view->activeGraph],prefix));
	    gtk_widget_set_sensitive(glade_xml_get_widget
				     (xml, "txtDefValue"), 0);
	    gtk_widget_hide(glade_xml_get_widget(xml, "attrAddBtn"));
	    gtk_widget_show(glade_xml_get_widget(xml, "attrApplyBtn"));
	    gtk_widget_show(glade_xml_get_widget(xml, "attrApplyAllBtn"));
	    gtk_widget_show(glade_xml_get_widget(xml, "attrSearchBtn"));
	    gtk_toggle_button_set_active((GtkToggleButton *)
					 glade_xml_get_widget(xml,
							      "attrProg"),
					 fl->attributes[0]->propagate);
	    break;

	}
    }

    tmp = (((objKind == AGNODE) && (sel_node))
	   || ((objKind == AGEDGE) && (sel_edge)) || ((objKind == AGRAPH)
						      && (sel_graph)));
    gtk_widget_set_sensitive(glade_xml_get_widget(xml, "attrApplyBtn"),
			     tmp);
}

/*asttribute text changed call back*/

_BB void on_txtAttr_changed(GtkWidget * widget, gpointer user_data)
{
//      printf ("attr text has been changed to %s \n",gtk_entry_get_text((GtkEntry*)widget));
    filter_attributes((char *) gtk_entry_get_text((GtkEntry *) widget),
		      view->Topview);
}


static void set_refresh_filters(ViewInfo * v, int type, char *name)
{
    if (STRCASECMP(name, "pos") == 0)
	v->refresh.pos = 1;
    if (STRCASECMP(name, "color") == 0)
	v->refresh.color = 1;
    if ((STRCASECMP(name, "size") == 0) && (type == AGNODE))
	v->refresh.nodesize = 1;
    if (STRCASECMP(name, "selected") == 0)
	v->refresh.selection = 1;
    if (STRCASECMP(name, "visible") == 0)
	v->refresh.visibility = 1;

}

static void doApply (GtkWidget * widget, int doAll)
{
    char *attr_name;
    char *value;
    char *def_val;
    int prog;
    topview *t;
    Agnode_t *v;
    Agedge_t *e;
    Agraph_t *g;
    int objKind;
    Agsym_t *sym;
    attr_t *attr;

    /*values to be applied to selected objects */
    attr_name =
	(char *) gtk_entry_get_text((GtkEntry *)
				    glade_xml_get_widget(xml, "txtAttr"));
    def_val =
	(char *) gtk_entry_get_text((GtkEntry *)
				    glade_xml_get_widget(xml,
							 "txtDefValue"));
    value =
	(char *) gtk_entry_get_text((GtkEntry *)
				    glade_xml_get_widget(xml, "txtValue"));
    prog =
	gtk_toggle_button_get_active((GtkToggleButton *)
				     glade_xml_get_widget(xml,
							  "attrProg"));
    g = view->g[view->activeGraph];
    t = view->Topview;
    objKind = get_object_type();
    attr = binarySearch(view->Topview->attributes, attr_name);
    assert(attr);
    attr->propagate = prog;
    sym = agattr(g, objKind, attr_name, (char *) 0);
    if (!sym)			/*it shouldnt be null, just in case it is null */
	sym = agattr(g, objKind, attr_name, def_val);
    /*graph */
    if (objKind == AGRAPH)
	agset(g, attr_name, value);
    /*nodes */
    else if (objKind == AGNODE) {
	for (v = agfstnode(g); v; v = agnxtnode(g, v)) {
	    if (doAll || ND_selected(v))
		agxset(v, sym, value);
	}
    }
    /*edges */
    else if (objKind == AGEDGE) {
	for (v = agfstnode(g); v; v = agnxtnode(g, v)) {
	    for (e = agfstout(g, v); e; e = agnxtout(g, e)) {
		if (doAll || ED_selected(e))
		    agxset(e, sym, value);
	    }
	}
    } else
	fprintf(stderr,
	    "on_attrApplyBtn_clicked: unknown object kind %d\n",
	    objKind);
    set_refresh_filters(view, objKind, attr_name);
}

_BB void on_attrApplyBtn_clicked(GtkWidget * widget, gpointer user_data)
{
    doApply (widget, 0);
}

_BB void on_attrApplyAllBtn_clicked(GtkWidget * widget, gpointer user_data)
{
    doApply (widget, 1);
}

_BB void on_attrRB0_clicked(GtkWidget * widget, gpointer user_data)
{
    filter_attributes((char *)
		      gtk_entry_get_text((GtkEntry *)
					 glade_xml_get_widget(xml,
							      "txtAttr")),
		      view->Topview);

}

/* This is the action attached to the publish button on the attributes
 * window. What should happen?
 */
_BB void on_attrProg_toggled(GtkWidget * widget, gpointer user_data)
{
  /* FIX */
}

_BB void attr_label_motion(GtkWidget * widget, GdkEventMotion * event,
			   gpointer data)
{
#ifdef UNUSED
    float x = (float) event->x;
    float y = (float) event->y;
    printf("%f %f \n", x, y);
#endif
}
_BB void on_attrAddBtn_clicked(GtkWidget * widget, gpointer user_data)
{
    char *attr_name;
    char *value;
    char *defValue;
    int objKind;
    int prog;
    topview *t;
    Agraph_t *g;
    attr_t *attr;
    objKind = get_object_type();

    /*values to be applied to selected objects */
    attr_name =
	(char *) gtk_entry_get_text((GtkEntry *)
				    glade_xml_get_widget(xml, "txtAttr"));
    value =
	(char *) gtk_entry_get_text((GtkEntry *)
				    glade_xml_get_widget(xml, "txtValue"));
    defValue =
	(char *) gtk_entry_get_text((GtkEntry *)
				    glade_xml_get_widget(xml,
							 "txtDefValue"));
    prog =
	gtk_toggle_button_get_active((GtkToggleButton *)
				     glade_xml_get_widget(xml,
							  "attrProg"));
    g = view->g[view->activeGraph];
    t = view->Topview;
    /*try to find first */
    attr = binarySearch(t->attributes, attr_name);
    if (!attr) {
	attr = new_attr();
	attr->index = 0;
	attr->name = safestrdup(attr_name);
	attr->type = attr_alpha;
	attr->value = safestrdup("");
	attr->widget = NULL;
	attr_list_add(t->attributes, attr);
    }
    attr->propagate = 0;

    if (objKind == AGRAPH) {
	agattr(g, AGRAPH, attr_name, defValue);
	attr->defValG = safestrdup(defValue);
	attr->objType[0] = 1;
    }

    /*nodes */
    else if (objKind == AGNODE) {
	agattr(g, AGNODE, attr_name, defValue);
	attr->defValN = safestrdup(defValue);
	attr->objType[1] = 1;
    } else if (objKind == AGEDGE) {
	agattr(g, AGEDGE, attr_name, defValue);
	attr->defValE = safestrdup(defValue);
	attr->objType[2] = 1;
    } else
	fprintf(stderr, "on_attrAddBtn_clicked: unknown object kind %d\n",
		objKind);
    filter_attributes(attr_name, view->Topview);

}

attr_list *load_attr_list(Agraph_t * g)
{
    attr_t *attr;
    attr_list *l;
    FILE *file;
    Agsym_t *sym;		/*cgraph atttribute */
    char line[BUFSIZ];
    static char *smyrna_attrs;
    char *a;

    if (!smyrna_attrs)
	smyrna_attrs = smyrnaPath("attrs.txt");
    g = view->g[view->activeGraph];
    l = attr_list_new(NULL, 1);
    file = fopen(smyrna_attrs, "r");
    if (file != NULL) {
	int i = 0;
	while (fgets(line, BUFSIZ, file) != NULL) {
	    int idx = 0;
	    attr = new_attr();
	    a = strtok(line, ",");
	    attr->index = i;
	    attr->type = get_attr_data_type(a[0]);
	    while ((a = strtok(NULL, ","))) {
		/*C,(0)color, (1)black, (2)EDGE Or NODE Or CLUSTER, (3)ALL_ENGINES */

		switch (idx) {
		case 0:
		    /**/ attr->name = safestrdup(a);
		    break;
		case 1:
		    /**/ attr->defValG = safestrdup(a);
		    attr->defValN = safestrdup(a);
		    attr->defValE = safestrdup(a);
		    break;
		case 2:
		    /**/ set_attr_object_type(a, attr->objType);
		    break;
		}
		idx++;
	    }
	    i++;
	    attr_list_add(l, attr);

	}
	fclose(file);
    }
    sym = NULL;
    while ((sym = agnxtattr(g, AGRAPH, sym))) {
	attr = binarySearch(l, sym->name);
	if (attr)
	    attr->objType[0] = 1;
	else {
	    attr = new_attr_with_ref(sym);
	    attr_list_add(l, attr);
	}
    }
    sym = NULL;
    while ((sym = agnxtattr(g, AGNODE, sym))) {

	attr = binarySearch(l, sym->name);
	if (attr) {
	    attr->objType[1] = 1;

	} else {
	    attr = new_attr_with_ref(sym);
	    attr_list_add(l, attr);
	}

    }
    sym = NULL;
    while ((sym = agnxtattr(g, AGEDGE, sym))) {

	attr = binarySearch(l, sym->name);
	if (attr)
	    attr->objType[2] = 1;
	else {
	    attr = new_attr_with_ref(sym);
	    attr_list_add(l, attr);
	}
    }
    return l;
}

 /**/ static void set_header_text(void)
{
    int nodeCnt = 0;
    int edgeCnt = 0;
    char buf[512];
    Agedge_t* ep;
    Agnode_t* v;
    Agraph_t* g;

    g = view->g[view->activeGraph];
    for (v = agfstnode(g); v; v = agnxtnode(g, v)) {
	if (ND_selected(v))
	    nodeCnt++;
	for (ep = agfstout(g, v); ep; ep = agnxtout(g, ep)) {
	    if (ND_selected(v))
		edgeCnt++;
	}
    }
    sel_node = nodeCnt;
    sel_edge = edgeCnt;
    sel_graph = 1;

    sprintf(buf, "%d Nodes and %d edges selected", nodeCnt, edgeCnt);
    gtk_label_set_text((GtkLabel *) glade_xml_get_widget(xml, "label124"),
		       buf);
    gtk_entry_set_text((GtkEntry *) glade_xml_get_widget(xml, "txtAttr"),
		       "");
    Color_Widget_bg("white", glade_xml_get_widget(xml, "fixed6"));
}

void showAttrsWidget(topview * t)
{
    gtk_widget_hide(glade_xml_get_widget(xml, "dlgSettings"));
    gtk_widget_show(glade_xml_get_widget(xml, "dlgSettings"));
    gtk_notebook_set_current_page((GtkNotebook *)
				  glade_xml_get_widget(xml, "notebook3"),
				  ATTR_NOTEBOOK_IDX);
    set_header_text();
    filter_attributes("", view->Topview);
}

static void gvpr_select(char *attr, char *regex_str, int objType)
{

    static Sfio_t *sf;
    char *bf2;
    int i, j, argc;
    char **argv;

    if (!sf)
	sf = sfstropen();

    if (objType == AGNODE)
	sfprintf(sf, "N[%s==\"%s\"]{selected = \"1\"}", attr, regex_str);
    else if (objType == AGEDGE)
	sfprintf(sf, "E[%s==\"%s\"]{selected = \"1\"}", attr, regex_str);



    bf2 = sfstruse(sf);

    argc = 1;
    if (*bf2 != '\0')
	argc++;
    argv = N_NEW(argc + 1, char *);
    j = 0;
    argv[j++] = "smyrna";
    argv[j++] = strdup(bf2);

    run_gvpr(view->g[view->activeGraph], j, argv);
    for (i = 1; i < argc; i++)
	free(argv[i]);
    free(argv);
    set_header_text();
}


_BB void on_attrSearchBtn_clicked(GtkWidget * widget, gpointer user_data)
{

    char *attr =
	(char *) gtk_entry_get_text((GtkEntry *)
				    glade_xml_get_widget(xml, "txtAttr"));
    char *regex_str =
	(char *) gtk_entry_get_text((GtkEntry *)
				    glade_xml_get_widget(xml, "txtValue"));
    gvpr_select(attr, regex_str, get_object_type());

}

