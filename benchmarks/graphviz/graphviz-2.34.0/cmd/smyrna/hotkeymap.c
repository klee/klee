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

#include "hotkeymap.h"

int static get_mouse_mode(const char *s)
{
    if (strcmp(s, "MM_PAN") == 0)
	return MM_PAN;
    if (strcmp(s, "MM_ZOOM") == 0)
	return MM_ZOOM;
    if (strcmp(s, "MM_ROTATE") == 0)
	return MM_ROTATE;
    if (strcmp(s, "MM_SINGLE_SELECT") == 0)
	return MM_SINGLE_SELECT;
    if (strcmp(s, "MM_RECTANGULAR_SELECT") == 0)
	return MM_RECTANGULAR_SELECT;
    if (strcmp(s, "MM_RECTANGULAR_X_SELECT") == 0)
	return MM_RECTANGULAR_X_SELECT;

    if (strcmp(s, "MM_POLYGON_SELECT") == 0)
	return MM_POLYGON_SELECT;

    if (strcmp(s, "MM_MOVE") == 0)
	return MM_MOVE;
    if (strcmp(s, "MM_MOVE") == 0)
	return MM_MOVE;
    if (strcmp(s, "MM_MAGNIFIER") == 0)
	return MM_MAGNIFIER;
    if (strcmp(s, "MM_FISHEYE_MAGNIFIER") == 0)
	return MM_FISHEYE_MAGNIFIER;
    if (strcmp(s, "MM_FISHEYE_PICK") == 0)
	return MM_FISHEYE_PICK;


    return -1;

}


int static get_button(const char *s)
{
    if(view->guiMode != GUI_FULLSCREEN)
    {
	if (strcmp(s, "B_LSHIFT") == 0)
	    return B_LSHIFT;
	if (strcmp(s, "B_RSHIFT") == 0)
	    return B_RSHIFT;
	if (strcmp(s, "B_LCTRL") == 0)
	    return B_LCTRL;
	if (strcmp(s, "B_RCTRL") == 0)
	    return B_RCTRL;
	if (strcmp(s, "0") == 0)
	    return 0;
    }
    else
    {
		int mod = glutGetModifiers();
		if (mod == GLUT_ACTIVE_ALT)

	if (strcmp(s, "B_LSHIFT") == 0)
	    return GLUT_ACTIVE_SHIFT;
	if (strcmp(s, "B_RSHIFT") == 0)
	    return GLUT_ACTIVE_SHIFT;
	if (strcmp(s, "B_LCTRL") == 0)
	    return GLUT_ACTIVE_CTRL;
	if (strcmp(s, "B_RCTRL") == 0)
	    return GLUT_ACTIVE_CTRL;
	if (strcmp(s, "0") == 0)
	    return 0;
    }
    return 0;

}
int static get_view_mode(const char *s)
{
    if (strcmp(s, "ALL") == 0)
	return smyrna_all;
    if (strcmp(s, "2D") == 0)
	return smyrna_2D;
    if (strcmp(s, "3D") == 0)
	return smyrna_3D;
    if (strcmp(s, "FISHEYE") == 0)
	return smyrna_fisheye;
    if (strcmp(s, "NO_FISHEYE") == 0)
	return smyrna_all_but_fisheye;
    return -1;
}
int static get_mouse_button(const char *s)
{

    if (strcmp(s, "LEFT") == 0)
	return glMouseLeftButton;
    if (strcmp(s, "RIGHT") == 0)
	return glMouseRightButton;
    if (strcmp(s, "MIDDLE") == 0)
	return glMouseMiddleButton;
    return -1;
}
int static get_drag(const char *s)
{
    if (s[0] == '1')
	return 1;
    return 0;
}



