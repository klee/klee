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
#ifndef GLCOMPDEFS_H
#define GLCOMPDEFS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>
#ifdef _WIN32
#include <windows.h>
#include <winuser.h>
#include <tchar.h>
#endif
#include <GL/gl.h>
#include <stdlib.h>
#include <stdio.h>
#include <GL/gl.h>
#include <GL/glu.h>
#ifdef WIN32
#define strdup _strdup
#endif

#ifdef __cplusplus
extern "C" {
#endif


#define	GLCOMPSET_PANEL_COLOR_R		(GLfloat)0.16
#define	GLCOMPSET_PANEL_COLOR_G		(GLfloat)0.44
#define	GLCOMPSET_PANEL_COLOR_B		(GLfloat)0.87
#define	GLCOMPSET_PANEL_COLOR_ALPHA	(GLfloat)0.5
#define	GLCOMPSET_PANEL_SHADOW_COLOR_R		(GLfloat)0
#define	GLCOMPSET_PANEL_SHADOW_COLOR_G		(GLfloat)0
#define	GLCOMPSET_PANEL_SHADOW_COLOR_B		(GLfloat)0
#define	GLCOMPSET_PANEL_SHADOW_COLOR_A		(GLfloat)0.3
#define GLCOMPSET_PANEL_SHADOW_WIDTH		(GLfloat)4

#define	GLCOMPSET_BUTTON_COLOR_R		(GLfloat)0
#define	GLCOMPSET_BUTTON_COLOR_G		(GLfloat)1
#define	GLCOMPSET_BUTTON_COLOR_B		(GLfloat)0.3
#define	GLCOMPSET_BUTTON_COLOR_ALPHA	(GLfloat)0.6
#define	GLCOMPSET_BUTTON_THICKNESS		(GLfloat)3
#define	GLCOMPSET_BUTTON_BEVEL_BRIGHTNESS		(GLfloat)1.7
#define GLCOMPSET_FONT_SIZE				(GLfloat)14

#define	GLCOMPSET_BUTTON_FONT_COLOR_R		(GLfloat)0
#define	GLCOMPSET_BUTTON_FONT_COLOR_G		(GLfloat)0
#define	GLCOMPSET_BUTTON_FONT_COLOR_B		(GLfloat)0
#define	GLCOMPSET_BUTTON_FONT_COLOR_ALPHA	(GLfloat)1

#define GLCOMPSET_FONT_SIZE_FACTOR			(GLfloat)0.7

#define	GLCOMPSET_LABEL_COLOR_R		(GLfloat)0
#define	GLCOMPSET_LABEL_COLOR_G		(GLfloat)0
#define	GLCOMPSET_LABEL_COLOR_B		(GLfloat)0
#define	GLCOMPSET_LABEL_COLOR_ALPHA	(GLfloat)1

#define	GLCOMPSET_FONT_COLOR_R		(GLfloat)0
#define	GLCOMPSET_FONT_COLOR_G		(GLfloat)0
#define	GLCOMPSET_FONT_COLOR_B		(GLfloat)0
#define	GLCOMPSET_FONT_COLOR_ALPHA	(GLfloat)1
#define GLCOMPSET_FONT_DESC  "Times Italic"
#define GL_FONTOPTIMIZE 1


#define GL_FONTVJUSTIFY	0
#define GL_FONTHJUSTIFY	0


#define DEFAULT_GLUT_FONT GLUT_BITMAP_HELVETICA_12

#define GLCOMPSET_BORDERWIDTH				(GLfloat)2
#define GLCOMPSET_PANEL_BORDERWIDTH				(GLfloat)3
#define GLCOMPSET_BUTTON_BEVEL				(GLfloat)5
#define	GLCOMPSET_BEVEL_DIFF				(GLfloat)0.001
#define GLCOMPSET_DEFAULT_PAD		(GLfloat)3
#define	GLCOMP_DEFAULT_WIDTH	(GLfloat)10
#define	GLCOMP_DEFAULT_HEIGHT	(GLfloat)10



#define FONT_MAX_LEN                1024	/* maximum chars to draw to the screen, used for buffers also */
#define FONT_TAB_SPACE              4	/* spaces to draw for a tab, make option? */

#define C_DPI              16
#define R_DPI              16

    typedef enum { inverted_y, scientific_y } glCompOrientation;
    typedef enum { gluttext, pangotext } glCompFontType;
    typedef enum { glAlignNone, glAlignLeft, glAlignTop, glAlignBottom,
	    glAlignRight, glAlignParent, glAlignCenter } glCompAlignment;

    typedef enum { glFontVJustifyNone, glFontVJustifyTop,
	    glFontVJustifyBottom, glFontVJustifyCenter } glCompVJustify;
    typedef enum { glFontHJustifyNone, glFontHJustifyLeft,
	    glFontHJustifyRight, glFontHJustifyCenter } glCompHJustify;
    typedef enum { glButtonGlyphLeft, glButtonGlyphRight, glButtonGlyphTop,
	    glButtonGlyphBottom } glCompButtonGlyph;
    typedef enum { glBorderNone, glBorderSolid, glBorderBevel,
	    glBorderCustom } glCompBorderType;

    typedef enum { glMouseDown, glMouseUp } glCompMouseStatus;
    typedef enum { glMouseLeftButton, glMouseRightButton,
	    glMouseMiddleButton } glMouseButtonType;

    typedef enum { glTexImage, glTexLabel } glCompTexType;
    typedef enum { glPanelObj, glButtonObj, glLabelObj,
	    glImageObj } glObjType;

    typedef struct _glCompButton glCompButton;
    typedef struct _glCompObj glCompObj;

/*call backs for widgets*/
    typedef void (*glcompdrawfunc_t) (void *obj);
    typedef void (*glcompclickfunc_t) (glCompObj * obj, GLfloat x,
				       GLfloat y, glMouseButtonType t);
    typedef void (*glcompdoubleclickfunc_t) (glCompObj * obj, GLfloat x,
					     GLfloat y,
					     glMouseButtonType t);
    typedef void (*glcompmouseoverfunc_t) (glCompObj * obj, GLfloat x,
					   GLfloat y);
    typedef void (*glcompmouseinfunc_t) (glCompObj * obj, GLfloat x,
					 GLfloat y);
    typedef void (*glcompmouseoutfunc_t) (glCompObj * obj, GLfloat x,
					  GLfloat y);
    typedef void (*glcompmousedownfunc_t) (glCompObj * obj, GLfloat x,
					   GLfloat y, glMouseButtonType t);
    typedef void (*glcompmouseupfunc_t) (glCompObj * obj, GLfloat x,
					 GLfloat y, glMouseButtonType t);
    typedef void (*glcompmousedragfunct_t) (glCompObj * obj, GLfloat dx,
					    GLfloat dy,
					    glMouseButtonType t);



    typedef struct _glCompAnchor {

	int topAnchor;		/*anchor booleans */
	int leftAnchor;
	int rightAnchor;
	int bottomAnchor;

	GLfloat top;		/*anchor values */
	GLfloat left;
	GLfloat right;
	GLfloat bottom;


    } glCompAnchor;

    typedef struct _glCompJustify {
	glCompVJustify VJustify;
	glCompHJustify HJustify;
    } glCompJustify;




    typedef struct _glCompPoint {
	GLfloat x, y, z;
    } glCompPoint;

    typedef struct _glCompPointI {
	int x, y;
    } glCompPointI;
    typedef struct {
	int cnt;
	int hotKey;
	glCompPoint* pts;
    }glCompPoly;

    typedef struct {
	GLfloat R;
	GLfloat G;
	GLfloat B;
	GLfloat A;		//Alpha
	int tag;
	int test;
    } glCompColor;


    typedef struct _glCompRect {
	glCompPoint pos;
	GLfloat w;
	GLfloat h;
    } glCompRect;

    typedef struct _glCompTex {
	GLuint id;
	char *def;
	char *text;
	float width;
	float height;
	glCompTexType type;
	int userCount;
	int fontSize;
	unsigned char *data;	/*data */
    } glCompTex;



/*opengl font*/
    typedef struct {
	char *fontdesc;		//font description , only used with pango fonts
	glCompColor color;
	glCompFontType type;
	void *glutfont;		/*glut font pointer if used */
	int transparent;
	glCompTex *tex;		/* texture, if type is pangotext */
	int size;
	int reference;		/*if font has references to parent */
	glCompJustify justify;
	int is2D;
	int optimize;
    } glCompFont;

    typedef struct _glCompCallBacks {
	glcompdrawfunc_t draw;
	glcompclickfunc_t click;
	glcompdoubleclickfunc_t doubleclick;
	glcompmouseoverfunc_t mouseover;
	glcompmouseinfunc_t mousein;
	glcompmouseoutfunc_t mouseout;
	glcompmousedownfunc_t mousedown;
	glcompmouseupfunc_t mouseup;
	glcompmousedragfunct_t mousedrag;

    } glCompCallBacks;


/*
	common widget properties
	also each widget has pointer to its parents common
*/
    typedef struct _glCompCommon {
	glCompPoint pos;
	glCompPoint refPos;	/*calculated pos after anchors and aligns */
	GLfloat width, height;
	GLfloat borderWidth;
	glCompBorderType borderType;
	glCompColor color;
	int enabled;
	int visible;
	void *compset;		// compset
	void *parent;		/*parent widget */
	int data;
	glCompFont *font;	//pointer to font to use
	glCompAlignment align;
	glCompAnchor anchor;
	int layer;		/*keep track of object order, what to draw on top */
	glCompCallBacks callbacks;
	glCompCallBacks functions;
	glCompJustify justify;
    } glCompCommon;

/*generic image*/
    typedef struct _glCompImage {
	glObjType objType;	/*always keep this here for each drawable object */
	glCompCommon common;
	glCompTex *texture;
	GLfloat width, height;  /* width and height in world coords */
	/* char *pngFile; */
	int stretch;
    } glCompImage;

/*generic panel*/
    typedef struct _glCompPanel {
	glObjType objType;	/*always keep this here for each drawable object */
	glCompCommon common;
	GLfloat shadowwidth;
	glCompColor shadowcolor;
	char *text;
	glCompImage *image;
    } glCompPanel;

/*label*/
    typedef struct _glCompLabel {
	glObjType objType;	/*always keep this here for each drawable object */
	glCompCommon common;
	int autosize;		/*if 1 label sized is calculated from font */
	char *text;
	int transparent;
    } glCompLabel;

/*buttons*/
    struct _glCompButton {
	glObjType objType;	/*always keep this here for each drawable object */
	glCompCommon common;
	GLfloat width, height;
	glCompLabel *label;
	int status;		//0 not pressed 1 pressed;
	int refStatus;		//0 not pressed 1 pressed;
	int groupid;
	glCompImage *image;	/*glyph */
	glCompButtonGlyph glyphPos;
	void *customptr;	//general purpose void pointer to pass to call back
	int data;

    };

/*texture based image*/

/*track bar*/
    typedef struct _glCompTrackBar {
	glObjType objType;	/*always keep this here for each drawable object */
	GLfloat width, height;
	glCompPanel *outerpanel;
	glCompPanel *trackline;
	glCompPanel *indicator;

	GLfloat bevel;
	glCompColor color;
	glCompColor shadowcolor;


	float value;
	float maxvalue;
	float minvalue;
	int enabled;
	int visible;
	void *parentset;	//parent compset
	int data;
	glCompFont *font;	//pointer to font to use
	glCompOrientation orientation;

    } glCompTrackBar;

/*glCompFont container class*/
    typedef struct {
	glCompFont **fonts;
	int count;
	int activefont;
	char *font_directory;	//location where the glfont files are stored
    } fontset_t;

/*object prototype*/
    struct _glCompObj {
	glObjType objType;
	glCompCommon common;
    };

    typedef struct _glCompMouse {
	glCompMouseStatus status;
	glMouseButtonType t;
	glCompPoint initPos; /*current mouse pos,*/
	glCompPoint pos; /*current mouse pos,*/
	glCompPoint finalPos; /*current mouse pos,*/
	glCompPoint GLpos;/*3d converted opengl position*/
	glCompPoint GLinitPos;/*mouse button down pos*/
	glCompPoint GLfinalPos;/*mouse button up pos*/

	GLfloat dragX, dragY;/*GLpos - GLinitpos*/
	glCompObj *clickedObj;
	glCompCallBacks callbacks;
	glCompCallBacks functions;
	int down;


    } glCompMouse;



/*main widget set manager*/
    typedef struct {
	glObjType objType;	/*always keep this here for each drawable object */
	glCompCommon common;

	glCompObj **obj;
	int objcnt;
	glCompPanel **panels;
	glCompButton **buttons;
	glCompLabel **labels;
	int groupCount;		/*group id counter */
	int active;		//0 dont draw, 1 draw
	int enabled;		//0 disabled 1 enabled(allow mouse interaction)
	GLfloat clickedX, clickedY;

	int textureCount;
	glCompTex **textures;
	glCompMouse mouse;
    } glCompSet;

#ifdef __cplusplus
}
#endif
#endif
