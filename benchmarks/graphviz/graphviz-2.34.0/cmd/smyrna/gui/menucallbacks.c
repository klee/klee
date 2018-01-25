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

#include "menucallbacks.h"
#include "viewport.h"
/* #include "topview.h" */
#include "tvnodes.h"

#include "gvprpipe.h"
#include "topviewsettings.h"
#include "gltemplate.h"
#include "memory.h"
#include <const.h>
#include <agxbuf.h>
#include <assert.h>
#include <ctype.h>
#include  "frmobjectui.h"

//file
char buf[255];
void mAttributesSlot(GtkWidget * widget, gpointer user_data)
{
    showAttrsWidget(view->Topview);
}

void mOpenSlot(GtkWidget * widget, gpointer user_data)
{
    GtkWidget *dialog;
    GtkFileFilter *filter;
    int i = 0;

    filter = gtk_file_filter_new();
    gtk_file_filter_add_pattern(filter, "*.gv");
    gtk_file_filter_add_pattern(filter, "*.dot");
    dialog = gtk_file_chooser_dialog_new("Open File",
					 NULL,
					 GTK_FILE_CHOOSER_ACTION_OPEN,
					 GTK_STOCK_CANCEL,
					 GTK_RESPONSE_CANCEL,
					 GTK_STOCK_OPEN,
					 GTK_RESPONSE_ACCEPT, NULL);

    gtk_file_chooser_set_filter((GtkFileChooser *) dialog, filter);
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
	char *filename;
	filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
/*	if (view->activeGraph == 0)
		close_graph(view,0);*/

	i = view->SignalBlock;
	view->SignalBlock = 1;
	add_graph_to_viewport_from_file(filename);
	g_free(filename);
	view->SignalBlock = i;

    }

    gtk_widget_destroy(dialog);
}

void mSaveSlot(GtkWidget * widget, gpointer user_data)
{

    save_graph();		//save without prompt

}

void mSaveAsSlot(GtkWidget * widget, gpointer user_data)
{
    save_as_graph();		//save with prompt
}

void mCloseSlot(GtkWidget * widget, gpointer user_data)
{
    if (view->activeGraph == 0)
	close_graph(view, 0);
}

void mOptionsSlot(GtkWidget * widget, gpointer user_data)
{
}

void mQuitSlot(GtkWidget * widget, gpointer user_data)
{
    if (close_graph(view, view->activeGraph));
    gtk_main_quit();
}

int show_close_nosavedlg(void)
{
    GtkWidget *dialog;
    char buf[512];
    int rv;			/*return value */
    sprintf(buf,
	    "%s has been modified. Do you want to save it before closing?",
	    view->Topview->Graphdata.GraphFileName);
    dialog =
	gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL,
			       GTK_MESSAGE_QUESTION, GTK_BUTTONS_NONE,
			       buf);

    gtk_window_set_title(GTK_WINDOW(dialog), "Smyrna Warning");
    gtk_dialog_add_button((GtkDialog *) dialog, "Yes", 0);
    gtk_dialog_add_button((GtkDialog *) dialog, "No", 1);
    gtk_dialog_add_button((GtkDialog *) dialog, "Cancel", 2);
    rv = gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
    return rv;
}



//edit
void mCutSlot(GtkWidget * widget, gpointer user_data)
{
}

void mCopySlot(GtkWidget * widget, gpointer user_data)
{
}

void mPasteSlot(GtkWidget * widget, gpointer user_data)
{
}

void mDeleteSlot(GtkWidget * widget, gpointer user_data)
{
}
void mTopviewSettingsSlot(GtkWidget * widget, gpointer user_data)
{
    show_settings_form();
}



//view
void mShowToolBoxSlot(GtkWidget * widget, gpointer user_data)
{


    if (!gtk_widget_set_gl_capability
	(glade_xml_get_widget(xml, "glfixed"), configure_gl(),
	 gtk_widget_get_gl_context(view->drawing_area), 0, 0))
	printf("glwidget creation failed \n");



}