void load_mouse_actions(char *modefile, ViewInfo * v)
{
/*#define MM_PAN					0
#define MM_ZOOM					1
#define MM_ROTATE				2
#define MM_SINGLE_SELECT		3
#define MM_RECTANGULAR_SELECT	4
#define MM_RECTANGULAR_X_SELECT	5
#define MM_MOVE					10
#define MM_MAGNIFIER			20
#define MM_FISHEYE_MAGNIFIER	21*/
    /*file parsing is temporarrily not available */
    int i = 0;
    FILE *file;
    char line[BUFSIZ];
    char *a;
    char *action_file = smyrnaPath("mouse_actions.txt");
    file = fopen(action_file, "r");
    if (file != NULL) {
	int ind = 0;
	while (fgets(line, BUFSIZ, file) != NULL) {
	    int idx = 0;
	    a = strtok(line, ",");

	    if ((line[0] == '#') || (line[0] == ' ')
		|| (strlen(line) == 0))
		continue;

	    v->mouse_action_count++;
	    v->mouse_actions =
		realloc(v->mouse_actions,
			v->mouse_action_count * sizeof(mouse_action_t));
	    v->mouse_actions[ind].action = get_mouse_mode(a);
	    v->mouse_actions[ind].index = i;

	    while ((a = strtok(NULL, ","))) {
		//#Action(0),hotkey(1),view_mode(2),mouse_button(3),drag(4)
		switch (idx) {
		case 0:
		    v->mouse_actions[ind].hotkey = get_button(a);
		    break;
		case 1:
		    v->mouse_actions[ind].mode = get_view_mode(a);
		    break;
		case 2:
		    v->mouse_actions[ind].type = get_mouse_button(a);
		    break;
		case 3:
		    v->mouse_actions[ind].drag = get_drag(a);
		    break;

		}
		idx++;
	    }
	    ind++;
	}
	fclose(file);
    }
    free(action_file);


/*
    v->mouse_action_count=7;
    v->mouse_actions=realloc(v->mouse_actions,v->mouse_action_count * sizeof(mouse_action_t));
    v->mouse_actions[ind].action=MM_PAN;
    v->mouse_actions[ind].drag=1;
    v->mouse_actions[ind].hotkey=0;
    v->mouse_actions[ind].index=ind;
    v->mouse_actions[ind].mode=smyrna_all;
    v->mouse_actions[ind].type=glMouseLeftButton;

    ind++;

    v->mouse_actions[ind].action=MM_ROTATE;
    v->mouse_actions[ind].drag=1;
    v->mouse_actions[ind].hotkey=B_LSHIFT;
    v->mouse_actions[ind].index=ind;
    v->mouse_actions[ind].mode=smyrna_3D;
    v->mouse_actions[ind].type=glMouseLeftButton;

    ind++;

    v->mouse_actions[ind].action=MM_SINGLE_SELECT;
    v->mouse_actions[ind].drag=0;
    v->mouse_actions[ind].hotkey=0;
    v->mouse_actions[ind].index=ind;
    v->mouse_actions[ind].mode=smyrna_all_but_fisheye;
    v->mouse_actions[ind].type=glMouseLeftButton;

    ind++;

    v->mouse_actions[ind].action=MM_RECTANGULAR_SELECT;
    v->mouse_actions[ind].drag=1;
    v->mouse_actions[ind].hotkey=0;
    v->mouse_actions[ind].index=ind;
    v->mouse_actions[ind].mode=smyrna_all;
    v->mouse_actions[ind].type=glMouseRightButton;

    ind++;

    v->mouse_actions[ind].action=MM_MOVE;
    v->mouse_actions[ind].drag=1;
    v->mouse_actions[ind].hotkey=B_LCTRL;
    v->mouse_actions[ind].index=ind;
    v->mouse_actions[ind].mode=smyrna_2D;
    v->mouse_actions[ind].type=glMouseLeftButton;

    ind++;

    v->mouse_actions[ind].action=MM_FISHEYE_MAGNIFIER;
    v->mouse_actions[ind].drag=1;
    v->mouse_actions[ind].hotkey=B_LSHIFT;
    v->mouse_actions[ind].index=ind;
    v->mouse_actions[ind].mode=smyrna_2D;
    v->mouse_actions[ind].type=glMouseLeftButton;

    ind++;

    v->mouse_actions[ind].action=MM_FISHEYE_PICK;
    v->mouse_actions[ind].drag=0;
    v->mouse_actions[ind].hotkey=0;
    v->mouse_actions[ind].index=ind;
    v->mouse_actions[ind].mode=smyrna_fisheye;
    v->mouse_actions[ind].type=glMouseRightButton;
*/
}

int get_key_action(ViewInfo * v, int key)
{
    int ind = 0;
    for (; ind < v->mouse_action_count; ind++) {

	if (v->mouse_actions[ind].hotkey == key)
	    return v->mouse_actions[ind].action;
    }
    return -1;
}


int get_mode(ViewInfo * v)
{


/*#define F_BUTTON1   B_LSHIFT
#define F_BUTTON2   B_RSHIFT
#define F_BUTTON3   B_LCTRL
#define F_BUTTON4   B_LCTRL

#define MOUSE_BUTTON_1	LEFT_MOUSE_BUTTON
#define MOUSE_BUTTON_2  RIGHT_MOUSE_BUTTON
#define MOUSE_BUTTON_3	MIDDLE_MOUSE_BUTTON*/
//typedef enum {smyrna_2D,smyrna_3D,smyrna_fisheye} smyrna_view_mode;
    int ind = 0;
    glMouseButtonType curMouseType = v->mouse.t;
    int curDragging = ((v->mouse.dragX != 0) || (v->mouse.dragY != 0));
    smyrna_view_mode view_mode;
    view_mode = smyrna_2D;
    if (v->active_camera >= 0)
	view_mode = smyrna_3D;
    if (v->Topview->fisheyeParams.active)
	view_mode = smyrna_fisheye;


    for (; ind < v->mouse_action_count; ind++) {

	if ((v->mouse_actions[ind].hotkey == v->keymap.keyVal)
	    && (v->mouse_actions[ind].type == curMouseType)
	    && (v->mouse_actions[ind].drag == curDragging)
	    &&
	    ((v->mouse_actions[ind].mode == view_mode)
	     || (v->mouse_actions[ind].mode == smyrna_all)
	     || ((v->mouse_actions[ind].mode == smyrna_all_but_fisheye)
		 && (view_mode != smyrna_fisheye))
	    )) {
	    return v->mouse_actions[ind].action;

	}
    }
    return -1;





/*    if ((view->mouse.t==MOUSE_BUTTON_1)&&(view->keymap.down) && (view->keymap.keyVal ==F_BUTTON1) && (view->active_camera==-1))
	return MM_FISHEYE_MAGNIFIER;
    if ((view->mouse.t==MOUSE_BUTTON_1)&&(view->keymap.down) && (view->keymap.keyVal == F_BUTTON1) && (view->active_camera>-1))
	return MM_ROTATE;
    if ((view->mouse.t==MOUSE_BUTTON_1)&&(view->keymap.down) && (view->keymap.keyVal == F_BUTTON3)) 
	return MM_MOVE;
    if ((view->mouse.t==MOUSE_BUTTON_1)&&(view->mouse.down) ) 
	return MM_PAN;
    if ((view->mouse.t==MOUSE_BUTTON_2)&&(view->mouse.down) ) 
	return MM_RECTANGULAR_SELECT;*/



}
