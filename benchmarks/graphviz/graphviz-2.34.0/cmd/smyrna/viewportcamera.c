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

#include "viewportcamera.h"
#include "gui.h"
#include "math.h"
#include "memory.h"
#include "glcompbutton.h"
#include "glcomplabel.h"
#include "glcomppanel.h"


static viewport_camera *new_viewport_camera(ViewInfo * view)
{
    return NEW(viewport_camera);
}

static void viewport_update_camera_indexes(ViewInfo * view)
{
    int ind = 0;
    for (ind = 0; ind < view->camera_count; ind++) {
	view->cameras[ind]->index = ind;
    }
}

static viewport_camera *add_camera_to_viewport(ViewInfo * view)
{
    view->camera_count++;
    view->cameras =
	RALLOC(view->camera_count, view->cameras, viewport_camera *);
    view->cameras[view->camera_count - 1] = new_viewport_camera(view);
    view->active_camera = view->camera_count - 1;
    viewport_update_camera_indexes(view);
    return view->cameras[view->camera_count - 1];
}

#if 0
static void set_camera_x_y(viewport_camera * c)
{
/*    c->x =
	c->r * cos((float) DEG2RAD * c->anglexy) * sin((float) DEG2RAD *
						       c->anglexyz) +
	view->panx;
    c->y =
	c->r * sin(DEG2RAD * c->anglexy) * sin(DEG2RAD * c->anglexyz) +
	view->pany;
    c->z = c->r * cos(DEG2RAD * c->anglexyz);*/
}
int delete_camera_from_viewport(ViewInfo * view, viewport_camera * c)
{
    int ind;
    int found = 0;
    for (ind = 0; ind < view->camera_count; ind++) {
	if ((view->cameras[ind] == c) && found == 0)
	    found = 1;
	if ((found == 1) && (ind < (view->camera_count - 1)))
	    view->cameras[ind] = view->cameras[ind + 1];
    }
    if (found) {
	free(c);
	view->camera_count--;
	view->cameras =
	    RALLOC(view->camera_count, view->cameras, viewport_camera *);
	viewport_update_camera_indexes(view);
	view->active_camera--;
	return 1;
    }
    return 0;
}
#endif
#if 0
int activate_viewport_camera(ViewInfo * view, int cam_index)
{
    if (cam_index < view->camera_count) {
	view->active_camera = cam_index;
	return 1;
    } else
	return 0;
}
int refresh_viewport_camera(ViewInfo * view)
{
    if (view->active_camera >= 0) {

/*		view->panx=view->cameras[view->active_camera]->panx;
		view->pany=view->cameras[view->active_camera]->pany;
		view->panz=view->cameras[view->active_camera]->panz;
		view->zoom=view->cameras[view->active_camera]->zoom;*/
	return 1;
    } else
	return 0;
}
#endif

void menu_click_add_camera(void *p)
{
    viewport_camera *c;
    /*add test cameras */
    c = add_camera_to_viewport(view);
    c->targetx = view->panx;
    c->targety = view->pany;
    c->targetz = view->panz;
    c->x = view->panx;
    c->y = view->pany;
    c->z = view->zoom;
    c->camera_vectorx = 0;
    c->camera_vectory = 1;
    c->camera_vectorz = 0;

    c->anglexy = 90;
    c->anglexyz = 0;
    c->anglex = 0;
    c->angley = 0;
    c->anglez = 0;

    c->r = view->zoom * -1;
//      set_camera_x_y(c);
//    attach_camera_widget(view);
}

#ifdef UNUSED
static int blocksignal = 0;
static void menu_click_2d(void *p)
{
    view->active_camera = -1;
}

static void menu_click_camera_select(void *p)
{
    view->active_camera = ((glCompButton *) p)->data;
}