#if 0
void mShowHostSelectionSlot(GtkWidget * widget, gpointer user_data)
{

    gtk_widget_hide(glade_xml_get_widget(xml, "frmHostSelection"));
    gtk_widget_show(glade_xml_get_widget(xml, "frmHostSelection"));
    gtk_window_set_keep_above((GtkWindow *)
			      glade_xml_get_widget(xml,
						   "frmHostSelection"), 1);

}
#endif

void mShowConsoleSlot(GtkWidget * widget, gpointer user_data)
{
    static int state = 0;  // off by default

    if (state) {
	gtk_widget_hide (glade_xml_get_widget(xml, "vbox13"));
	gtk_widget_show (glade_xml_get_widget(xml, "show_console1"));
	gtk_widget_hide (glade_xml_get_widget(xml, "hide_console1")); 
	state = 0;
    }
    else {
	gtk_widget_show (glade_xml_get_widget(xml, "vbox13"));
	gtk_widget_hide (glade_xml_get_widget(xml, "show_console1"));
	gtk_widget_show (glade_xml_get_widget(xml, "hide_console1")); 
	state = 1;
    }
}

#if 0
void mHideConsoleSlot(GtkWidget * widget, gpointer user_data)
{
    gtk_widget_hide(glade_xml_get_widget(xml, "vbox13"));

}
#endif

//Graph
void mNodeListSlot(GtkWidget * widget, gpointer user_data)
{
	gtk_widget_show(glade_xml_get_widget(xml, "frmTVNodes"));
	setup_tree (view->g[view->activeGraph]);


}

void mNewNodeSlot(GtkWidget * widget, gpointer user_data)
{
}

void mNewEdgeSlot(GtkWidget * widget, gpointer user_data)
{
}
void mNewClusterSlot(GtkWidget * widget, gpointer user_data)
{
}

void mGraphPropertiesSlot(GtkWidget * widget, gpointer user_data)
{
    int respond;
    //there has to be an active graph to open the graph prop page
    if (view->activeGraph > -1) {
	load_graph_properties(view->g[view->activeGraph]);	//load from graph to gui              
	gtk_dialog_set_response_sensitive((GtkDialog *)
					  glade_xml_get_widget(xml,
							       "dlgOpenGraph"),
					  1, 1);
	gtk_dialog_set_response_sensitive((GtkDialog *)
					  glade_xml_get_widget(xml,
							       "dlgOpenGraph"),
					  2, 1);
	respond = gtk_dialog_run((GtkDialog *)
				 glade_xml_get_widget(xml,
						      "dlgOpenGraph"));
	//need to hide the dialog , again freaking GTK!!!!
	gtk_widget_hide(glade_xml_get_widget(xml, "dlgOpenGraph"));
    }
}

void mNodeFindSlot(GtkWidget * widget, gpointer user_data)
{

}


static void mPropertiesSlot(gve_element element)
{
    if (view->activeGraph >= 0)
	gtk_widget_hide(glade_xml_get_widget(xml, "frmObject"));
    gtk_widget_show(glade_xml_get_widget(xml, "frmObject"));
//      load_object_properties(element, view->g[view->activeGraph]);
}

void mClusterPropertiesSlot(GtkWidget * widget, gpointer user_data)
{
    mPropertiesSlot(GVE_CLUSTER);
}

void mNodePropertiesSlot(GtkWidget * widget, gpointer user_data)
{
    mPropertiesSlot(GVE_NODE);
}

void mEdgePropertiesSlot(GtkWidget * widget, gpointer user_data)
{
    mPropertiesSlot(GVE_EDGE);
}

void mShowCodeSlot(GtkWidget * widget, gpointer user_data)
{
}
static void mSlot(GtkWidget * widget, gpointer user_data,
		  gvk_layout layout, int doCursor)
{
    /* GdkCursor *cursor; */
    /* GdkWindow *w; */
/*    Dlg = (GtkMessageDialog *) gtk_message_dialog_new(NULL,
						      GTK_DIALOG_MODAL,
						      GTK_MESSAGE_QUESTION,
						      GTK_BUTTONS_YES_NO,
						      "This will change the graph layout\n all your position changes will be lost\n Are you sure?");

    respond = gtk_dialog_run((GtkDialog *) Dlg);
    if (respond == GTK_RESPONSE_YES)
	do_graph_layout(view->g[view->activeGraph], layout, 0);
    gtk_object_destroy((GtkObject *) Dlg);*/
    return;


}

