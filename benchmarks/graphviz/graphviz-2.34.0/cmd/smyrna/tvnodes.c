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

#include "tvnodes.h"
#include "viewport.h"
#include "topviewfuncs.h"
#include "memory.h"

typedef struct {
    GType type;
    char *name;
    int editable;
} gridCol;
typedef struct {
    int count;
    gridCol **columns;
    GtkTreeStore *store;
    char *flds;
    char *buf;
} grid;

static char* ID = "ID";
static char* Name = "Name";
static char* Visible = "Visible";

/* induceEdges:
 * Add all edges of g which have both ends in sg to sg
 */
static void induceEdges (Agraph_t * g, Agraph_t * sg)
{
    Agnode_t *n;
    Agedge_t *e;

    for (n = agfstnode(sg); n; n = agnxtnode(sg, n)) {
	for (e = agfstout(g, n); e; e = agnxtout(g, e)) {
	    if (agsubnode(sg, aghead(e), 0)) {
		agsubedge(sg, e, 1);
	    }
	}
    }
}

/*
	call this function to create a subgraph from filtered nodes and maybe edges
*/
static int create_save_subgraph_from_filter(char *filename, int withEdges)
{
    Agraph_t *subg = agsubg(view->g[view->activeGraph], "temp", 1);
    FILE *outputfile;
    Agnode_t *v;
    Agraph_t *g;
    int ret;

    g = view->g[view->activeGraph];
    for (v = agfstnode(g); v; v = agnxtnode(g, v)) {
	if (ND_selected(v))
	    agsubnode(subg, v, 1);
    }
    if (withEdges)
	induceEdges (view->g[view->activeGraph], subg);

    if ((outputfile = fopen(filename, "w"))) {
	ret = agwrite(subg, outputfile);
	fclose(outputfile);
    } else {
	fprintf (stderr, "Could not open %s for writing\n", filename);
	ret = 1;
    }
    agdelsubg(view->g[view->activeGraph], subg);
    return ret;
}


static void set_visibility(Agraph_t * g, int visibility)
{
    Agnode_t *v;
    char *bf;
    Agsym_t *visible_attr = GN_visible(g);
    Agsym_t *selected_attr = GN_selected(g);

    if (!selected_attr)
	return;
    if (!visible_attr)
	visible_attr = GN_visible(g) = agattr(g, AGNODE, "visible", "1");
    if (visibility)
	bf = "1";
    else
	bf = "0";
    for (v = agfstnode(g); v; v = agnxtnode(g, v)) {
	if (!ND_selected(v)) continue;
	agxset(v, visible_attr, bf);
    }
}

int tv_show_all(void)
{
    set_visibility(view->g[view->activeGraph], 1);
    updateSmGraph(view->g[view->activeGraph], view->Topview);
    return 1;
}

int tv_hide_all(void)
{
    set_visibility(view->g[view->activeGraph], 0);
    updateSmGraph(view->g[view->activeGraph], view->Topview);

    return 1;
}

int tv_save_as(int withEdges)
{
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
	filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));

	create_save_subgraph_from_filter(filename, withEdges);
	g_free(filename);
	gtk_widget_destroy(dialog);

	return 1;
    } else {
	gtk_widget_destroy(dialog);
	return 0;
    }
    return 0;
}

static void create_text_column(char *Title, GtkTreeView * tree, int asso,
			       int editable)
{
    PangoColor c;
    GtkTreeViewColumn *column;
    GtkCellRendererText *renderer;


    renderer = (GtkCellRendererText *) gtk_cell_renderer_text_new();
    ((GtkCellRenderer *) renderer)->mode = GTK_CELL_RENDERER_MODE_EDITABLE;
    renderer->editable = editable;
    c.blue = 0;
    c.green = 1;
    c.red = 0;
    renderer->foreground = c;

    column =
	gtk_tree_view_column_new_with_attributes(Title,
						 (GtkCellRenderer *)
						 renderer, "text", asso,
						 NULL);

    gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);
    gtk_tree_view_column_set_resizable(column, 1);

}

static void create_toggle_column(char *Title, GtkTreeView * tree, int asso,
				 int editable)
{
    GtkTreeViewColumn *column;
    GtkCellRendererToggle *renderer;
    renderer = (GtkCellRendererToggle *) gtk_cell_renderer_toggle_new();
    renderer->activatable = editable;
    /* g_object_set(G_OBJECT(renderer), "foreground", "red", NULL); */

    column =
	gtk_tree_view_column_new_with_attributes(Title,
						 (GtkCellRenderer *)
						 renderer, "active", asso,
						 NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);
    gtk_tree_view_column_set_resizable(column, 1);

}

#ifdef UNUSED
static int boolStrMap(char *str)
{
    if (strcmp(str, "1") || strcmp(str, "true") || strcmp(str, "TRUE")
	|| strcmp(str, "True"))
	return 1;
    return 0;

}

#endif