static int show_camera_settings(viewport_camera * c)
{

    char buf[50];
    sprintf(buf, "Camera:%i", c->index);
    gtk_label_set_text((GtkLabel *)
		       glade_xml_get_widget(xml, "dlgcameralabel1"), buf);
    gtk_spin_button_set_value((GtkSpinButton *)
			      glade_xml_get_widget(xml,
						   "dlgcameraspinbutton1"),
			      c->x);
    gtk_spin_button_set_value((GtkSpinButton *)
			      glade_xml_get_widget(xml,
						   "dlgcameraspinbutton2"),
			      c->y);
    gtk_spin_button_set_value((GtkSpinButton *)
			      glade_xml_get_widget(xml,
						   "dlgcameraspinbutton3"),
			      c->z);
    gtk_spin_button_set_value((GtkSpinButton *)
			      glade_xml_get_widget(xml,
						   "dlgcameraspinbutton4"),
			      c->targetx);
    gtk_spin_button_set_value((GtkSpinButton *)
			      glade_xml_get_widget(xml,
						   "dlgcameraspinbutton5"),
			      c->targety);
    gtk_spin_button_set_value((GtkSpinButton *)
			      glade_xml_get_widget(xml,
						   "dlgcameraspinbutton6"),
			      c->targetz);


    gtk_spin_button_set_value((GtkSpinButton *)
			      glade_xml_get_widget(xml,
						   "dlgcameraspinbutton7"),
			      c->camera_vectorx * 360.0);
    gtk_spin_button_set_value((GtkSpinButton *)
			      glade_xml_get_widget(xml,
						   "dlgcameraspinbutton8"),
			      c->camera_vectory * 360.0);
    gtk_spin_button_set_value((GtkSpinButton *)
			      glade_xml_get_widget(xml,
						   "dlgcameraspinbutton9"),
			      c->camera_vectorz * 360.0);





    gtk_widget_hide(glade_xml_get_widget(xml, "dlgCamera"));
    gtk_widget_show(glade_xml_get_widget(xml, "dlgCamera"));
    gtk_window_set_keep_above((GtkWindow *)
			      glade_xml_get_widget(xml, "dlgCamera"), 1);
    view->selected_camera = c;
    return 1;


}

static void menu_click_camera_edit(void *p)
{
    show_camera_settings(view->cameras[(int) ((glCompButton *) p)->data]
	);
}