void mDotSlot(GtkWidget * widget, gpointer user_data)
{
    mSlot(widget, user_data, GVK_DOT, 1);
}

void mNeatoSlot(GtkWidget * widget, gpointer user_data)
{
    mSlot(widget, user_data, GVK_NEATO, 0);
}

void mTwopiSlot(GtkWidget * widget, gpointer user_data)
{
    mSlot(widget, user_data, GVK_TWOPI, 0);
}

void mCircoSlot(GtkWidget * widget, gpointer user_data)
{
    mSlot(widget, user_data, GVK_CIRCO, 0);
}

void mFdpSlot(GtkWidget * widget, gpointer user_data)
{
    mSlot(widget, user_data, GVK_FDP, 0);
}

void mSfdpSlot(GtkWidget * widget, gpointer user_data)
{
    mSlot(widget, user_data, GVK_SFDP, 0);
}



void mAbout(GtkWidget * widget, gpointer user_data)
{
}

void mHelp(GtkWidget * widget, gpointer user_data)
{
}



void change_cursor(GdkCursorType C)
{

    GdkCursor *cursor;
    GdkWindow *w;
    cursor = gdk_cursor_new(C);
    w = (GdkWindow *) glade_xml_get_widget(xml, "frmMain");
    gdk_window_set_cursor((GdkWindow *) view->drawing_area->window,
			  cursor);
    gdk_cursor_destroy(cursor);
}

#if OLD
static const char *endQuote(const char *args, agxbuf * xb, char endc)
{
    int more = 1;
    char c;

    while (more) {
	c = *args++;
	if (c == endc)
	    more = 0;
	else if (c == '\0') {
	    more = 0;
	    args--;
	} else if (c == '\\') {
	    c = *args++;
	    if (c == '\0')
		args--;
	    else
		agxbputc(xb, c);
	} else
	    agxbputc(xb, c);
    }
    return args;
}

static const char *skipWS(const char *args)
{
    char c;

    while ((c = *args)) {
	if (isspace(c))
	    args++;
	else
	    break;
    }
    return args;
}

static const char *getTok(const char *args, agxbuf * xb)
{
    char c;
    int more = 1;

    args = skipWS(args);
    if (*args == '\0')
	return 0;

    while (more) {
	c = *args++;
	if (isspace(c))
	    more = 0;
	else if (c == '\0') {
	    more = 0;
	    args--;
	} else if ((c == '"') || (c == '\'')) {
	    args = endQuote(args, xb, c);
	} else if (c == '\\') {
	    c = *args++;
	    if (c == '\0')
		args--;
	    else
		agxbputc(xb, c);
	} else
	    agxbputc(xb, c);
    }
    return args;
}

static char **splitArgs(const char *args, int *argcp)
{
    char **argv;
    int argc;
    int asize;
    agxbuf xbuf;
    unsigned char buf[SMALLBUF];

    if (*args == '\0') {
	*argcp = 0;
	return 0;
    }

    argc = 0;
    asize = 0;
    argv = 0;
    agxbinit(&xbuf, SMALLBUF, buf);
    while ((args = getTok(args, &xbuf))) {
	if (asize <= argc) {
	    asize += 10;
	    argv = ALLOC(asize, argv, char *);
	}
	argv[argc++] = strdup(agxbuse(&xbuf));
    }

    agxbfree(&xbuf);
    *argcp = argc;
    return argv;
}
#endif