static void populate_data(Agraph_t * g, grid * grid)
{
    Agnode_t *v;
    int id = 0;
    GtkTreeIter iter;
    GValue value = {0};
    char* bf;
    gridCol* cp;

    for (v = agfstnode(g); v; v = agnxtnode(g, v)) {
	if (!ND_selected(v))
	    continue;
	gtk_tree_store_append(grid->store, &iter, NULL);

	for (id = 0; id < grid->count; id++) {
	    cp = grid->columns[id];
	    if (cp->name == ID) continue;

	    if (cp->name == Name)
		bf = agnameof(v);
	    else
		bf = agget(v, cp->name);
	    if ((!bf) && (cp->name != Visible))
		continue;

	    g_value_init(&value, cp->type);
	    switch (grid->columns[id]->type) {
	    case G_TYPE_BOOLEAN:
		if (bf) {
		    if ((strcmp(bf, "1") == 0) || (strcmp(bf, "true") == 0)
			|| (strcmp(bf, "True") == 0))
			g_value_set_boolean(&value, 1);
		    else
			g_value_set_boolean(&value, 0);
		} else {
		    if (cp->name == Visible)
			g_value_set_boolean(&value, 1);
		}
		break;
	    default:
		g_value_set_static_string(&value, bf);

	    }
	    gtk_tree_store_set_value(grid->store, &iter, id, &value);
	    g_value_unset(&value);
	}
    }
}

static GtkTreeStore *update_tree_store(GtkTreeStore * store, int ncolumns,
				       GType * types)
{
    if ((ncolumns == 0) || (types == NULL))
	return NULL;
    if (store) {
	gtk_tree_store_clear(store);
	g_object_unref(store);
    }

    store = gtk_tree_store_newv(ncolumns, types);
    return store;
}


static void create_column(gridCol * c, GtkTreeView * tree, int id)
{
    if (!c)
	return;
    switch (c->type) {
    case G_TYPE_STRING:
    case G_TYPE_INT:
	create_text_column(c->name, tree, id, c->editable);
	break;
    case G_TYPE_BOOLEAN:
	create_toggle_column(c->name, tree, id, c->editable);
	break;
    default:
	create_text_column(c->name, tree, id, c->editable);
    }
}

static GtkTreeView *update_tree(GtkTreeView * tree, grid * g)
{

    GtkTreeStore *store = NULL;
    GtkTreeViewColumn *column;
    GType *types;
    int id = 0;

    if (tree) {
	while ((column = gtk_tree_view_get_column(tree, 0)))	/*clear all columns */
	    gtk_tree_view_remove_column(tree, column);
	store = (GtkTreeStore *) gtk_tree_view_get_model(tree);
    } else {
	tree = (GtkTreeView *) gtk_tree_view_new();
	gtk_widget_show((GtkWidget *) tree);

	gtk_container_add((GtkContainer *)
			  glade_xml_get_widget(xml, "scrolledwindow9"),
			  (GtkWidget *) tree);

    }
    if (g->count > 0) {
	types = N_NEW(g->count, GType);
	for (id = 0; id < g->count; id++)
	    types[id] = g->columns[id]->type;
	store = update_tree_store(g->store, g->count, types);
	free (types);
	gtk_tree_view_set_model(tree, (GtkTreeModel *) store);
	/*insert columns */
	for (id = 0; id < g->count; id++)
	    create_column(g->columns[id], tree, id);
    }
    g->store = store;
    return tree;
}

static void add_column(grid * g, char *name, int editable, GType g_type)
{
    if (*name == '\0')
	return;
    g->columns =
	(gridCol **) realloc(g->columns,
			     sizeof(gridCol *) * (g->count + 1));
    g->columns[g->count] = NEW(gridCol);
    g->columns[g->count]->editable = editable;
    g->columns[g->count]->name = name;
    g->columns[g->count]->type = g_type;
    g->count++;
}

static void clearGrid(grid * g)
{
    int id;
    for (id = 0; id < g->count; id++) {
	free(g->columns[id]);
    }
    free(g->columns);
    free(g->buf);
    g->count = 0;
    g->columns = 0;
    g->flds = 0;
}

static grid *initGrid()
{
    grid *gr = NEW(grid);
    gr->columns = NULL;
    gr->count = 0;
    gr->buf = 0;
    return gr;
}

static grid *update_columns(grid * g, char *str)
{
    /*free existing memory if necessary */
    char *a;
    char *buf;

    if (g) {
	if (g->flds != str)
	    clearGrid(g);
	else
	    return g;
    } else
	g = initGrid(str);
    /* add_column(g, ID, 0, G_TYPE_STRING); */
    add_column(g, Name, 0, G_TYPE_STRING);
    /* add_column(g, visible, 0, G_TYPE_BOOLEAN); */
    if (!str)
	return g;

    g->flds = str;
    buf = strdup (str);  /* need to dup because str is actual graph attribute value */
    a = strtok(buf, ",");
    add_column(g, a, 1, G_TYPE_STRING);
    while ((a = strtok(NULL, ",")))
	add_column(g, a, 1, G_TYPE_STRING);
    return g;
}

void setup_tree(Agraph_t * g)
{
    /*
       G_TYPE_STRING:
       G_TYPE_INT:
       G_TYPE_BOOLEAN:
     */
    static GtkTreeView *tree;
    static grid *gr;
    char *buf = agget(g, "datacolumns");

    gr = update_columns(gr, buf);
    tree = update_tree(tree, gr);
    populate_data(g, gr);
}