void attach_camera_widget(ViewInfo * view)
{

#define PANEL_PADDING			5
#define	CAMERA_BUTTON_HEIGHT	25
#define	CAMERA_BUTTON_WIDTH		75


    int ind, ind2;
    GLfloat x, y;
    char buf[256];
    glCompPanel *p;
    glCompButton *b;
    glCompSet *s = view->widgets;
    int p_height;
    /*first we need to get rid of the old menu */
    for (ind = 0; ind < s->panelcount; ind++) {
	if (s->panels[ind]->data == 3) {
	    /*remove buttons in the panel */
	    for (ind2 = 0; ind2 < s->buttoncount; ind2++) {
		if (s->buttons[ind2]->panel == s->panels[ind]) {
		    glCompSetRemoveButton(s, s->buttons[ind2]);
		    ind2--;
		}
	    }
	    /*remove panel itself */
	    glCompSetRemovePanel(s, s->panels[ind]);
	    break;
	}
    }




    /*calculate height of panel */
    p_height =
	2 * PANEL_PADDING + view->camera_count * (CAMERA_BUTTON_HEIGHT +
						  PANEL_PADDING) +
	CAMERA_BUTTON_HEIGHT;


    /*container for camera buttons */
    p = glCompPanelNew((GLfloat) 25, (GLfloat) 75,
		       (GLfloat) 4 * PANEL_PADDING +
		       3 * CAMERA_BUTTON_WIDTH, (GLfloat) p_height,
		       scientific_y);
    p->data = 3;
    glCompSetAddPanel(s, p);

    b = glCompButtonNew((GLfloat) PANEL_PADDING, (GLfloat) PANEL_PADDING,
			(GLfloat) CAMERA_BUTTON_WIDTH,
			(GLfloat) CAMERA_BUTTON_HEIGHT, "ADD", '\0', 0, 0,
			scientific_y);
    b->panel = p;
    b->groupid = 0;
    b->customptr = view;
    glCompSetAddButton(s, b);
    b->callbackfunc = menu_click_add_camera;

    b = glCompButtonNew((GLfloat) PANEL_PADDING * 2 +
			(GLfloat) CAMERA_BUTTON_WIDTH,
			(GLfloat) PANEL_PADDING,
			(GLfloat) CAMERA_BUTTON_WIDTH,
			(GLfloat) CAMERA_BUTTON_HEIGHT, "2D", '\0', 0, 0,
			scientific_y);
    b->panel = p;
    b->groupid = 4;		//4 is assigned to all camera buttons 
    b->customptr = view;
    glCompSetAddButton(s, b);
    b->callbackfunc = menu_click_2d;

    for (ind = 0; ind < view->camera_count; ind++) {
	y = p->height - ((GLfloat) PANEL_PADDING +
			 (GLfloat) ind * ((GLfloat) CAMERA_BUTTON_HEIGHT +
					  (GLfloat) PANEL_PADDING)) -
	    (GLfloat) CAMERA_BUTTON_HEIGHT;
	x = PANEL_PADDING;
	sprintf(buf, "CAM%i", ind + 1);
	b = glCompButtonNew((GLfloat) x, (GLfloat) y,
			    (GLfloat) CAMERA_BUTTON_WIDTH,
			    (GLfloat) CAMERA_BUTTON_HEIGHT, buf, '\0', 0,
			    0, scientific_y);
	b->panel = p;
	b->groupid = 4;		//4 is assigned to all camera buttons 
	b->data = ind;		//assign camera id to custom data to use single call back
	b->customptr = view;
	glCompSetAddButton(s, b);
	b->callbackfunc = menu_click_camera_select;

	x = PANEL_PADDING * 2 + CAMERA_BUTTON_WIDTH;
	b = glCompButtonNew((GLfloat) x, (GLfloat) y,
			    (GLfloat) CAMERA_BUTTON_WIDTH,
			    (GLfloat) CAMERA_BUTTON_HEIGHT, "Remove", '\0',
			    0, 0, scientific_y);
	b->panel = p;
	b->groupid = 0;
	b->data = ind;		//assign camera id to custom data to use single call back
	b->customptr = view;
	glCompSetAddButton(s, b);
	b->callbackfunc = menu_click_camera_remove;

	x = PANEL_PADDING * 3 + CAMERA_BUTTON_WIDTH * 2;
	b = glCompButtonNew((GLfloat) x, (GLfloat) y,
			    (GLfloat) CAMERA_BUTTON_WIDTH,
			    (GLfloat) CAMERA_BUTTON_HEIGHT, "Edit", '\0',
			    0, 0, scientific_y);
	b->panel = p;
	b->groupid = 0;
	b->data = ind;		//assign camera id to custom data to use single call back
	b->customptr = view;
	glCompSetAddButton(s, b);
	b->callbackfunc = menu_click_camera_edit;
    }
}
#endif
#if 0
int save_camera_settings(viewport_camera * c)
{
    c->x = (float) gtk_spin_button_get_value((GtkSpinButton *)
					     glade_xml_get_widget(xml,
								  "dlgcameraspinbutton1"));
    c->y = (float) gtk_spin_button_get_value((GtkSpinButton *)
					     glade_xml_get_widget(xml,
								  "dlgcameraspinbutton2"));
    c->z = (float) gtk_spin_button_get_value((GtkSpinButton *)
					     glade_xml_get_widget(xml,
								  "dlgcameraspinbutton3"));
    c->targetx = (float) gtk_spin_button_get_value((GtkSpinButton *)
						   glade_xml_get_widget
						   (xml,
						    "dlgcameraspinbutton4"));
    c->targety = (float) gtk_spin_button_get_value((GtkSpinButton *)
						   glade_xml_get_widget
						   (xml,
						    "dlgcameraspinbutton5"));
    c->targetz = (float) gtk_spin_button_get_value((GtkSpinButton *)
						   glade_xml_get_widget
						   (xml,
						    "dlgcameraspinbutton6"));

    c->camera_vectorx = (float) gtk_spin_button_get_value((GtkSpinButton *)
							  glade_xml_get_widget
							  (xml,
							   "dlgcameraspinbutton7"))
	/ 360;
    c->camera_vectory = (float) gtk_spin_button_get_value((GtkSpinButton *)
							  glade_xml_get_widget
							  (xml,
							   "dlgcameraspinbutton8"))
	/ 360;
    c->camera_vectorz = (float) gtk_spin_button_get_value((GtkSpinButton *)
							  glade_xml_get_widget
							  (xml,
							   "dlgcameraspinbutton9"))
	/ 360;


    return 1;

}

void dlgcameraokbutton_clicked_cb(GtkWidget * widget, gpointer user_data)
{
    gtk_widget_hide(glade_xml_get_widget(xml, "dlgCamera"));
    save_camera_settings(view->selected_camera);
}

void dlgcameracancelbutton_clicked_cb
    (GtkWidget * widget, gpointer user_data) {
    gtk_widget_hide(glade_xml_get_widget(xml, "dlgCamera"));
}
#endif