void mTestgvpr(GtkWidget * widget, gpointer user_data)
{
    char *bf2;
    GtkTextBuffer *gtkbuf;
    GtkTextIter startit;
    GtkTextIter endit;
    const char *args;
    int i, j, argc, cloneGraph;
    char **argv;
#if OLD
    char **inargv;
    int inargc;
#endif

    args =
	gtk_entry_get_text((GtkEntry *)
			   glade_xml_get_widget(xml, "gvprargs"));
    gtkbuf =
	gtk_text_view_get_buffer((GtkTextView *)
				 glade_xml_get_widget(xml,
						      "gvprtextinput"));
    gtk_text_buffer_get_start_iter(gtkbuf, &startit);
    gtk_text_buffer_get_end_iter(gtkbuf, &endit);
    bf2 = gtk_text_buffer_get_text(gtkbuf, &startit, &endit, 0);

    if ((*args == '\0') && (*bf2 == '\0'))
	return;

#if OLD
    inargv = splitArgs(args, &inargc);

    argc = inargc + 1;
#else
    argc = 1;
    if (*args != '\0')
	argc += 2;
#endif
    if (*bf2 != '\0')
	argc++;
    if (gtk_toggle_button_get_active
	((GtkToggleButton *) glade_xml_get_widget(xml, "gvprapplycb"))) {
	cloneGraph = 1;
	argc++;
    } else
	cloneGraph = 0;
    argv = N_NEW(argc + 1, char *);
    j = 0;
    argv[j++] = "smyrna";
    if (cloneGraph)
	argv[j++] = strdup("-C");
#if OLD
    for (i = 0; i < inargc; i++)
	argv[j++] = inargv[i];
    free(inargv);
#else
    if (*args != '\0') {
	argv[j++] = strdup("-a");
	argv[j++] = strdup(args);
    }
#endif
    if (*bf2 != '\0') {
	argv[j++] = strdup(bf2);
	g_free(bf2);
    }
    assert(j == argc);


    run_gvpr(view->g[view->activeGraph], argc, argv);
    for (i = 1; i < argc; i++)
	free(argv[i]);
    free(argv);
}


/*
   opens a file open dialog and load a gvpr program to gvpr script text box
   if the current script is modified, user should be informed about it
*/
void on_gvprbuttonload_clicked(GtkWidget * widget, gpointer user_data)
{
    FILE *input_file = NULL;
    char *str;
    agxbuf xbuf;
    GtkTextBuffer *gtkbuf;	/*GTK buffer from glade GUI */

    char buf[BUFSIZ];
    unsigned char xbuffer[BUFSIZ];

    agxbinit(&xbuf, SMALLBUF, xbuffer);

    /*file name should be returned in xbuf */
    if (openfiledlg(0, NULL, &xbuf)) {
	input_file = fopen(agxbuse(&xbuf), "r");
	if (input_file) {
	    while (fgets(buf, BUFSIZ, input_file))
		agxbput(&xbuf, buf);
	    gtkbuf =
		gtk_text_view_get_buffer((GtkTextView *)
					 glade_xml_get_widget(xml,
							      "gvprtextinput"));
	    str = agxbuse(&xbuf);
	    if (g_utf8_validate(str, -1, NULL)) {
		gtk_text_buffer_set_text(gtkbuf, str, -1);
	    } else {
		show_gui_warning("File format is not UTF8!");
	    }
	    fclose(input_file);
	} else {
	    show_gui_warning("file couldn't be opened\n");
	}
    }
    agxbfree(&xbuf);
}

/*
	opens a file open dialog and load a gvpr program to gvpr script text box
	if the current script is modified, user should be informed about it
*/
void on_gvprbuttonsave_clicked(GtkWidget * widget, gpointer user_data)
{
    FILE *output_file = NULL;
    agxbuf xbuf;
    GtkTextBuffer *gtkbuf;	/*GTK buffer from glade GUI */
    int charcnt;
    char *bf2;
    GtkTextIter startit;
    GtkTextIter endit;



    agxbinit(&xbuf, SMALLBUF, NULL);
    /*file name should be returned in xbuf */
    if (savefiledlg(0, NULL, &xbuf)) {
	output_file = fopen(agxbuse(&xbuf), "w");
	if (output_file) {
	    gtkbuf =
		gtk_text_view_get_buffer((GtkTextView *)
					 glade_xml_get_widget(xml,
							      "gvprtextinput"));
	    charcnt = gtk_text_buffer_get_char_count(gtkbuf);
	    gtk_text_buffer_get_start_iter(gtkbuf, &startit);
	    gtk_text_buffer_get_end_iter(gtkbuf, &endit);
	    bf2 = gtk_text_buffer_get_text(gtkbuf, &startit, &endit, 0);
	    fprintf(output_file, "%s", bf2);
	    fclose(output_file);

	}


	/*Code has not been completed for this function yet */
    }


}
