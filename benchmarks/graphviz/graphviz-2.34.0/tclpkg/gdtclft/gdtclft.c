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


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <tcl.h>
#include <unistd.h>
#include "gd.h"
#include "tclhandle.h"

#ifdef WIN32
#include <windows.h>
#endif

void *GDHandleTable;

/* global data */
typedef struct {
    tblHeader_pt handleTbl;
} GdData;

typedef int (CmdFunc)(Tcl_Interp *, GdData *, int, Tcl_Obj *CONST []);
typedef int (ColCmdFunc)(Tcl_Interp *, gdImagePtr, int, int[]);

static CmdFunc tclGdCreateCmd;
static CmdFunc tclGdDestroyCmd;
static CmdFunc tclGdWriteCmd;
static CmdFunc tclGdColorCmd;
static CmdFunc tclGdInterlaceCmd;
static CmdFunc tclGdSetCmd;
static CmdFunc tclGdLineCmd;
static CmdFunc tclGdRectCmd;
static CmdFunc tclGdArcCmd;
static CmdFunc tclGdFillCmd;
static CmdFunc tclGdSizeCmd;
static CmdFunc tclGdTextCmd;
static CmdFunc tclGdCopyCmd;
static CmdFunc tclGdGetCmd;
static CmdFunc tclGdBrushCmd;
static CmdFunc tclGdStyleCmd;
static CmdFunc tclGdTileCmd;
static CmdFunc tclGdPolygonCmd;
#ifdef HAVE_GD_PNG
static CmdFunc tclGdWriteBufCmd;
#endif

static ColCmdFunc tclGdColorNewCmd;
static ColCmdFunc tclGdColorExactCmd;
static ColCmdFunc tclGdColorClosestCmd;
static ColCmdFunc tclGdColorResolveCmd;
static ColCmdFunc tclGdColorFreeCmd;
static ColCmdFunc tclGdColorTranspCmd;
static ColCmdFunc tclGdColorGetCmd;

typedef struct {
    char *cmd;
    CmdFunc *f;
    int minargs, maxargs;
    int subcmds;
    int ishandle;
    char *usage;
} cmdOptions;

typedef struct {
    char *cmd;
    ColCmdFunc *f;
    int minargs, maxargs;
    int subcmds;
    int ishandle;
    char *usage;
} colCmdOptions;

typedef struct {
    char *buf;
    int buflen;
} BuffSinkContext;

static cmdOptions subcmdVec[] = {
    {"create", tclGdCreateCmd, 2, 2, 0, 0,
     "width height"},
    {"createTrueColor", tclGdCreateCmd, 2, 2, 0, 0,
     "width height"},
    {"createFromGD", tclGdCreateCmd, 1, 1, 0, 0,
     "filehandle"},
#ifdef HAVE_LIBZ
    {"createFromGD2", tclGdCreateCmd, 1, 1, 0, 0,
     "filehandle"},
#endif
#ifdef HAVE_GD_GIF
    {"createFromGIF", tclGdCreateCmd, 1, 1, 0, 0,
     "filehandle"},
#endif
#ifdef HAVE_GD_JPEG
    {"createFromJPEG", tclGdCreateCmd, 1, 1, 0, 0,
     "filehandle"},
#endif
#ifdef HAVE_GD_PNG
    {"createFromPNG", tclGdCreateCmd, 1, 1, 0, 0,
     "filehandle"},
#endif
    {"createFromWBMP", tclGdCreateCmd, 1, 1, 0, 0,
     "filehandle"},
#ifdef HAVE_GD_XPM
    {"createFromXBM", tclGdCreateCmd, 1, 1, 0, 0,
     "filehandle"},
#endif

    {"destroy", tclGdDestroyCmd, 1, 1, 0, 1,
     "gdhandle"},

    {"writeGD", tclGdWriteCmd, 2, 2, 0, 1,
     "gdhandle filehandle"},
#ifdef HAVE_LIBZ
    {"writeGD2", tclGdWriteCmd, 2, 2, 0, 1,
     "gdhandle filehandle"},
#endif
#ifdef HAVE_GD_GIF
    {"writeGIF", tclGdWriteCmd, 2, 2, 0, 1,
     "gdhandle filehandle"},
#endif
#ifdef HAVE_GD_JPEG
    {"writeJPEG", tclGdWriteCmd, 2, 2, 0, 1,
     "gdhandle filehandle"},
#endif
#ifdef HAVE_GD_PNG
    {"writePNG", tclGdWriteCmd, 2, 2, 0, 1,
     "gdhandle filehandle"},
#endif
    {"writeWBMP", tclGdWriteCmd, 2, 2, 0, 1,
     "gdhandle filehandle"},
#ifdef HAVE_GD_XPM
    {"writeXBM", tclGdWriteCmd, 2, 2, 0, 1,
     "gdhandle filehandle"},
#endif
#ifdef HAVE_GD_PNG
    {"writePNGvar", tclGdWriteBufCmd, 2, 2, 0, 1,
     "gdhandle var"},
#endif

    {"interlace", tclGdInterlaceCmd, 1, 2, 0, 1,
     "gdhandle ?on-off?"},

    {"color", tclGdColorCmd, 2, 5, 1, 1,
     "option values..."},
    {"brush", tclGdBrushCmd, 2, 2, 0, 2,
     "gdhandle brushhandle"},
    {"style", tclGdStyleCmd, 2, 999, 0, 1,
     "gdhandle color..."},
    {"tile", tclGdTileCmd, 2, 2, 0, 2,
     "gdhandle tilehandle"},

    {"set", tclGdSetCmd, 4, 4, 0, 1,
     "gdhandle color x y"},
    {"line", tclGdLineCmd, 6, 6, 0, 1,
     "gdhandle color x1 y1 x2 y2"},
    {"rectangle", tclGdRectCmd, 6, 6, 0, 1,
     "gdhandle color x1 y1 x2 y2"},
    {"fillrectangle", tclGdRectCmd, 6, 6, 0, 1,
     "gdhandle color x1 y1 x2 y2"},
    {"arc", tclGdArcCmd, 8, 8, 0, 1,
     "gdhandle color cx cy width height start end"},
    {"fillarc", tclGdArcCmd, 8, 8, 0, 1,
     "gdhandle color cx cy width height start end"},
    {"openarc", tclGdArcCmd, 8, 8, 0, 1,
     "gdhandle color cx cy width height start end"},
    {"chord", tclGdArcCmd, 8, 8, 0, 1,
     "gdhandle color cx cy width height start end"},
    {"fillchord", tclGdArcCmd, 8, 8, 0, 1,
     "gdhandle color cx cy width height start end"},
    {"openchord", tclGdArcCmd, 8, 8, 0, 1,
     "gdhandle color cx cy width height start end"},
    {"pie", tclGdArcCmd, 8, 8, 0, 1,
     "gdhandle color cx cy width height start end"},
    {"fillpie", tclGdArcCmd, 8, 8, 0, 1,
     "gdhandle color cx cy width height start end"},
    {"openpie", tclGdArcCmd, 8, 8, 0, 1,
     "gdhandle color cx cy width height start end"},
    {"polygon", tclGdPolygonCmd, 2, 999, 0, 1,
     "gdhandle color x1 y1 x2 y2 x3 y3 ..."},
    {"fillpolygon", tclGdPolygonCmd, 3, 999, 0, 1,
     "gdhandle color x1 y1 x2 y2 x3 y3 ..."},
    {"fill", tclGdFillCmd, 4, 5, 0, 1,
     "gdhandle color x y ?bordercolor?"},
/* 
 * we allow null gd handles to the text command to allow program to get size
 * of text string, so the text command provides its own handle processing and checking
 */
    {"text", tclGdTextCmd, 8, 8, 0, 0,
     "gdhandle color fontname size angle x y string"},


    {"copy", tclGdCopyCmd, 8, 10, 0, 2,
     "desthandle srchandle destx desty srcx srcy destw desth ?srcw srch?"},

    {"get", tclGdGetCmd, 3, 3, 0, 1,
     "gdhandle x y"},
    {"size", tclGdSizeCmd, 1, 1, 0, 1,
     "gdhandle"},
};

static colCmdOptions colorCmdVec[] = {
    {"new", tclGdColorNewCmd, 5, 5, 1, 1,
     "gdhandle red green blue"},
    {"exact", tclGdColorExactCmd, 5, 5, 1, 1,
     "gdhandle red green blue"},
    {"closest", tclGdColorClosestCmd, 5, 5, 1, 1,
     "gdhandle red green blue"},
    {"resolve", tclGdColorResolveCmd, 5, 5, 1, 1,
     "gdhandle red green blue"},
    {"free", tclGdColorFreeCmd, 3, 3, 1, 1,
     "gdhandle color"},
    {"transparent", tclGdColorTranspCmd, 2, 3, 1, 1,
     "gdhandle ?color?"},
    {"get", tclGdColorGetCmd, 2, 3, 1, 1,
     "gdhandle ?color?"}
};

/*
 * Helper function to interpret color_idx values.
 */
static int tclGd_GetColor(Tcl_Interp * interp, Tcl_Obj * obj, int *color)
{
    int nlist, retval = TCL_OK;
    Tcl_Obj **theList;
    char *firsttag, *secondtag;

    /* Assume it's an integer, check other cases on failure. */
    if (Tcl_GetIntFromObj(interp, obj, color) == TCL_OK)
	return TCL_OK;
    else {
	Tcl_ResetResult(interp);
	if (Tcl_ListObjGetElements(interp, obj, &nlist, &theList) !=
	    TCL_OK)
	    return TCL_ERROR;
	if (nlist < 1 || nlist > 2)
	    retval = TCL_ERROR;
	else {
	    firsttag = Tcl_GetString(theList[0]);
	    switch (firsttag[0]) {
	    case 'b':
		*color = gdBrushed;
		if (nlist == 2) {
		    secondtag = Tcl_GetString(theList[1]);
		    if (secondtag[0] == 's') {
			*color = gdStyledBrushed;
		    } else {
			retval = TCL_ERROR;
		    }
		}
		break;

	    case 's':
		*color = gdStyled;
		if (nlist == 2) {
		    secondtag = Tcl_GetString(theList[1]);
		    if (secondtag[0] == 'b') {
			*color = gdStyledBrushed;
		    } else {
			retval = TCL_ERROR;
		    }
		}
		break;

	    case 't':
		*color = gdTiled;
		break;

	    default:
		retval = TCL_ERROR;
	    }
	}
    }
    if (retval == TCL_ERROR)
	Tcl_SetResult(interp, "Malformed special color value", TCL_STATIC);

    return retval;
}

/*
 * GD composite command:
 *
 * gd create <width> <height>
 * 		Return a handle to a new gdImage that is width X height.
 * gd createTrueColor <width> <height>
 * 		Return a handle to a new trueColor gdImage that is width X height.
 * gd createFromGD <filehandle>
 * gd createFromGD2 <filehandle>
 * gd createFromGIF <filehandle>
 * gd createFromJPEG <filehandle>
 * gd createFromPNG <filehandle>
 * gd createFromWBMP <filehandle>
 * gd createFromXBM <filehandle>
 * 		Return a handle to a new gdImage created by reading an
 * 		image from the file of the indicated format
 *              open on filehandle.  
 *
 * gd destroy <gdhandle>
 * 		Destroy the gdImage referred to by gdhandle.
 *
 * gd writeGD  <gdhandle> <filehandle>
 * gd writeGD2 <gdhandle> <filehandle>
 * gd writeGIF <gdhandle> <filehandle>
 * gd writeJPEG <gdhandle> <filehandle>
 * gd writePNG <gdhandle> <filehandle>
 * gd writeWBMP <gdhandle> <filehandle>
 * gd writeXBM <gdhandle> <filehandle>
 * 		Write the image in gdhandle to filehandle in the
 *              format indicated.
 *
 * gd color new <gdhandle> <red> <green> <blue>
 * 		Allocate a new color with the given RGB values.  Returns the
 * 		color_idx, or -1 on failure (256 colors already allocated).
 * gd color exact <gdhandle> <red> <green> <blue>
 * 		Find a color_idx in the image that exactly matches the given RGB color.
 *      Returns the color_idx, or -1 if no exact match.
 * gd color closest <gdhandle> <red> <green> <blue>
 * 		Find a color in the image that is closest to the given RGB color.  
 *      Guaranteed to return a color idx.
 * gd color resolve <gdhandle> <red> <green> <blue>
 * 		Return the index of the best possible effort to get a color.  Guaranteed
 *      to return a color idx.   Equivalent to:
 *          if {[set idx [gd color exact $gd $r $g $b]] == -1} {
 *              if {[set idx [gd color neW $Gd $r $g $b]] == -1} {
 *                  set idx [gd color closest $gd $r $g $b]
 *              }
 *          }
 * gd color free <gdhandle> <color_idx>
 * 		Free the color at the given color_idx for reuse.
 * gd color transparent <gdhandle> <color_idx>
 * 		Mark the color_idx as the transparent background color.
 * gd color get <gdhandle> [<color_idx>]
 * 		Return the RGB value at <color_idx>, or {} if it is not allocated.
 * 		If <color_idx> is not specified, return a list of {color_idx R G B} values
 * 		for all allocated colors.
 * gd color gettransparent <gdhandle>
 * 		Return the color_idx of the transparent color.
 * 
 * gd brush <gdhandle> <brushhandle>
 * 		Set the brush image to be used for brushed lines.  Transparent
 * 		pixels in the brush will not change the image when the brush
 * 		is applied. 
 * gd style <gdhandle> <color_idx> ...
 * 		Set the line style to the list of color indices.  This is interpreted
 * 		in one of two ways.  For a simple styled line, each color is
 * 		applied to points along the line in turn.  The transparent color
 * 		value may be used to leave gaps in the line.  For a styled, brushed
 * 		line, a 0 (or the transparent color_idx) means not to fill the pixel,
 * 		and a non-zero value means to apply the brush.
 * gd tile <gdhandle> <tilehandle>
 * 		Set the tile image to be used for tiled fills.  Transparent pixels in
 * 		the tile will not change the underlying image during tiling.
 * 
 * In all drawing functions, the color_idx is a number, or may be one of the
 * strings styled, brushed, tiled, "styled brushed" or "brushed styled".  The
 * style, brush, or tile currently in effect will be used.  Brushing and
 * styling apply to lines, tiling to filled areas.
 * 
 * gd set <gdhandle> <color_idx> <x> <y>
 * 		Set the pixel at (x,y) to color <color_idx>.
 * gd line <gdhandle> <color_idx> <x1> <y1> <x2> <y2>
 * 		Draw a line in color <color_idx> from (x1,y1) to (x2,y2).
 * gd rectangle <gdhandle> <color_idx> <x1> <y1> <x2> <y2>
 * gd fillrectangle <gdhandle> <color_idx> <x1> <y1> <x2> <y2>
 * 		Draw the outline of (resp. fill) a rectangle in color <color_idx>
 * 		with corners at (x1,y1) and (x2,y2).
 * gd arc <gdhandle> <color_idx> <cx> <cy> <width> <height> <start> <end>
 * gd fillarc <gdhandle> <color_idx> <cx> <cy> <width> <height> <start> <end>
 * 		Draw an arc, or filled segment, in color <color_idx>, centered at (cx,cy)
 *      in a rectangle width x height, starting at start degrees and ending 
 *      at end degrees.		Start must be > end.
 * gd polygon <gdhandle> <color_idx> <x1> <y1> ...
 * gd fillpolygon <gdhandle> <color_idx> <x1> <y1> ...
 * 		Draw the outline of, or fill, a polygon specified by the x, y
 * 		coordinate list.  
 * 
 * gd fill <gdhandle> <color_idx> <x> <y>
 * gd fill <gdhandle> <color_idx> <x> <y> <borderindex>
 * 		Fill with color <color_idx>, starting from (x,y) within a region
 * 		of pixels all the color of the pixel at (x,y) (resp., within a
 * 		border colored borderindex).
 *
 * gd size <gdhandle>
 * 		Returns a list {width height} of the image.
 *
 * gd text <gdhandle> <color_idx> <fontname> <size> <angle> <x> <y> <string>
 *      Draw text using <fontname> in color <color_idx>,
 *      with pointsize <size>, rotation in radians <angle>, with lower left 
 *      corner at (x,y).  String may contain UTF8 sequences like: "&#192;"
 *      Returns 4 corner coords of bounding rectangle.
 *      Use gdhandle = {} to get boundary without rendering.
 *      Use negative of color_idx to disable antialiasing.
 *
 *      The file <fontname>.ttf must be found in the builtin DEFAULT_FONTPATH
 *      or in the fontpath specified in a GDFONTPATH environment variable.
 *
 * gd copy <desthandle> <srchandle> <destx> <desty> <srcx> <srcy> <w> <h>
 * gd copy <desthandle> <srchandle> <destx> <desty> <srcx> <srcy> \
 * 				<destw> <desth> <srcw> <srch>
 * 		Copy a subimage from srchandle(srcx, srcy) to
 * 		desthandle(destx, desty), size w x h.  Or, resize the subimage
 * 		in copying from srcw x srch to destw x desth.
 * 
 */
int
gdCmd(ClientData clientData, Tcl_Interp * interp,
      int argc, Tcl_Obj * CONST objv[])
{
    GdData *gdData = (GdData *) clientData;
    int argi, subi;
    char buf[100];
    /* Check for subcommand. */
    if (argc < 2) {
	Tcl_SetResult(interp,
		      "wrong # args: should be \"gd option ...\"",
		      TCL_STATIC);
	return TCL_ERROR;
    }

    /* Find the subcommand. */
    for (subi = 0; subi < (sizeof subcmdVec) / (sizeof subcmdVec[0]);
	 subi++) {
	if (strcmp(subcmdVec[subi].cmd, Tcl_GetString(objv[1])) == 0) {

	    /* Check arg count. */
	    if (argc - 2 < subcmdVec[subi].minargs ||
		argc - 2 > subcmdVec[subi].maxargs) {
		sprintf(buf, "wrong # args: should be \"gd %s %s\"",
			subcmdVec[subi].cmd, subcmdVec[subi].usage);
		Tcl_SetResult(interp, buf, TCL_VOLATILE);
		return TCL_ERROR;
	    }

	    /* Check for valid handle(s). */
	    if (subcmdVec[subi].ishandle > 0) {
		/* Are any handles allocated? */
		if (gdData->handleTbl == NULL) {
		    sprintf(buf, "no such handle%s: ",
			    subcmdVec[subi].ishandle > 1 ? "s" : "");
		    Tcl_SetResult(interp, buf, TCL_VOLATILE);
		    for (argi = 2 + subcmdVec[subi].subcmds;
			 argi < 2 + subcmdVec[subi].subcmds +
			 subcmdVec[subi].ishandle; argi++) {
			Tcl_AppendResult(interp,
					 Tcl_GetString(objv[argi]), " ",
					 0);
		    }
		    return TCL_ERROR;
		}
		/* Check each handle to see if it's a valid handle. */
		if (2 + subcmdVec[subi].subcmds +
		    subcmdVec[subi].ishandle > argc) {
		    Tcl_SetResult(interp, "GD handle(s) not specified",
				  TCL_STATIC);
		    return TCL_ERROR;
		}
		for (argi = 2 + subcmdVec[subi].subcmds;
		     argi < (2 + subcmdVec[subi].subcmds +
			     subcmdVec[subi].ishandle); argi++) {
		    if (!tclhandleXlate(gdData->handleTbl,
					Tcl_GetString(objv[argi])))
			return TCL_ERROR;
		}
	    }

	    /* Call the subcommand function. */
	    return (*subcmdVec[subi].f) (interp, gdData, argc, objv);
	}
    }

    /* If we get here, the option doesn't match. */
    Tcl_AppendResult(interp, "bad option \"",
		     Tcl_GetString(objv[1]), "\": should be ", 0);
    for (subi = 0; subi < (sizeof subcmdVec) / (sizeof subcmdVec[0]);
	 subi++)
	Tcl_AppendResult(interp, (subi > 0 ? ", " : ""),
			 subcmdVec[subi].cmd, 0);
    return TCL_ERROR;
}

static int
tclGdCreateCmd(Tcl_Interp * interp, GdData * gdData,
	       int argc, Tcl_Obj * CONST objv[])
{
    int w, h;
    unsigned long idx;
    gdImagePtr im = NULL;
    FILE *filePtr;
    ClientData clientdata;
    char *cmd, buf[50];
    int fileByName;

    cmd = Tcl_GetString(objv[1]);
    if (strcmp(cmd, "create") == 0) {
	if (Tcl_GetIntFromObj(interp, objv[2], &w) != TCL_OK)
	    return TCL_ERROR;
	if (Tcl_GetIntFromObj(interp, objv[3], &h) != TCL_OK)
	    return TCL_ERROR;
	im = gdImageCreate(w, h);
	if (im == NULL) {
	    sprintf(buf, "GD unable to allocate %d X %d image", w, h);
	    Tcl_SetResult(interp, buf, TCL_VOLATILE);
	    return TCL_ERROR;
	}
    } else if (strcmp(cmd, "createTrueColor") == 0) {
	if (Tcl_GetIntFromObj(interp, objv[2], &w) != TCL_OK)
	    return TCL_ERROR;
	if (Tcl_GetIntFromObj(interp, objv[3], &h) != TCL_OK)
	    return TCL_ERROR;
	im = gdImageCreateTrueColor(w, h);
	if (im == NULL) {
	    sprintf(buf, "GD unable to allocate %d X %d image", w, h);
	    Tcl_SetResult(interp, buf, TCL_VOLATILE);
	    return TCL_ERROR;
	}
    } else {
	fileByName = 0;		/* first try to get file from open channel */
	if (Tcl_GetOpenFile
	    (interp, Tcl_GetString(objv[2]), 0, 1,
	     &clientdata) == TCL_OK) {
	    filePtr = (FILE *) clientdata;
	} else {
	    /* Not a channel, or Tcl_GetOpenFile() not supported.
	     *   See if we can open directly.
	     */
	    fileByName++;
	    if ((filePtr = fopen(Tcl_GetString(objv[2]), "rb")) == NULL) {
		return TCL_ERROR;
	    }
	    Tcl_ResetResult(interp);
	}

	/* Read file */
	if (strcmp(&cmd[10], "GD") == 0) {
	    im = gdImageCreateFromGd(filePtr);
#ifdef HAVE_LIBZ
	} else if (strcmp(&cmd[10], "GD2") == 0) {
	    im = gdImageCreateFromGd2(filePtr);
#endif
#ifdef HAVE_GD_GIF
	} else if (strcmp(&cmd[10], "GIF") == 0) {
	    im = gdImageCreateFromGif(filePtr);
#endif
#ifdef HAVE_GD_JPEG
	} else if (strcmp(&cmd[10], "JPEG") == 0) {
	    im = gdImageCreateFromJpeg(filePtr);
#endif
#ifdef HAVE_GD_PNG
	} else if (strcmp(&cmd[10], "PNG") == 0) {
	    im = gdImageCreateFromPng(filePtr);
#endif
	} else if (strcmp(&cmd[10], "WBMP") == 0) {
	    im = gdImageCreateFromWBMP(filePtr);
#ifdef HAVE_GD_XPM
	} else if (strcmp(&cmd[10], "XBM") == 0) {
/* FIXME - also "XPM" ? */
	    im = gdImageCreateFromXbm(filePtr);
#endif
	} else {
	    /* no im struct - will result in error */
	}
	if (fileByName) {
	    fclose(filePtr);
	}
	if (im == NULL) {
	    Tcl_SetResult(interp, "GD unable to read image file",
			  TCL_STATIC);
	    return TCL_ERROR;
	}
    }

    *(gdImagePtr *) (tclhandleAlloc(gdData->handleTbl, buf, &idx)) = im;
    Tcl_SetResult(interp, buf, TCL_VOLATILE);
    return TCL_OK;
}

static int
tclGdDestroyCmd(Tcl_Interp * interp, GdData * gdData, int argc,
		Tcl_Obj * CONST objv[])
{
    gdImagePtr im;

    unsigned long idx;

    if (tclhandleIndex(gdData->handleTbl, Tcl_GetString(objv[2]), &idx) != TCL_OK)

	return TCL_ERROR;
    im = *(gdImagePtr *) tclhandleXlateIndex(gdData->handleTbl, idx);
    tclhandleFreeIndex(gdData->handleTbl, idx);
    gdImageDestroy(im);

    return TCL_OK;
}

static int
tclGdWriteCmd(Tcl_Interp * interp, GdData * gdData, int argc,
	      Tcl_Obj * CONST objv[])
{
    gdImagePtr im;
    FILE *filePtr;
    ClientData clientdata;
    char *cmd;
    int fileByName;

    cmd = Tcl_GetString(objv[1]);
    /* Get the image pointer. */
    im = *(gdImagePtr *) tclhandleXlate(gdData->handleTbl,
					Tcl_GetString(objv[2]));

    /* Get the file reference. */
    fileByName = 0;		/* first try to get file from open channel */
    if (Tcl_GetOpenFile(interp, Tcl_GetString(objv[3]), 1, 1, &clientdata)
	== TCL_OK) {
	filePtr = (FILE *) clientdata;
    } else {
	/* Not a channel, or Tcl_GetOpenFile() not supported.
	 *   See if we can open directly.
	 */
	fileByName++;
	if ((filePtr = fopen(Tcl_GetString(objv[3]), "wb")) == NULL) {
	    return TCL_ERROR;
	}
	Tcl_ResetResult(interp);
    }

/*
 * Write IM to OUTFILE as a JFIF-formatted JPEG image, using quality
 * JPEG_QUALITY.  If JPEG_QUALITY is in the range 0-100, increasing values
 * represent higher quality but also larger image size.  If JPEG_QUALITY is
 * negative, the IJG JPEG library's default quality is used (which
 * should be near optimal for many applications).  See the IJG JPEG
 * library documentation for more details.  */

    /* Do it. */
    if (strcmp(&cmd[5], "GD") == 0) {
	gdImageGd(im, filePtr);
    } else if (strcmp(&cmd[5], "GD2") == 0) {
#ifdef HAVE_LIBZ
#define GD2_CHUNKSIZE 128
#define GD2_COMPRESSED 2
	gdImageGd2(im, filePtr, GD2_CHUNKSIZE, GD2_COMPRESSED);
#endif
#ifdef HAVE_GD_GIF
    } else if (strcmp(&cmd[5], "GIF") == 0) {
	gdImageGif(im, filePtr);
#endif
#ifdef HAVE_GD_JPEG
    } else if (strcmp(&cmd[5], "JPEG") == 0) {
#define JPEG_QUALITY -1
	gdImageJpeg(im, filePtr, JPEG_QUALITY);
#endif
#ifdef HAVE_GD_PNG
    } else if (strcmp(&cmd[5], "PNG") == 0) {
	gdImagePng(im, filePtr);
#endif
    } else if (strcmp(&cmd[5], "WBMP") == 0) {
	/* Assume the color closest to black is the foreground
	   color for the B&W wbmp image. */
	int foreground = gdImageColorClosest(im, 0, 0, 0);
	gdImageWBMP(im, foreground, filePtr);
#if 0
/* libgd only supports reading xbm format */
#ifdef HAVE_GD_XPM
    } else if (strcmp(&cmd[5], "XBM") == 0) {
	gdImageXbm(im, filePtr);
#endif
#endif
    } else {
	/* cannot happen - but would result in an empty output file */
    }
    if (fileByName) {
	fclose(filePtr);
    } else {
	fflush(filePtr);
    }
    return TCL_OK;
}

static int
tclGdInterlaceCmd(Tcl_Interp * interp, GdData * gdData,
		  int argc, Tcl_Obj * CONST objv[])
{
    gdImagePtr im;
    int on_off;

    /* Get the image pointer. */
    im = *(gdImagePtr *) tclhandleXlate(gdData->handleTbl,
					Tcl_GetString(objv[2]));

    if (argc == 4) {
	/* Get the on_off values. */
	if (Tcl_GetBooleanFromObj(interp, objv[3], &on_off) != TCL_OK)
	    return TCL_ERROR;

	/* Do it. */
	gdImageInterlace(im, on_off);
    } else {
	/* Get the current state. */
	on_off = gdImageGetInterlaced(im);
    }
    Tcl_SetObjResult(interp, Tcl_NewBooleanObj(on_off));
    return TCL_OK;
}


static int
tclGdColorCmd(Tcl_Interp * interp, GdData * gdData,
	      int argc, Tcl_Obj * CONST objv[])
{
    gdImagePtr im;
    int subi, nsub, i, args[3];

    nsub = (sizeof colorCmdVec) / (sizeof colorCmdVec[0]);
    if (argc >= 3) {
	/* Find the subcommand. */
	for (subi = 0; subi < nsub; subi++) {
	    if (strcmp(colorCmdVec[subi].cmd, Tcl_GetString(objv[2])) == 0) {
		/* Check arg count. */
		if (argc - 2 < colorCmdVec[subi].minargs ||
		    argc - 2 > colorCmdVec[subi].maxargs) {
		    Tcl_AppendResult(interp,
				     "wrong # args: should be \"gd color ",
				     colorCmdVec[subi].cmd, " ",
				     colorCmdVec[subi].usage, "\"", 0);
		    return TCL_ERROR;
		}

		/* Get the image pointer. */
		im = *(gdImagePtr *) tclhandleXlate(gdData->handleTbl,
						    Tcl_GetString(objv
								  [3]));
		/* Parse off integer arguments.
		 * 1st 4 are gd color <opt> <handle>
		 */
		for (i = 0; i < argc - 4; i++) {
		    if (Tcl_GetIntFromObj(interp, objv[i + 4], &args[i]) !=
			TCL_OK) {


#if 0
			if (args[i] < 0 || args[i] > 255) {
#else
/* gd text uses -ve colors to turn off anti-aliasing */
			if (args[i] < -255 || args[i] > 255) {
#endif
			    Tcl_SetResult(interp,
					  "argument out of range 0-255",
					  TCL_STATIC);
			    return TCL_ERROR;
			}
		    }
		}

		/* Call the subcommand function. */
		return (*colorCmdVec[subi].f) (interp, im, argc - 4, args);
	    }
	}
    }

    /* If we get here, the option doesn't match. */
    if (argc > 2) {
	Tcl_AppendResult(interp, "bad option \"",
			 Tcl_GetString(objv[2]), "\": ", 0);
    } else {
	Tcl_AppendResult(interp, "wrong # args: ", 0);
    }
    Tcl_AppendResult(interp, "should be ", 0);
    for (subi = 0; subi < nsub; subi++)
	Tcl_AppendResult(interp, (subi > 0 ? ", " : ""),
			 colorCmdVec[subi].cmd, 0);

    return TCL_ERROR;
}

static int
tclGdColorNewCmd(Tcl_Interp * interp, gdImagePtr im, int argc, int args[])
{
    int color;

    color = gdImageColorAllocate(im, args[0], args[1], args[2]);
    Tcl_SetObjResult(interp, Tcl_NewIntObj(color));
    return TCL_OK;
}

static int
tclGdColorExactCmd(Tcl_Interp * interp, gdImagePtr im, int argc,
		   int args[])
{
    int color;

    color = gdImageColorExact(im, args[0], args[1], args[2]);
    Tcl_SetObjResult(interp, Tcl_NewIntObj(color));
    return TCL_OK;
}

static int
tclGdColorClosestCmd(Tcl_Interp * interp, gdImagePtr im, int argc,
		     int args[])
{
    int color;

    color = gdImageColorClosest(im, args[0], args[1], args[2]);
    Tcl_SetObjResult(interp, Tcl_NewIntObj(color));
    return TCL_OK;
}

static int
tclGdColorResolveCmd(Tcl_Interp * interp, gdImagePtr im, int argc,
		     int args[])
{
    int color;

    color = gdImageColorResolve(im, args[0], args[1], args[2]);
    Tcl_SetObjResult(interp, Tcl_NewIntObj(color));
    return TCL_OK;
}

static int
tclGdColorFreeCmd(Tcl_Interp * interp, gdImagePtr im, int argc, int args[])
{
    gdImageColorDeallocate(im, args[0]);
    return TCL_OK;
}

static int
tclGdColorTranspCmd(Tcl_Interp * interp, gdImagePtr im, int argc,
		    int args[])
{
    int color;

    if (argc > 0) {
	color = args[0];
	gdImageColorTransparent(im, color);
    } else {
	color = gdImageGetTransparent(im);
    }
    Tcl_SetObjResult(interp, Tcl_NewIntObj(color));
    return TCL_OK;
}

static int
tclGdColorGetCmd(Tcl_Interp * interp, gdImagePtr im, int argc, int args[])
{
    char buf[30];
    int i;

    /* IF one arg, return the single color, else return list of all colors. */
    if (argc == 1) {
	i = args[0];
	if (i >= gdImageColorsTotal(im) || im->open[i]) {
	    Tcl_SetResult(interp, "No such color", TCL_STATIC);
	    return TCL_ERROR;
	}
	sprintf(buf, "%d %d %d %d", i,
		gdImageRed(im, i),
		gdImageGreen(im, i), gdImageBlue(im, i));
	Tcl_SetResult(interp, buf, TCL_VOLATILE);
    } else {
	for (i = 0; i < gdImageColorsTotal(im); i++) {
	    if (im->open[i])
		continue;
	    sprintf(buf, "%d %d %d %d", i,
		    gdImageRed(im, i),
		    gdImageGreen(im, i), gdImageBlue(im, i));
	    Tcl_AppendElement(interp, buf);
	}
    }

    return TCL_OK;
}

static int
tclGdBrushCmd(Tcl_Interp * interp, GdData * gdData,
	      int argc, Tcl_Obj * CONST objv[])
{
    gdImagePtr im, imbrush;

    /* Get the image pointers. */
    im = *(gdImagePtr *) tclhandleXlate(gdData->handleTbl,
					Tcl_GetString(objv[2]));
    imbrush = *(gdImagePtr *) tclhandleXlate(gdData->handleTbl,
					     Tcl_GetString(objv[3]));

    /* Do it. */
    gdImageSetBrush(im, imbrush);

    return TCL_OK;
}

static int
tclGdTileCmd(Tcl_Interp * interp, GdData * gdData,
	     int argc, Tcl_Obj * CONST objv[])
{
    gdImagePtr im, tile;

    /* Get the image pointers. */
    im = *(gdImagePtr *) tclhandleXlate(gdData->handleTbl,
					Tcl_GetString(objv[2]));
    tile = *(gdImagePtr *) tclhandleXlate(gdData->handleTbl,
					  Tcl_GetString(objv[3]));

    /* Do it. */
    gdImageSetTile(im, tile);

    return TCL_OK;
}


static int
tclGdStyleCmd(Tcl_Interp * interp, GdData * gdData,
	      int argc, Tcl_Obj * CONST objv[])
{
    gdImagePtr im;
    int ncolor, *colors = NULL, i;
    Tcl_Obj **colorObjv = (Tcl_Obj **) (&objv[3]);	/* By default, colors are listed in objv. */
    int retval = TCL_OK;

    /* Get the image pointer. */
    im = *(gdImagePtr *) tclhandleXlate(gdData->handleTbl,
					Tcl_GetString(objv[2]));

    /* Figure out how many colors in the style list and allocate memory. */
    ncolor = argc - 3;
    /* If only one argument, treat it as a list. */
    if (ncolor == 1)
	if (Tcl_ListObjGetElements(interp, objv[3],
				   &ncolor, &colorObjv) != TCL_OK)
	    return TCL_ERROR;

    colors = (int *) Tcl_Alloc(ncolor * sizeof(int));
    if (colors == NULL) {
	Tcl_SetResult(interp, "Memory allocation failed", TCL_STATIC);
	retval = TCL_ERROR;
	goto out;
    }
    /* Get the color values. */
    for (i = 0; i < ncolor; i++)
	if (Tcl_GetIntFromObj(interp, colorObjv[i], &colors[i]) != TCL_OK) {
	    retval = TCL_ERROR;
	    break;
	}

    /* Call the Style function if no error. */
    if (retval == TCL_OK)
	gdImageSetStyle(im, colors, ncolor);

  out:
    /* Free the colors. */
    if (colors != NULL)
	Tcl_Free((char *) colors);

    return retval;
}

static int
tclGdSetCmd(Tcl_Interp * interp, GdData * gdData,
	    int argc, Tcl_Obj * CONST objv[])
{
    gdImagePtr im;
    int color, x, y;

    /* Get the image pointer. */
    im = *(gdImagePtr *) tclhandleXlate(gdData->handleTbl,
					Tcl_GetString(objv[2]));

    /* Get the color, x, y values. */
    if (tclGd_GetColor(interp, objv[3], &color) != TCL_OK)
	return TCL_ERROR;
    if (Tcl_GetIntFromObj(interp, objv[4], &x) != TCL_OK)
	return TCL_ERROR;
    if (Tcl_GetIntFromObj(interp, objv[5], &y) != TCL_OK)
	return TCL_ERROR;

    /* Call the Set function. */
    gdImageSetPixel(im, x, y, color);

    return TCL_OK;
}

static int
tclGdLineCmd(Tcl_Interp * interp, GdData * gdData,
	     int argc, Tcl_Obj * CONST objv[])
{
    gdImagePtr im;
    int color, x1, y1, x2, y2;

    /* Get the image pointer. */
    im = *(gdImagePtr *) tclhandleXlate(gdData->handleTbl,
					Tcl_GetString(objv[2]));

    /* Get the color, x, y values. */
    if (tclGd_GetColor(interp, objv[3], &color) != TCL_OK)
	return TCL_ERROR;
    if (Tcl_GetIntFromObj(interp, objv[4], &x1) != TCL_OK)
	return TCL_ERROR;
    if (Tcl_GetIntFromObj(interp, objv[5], &y1) != TCL_OK)
	return TCL_ERROR;
    if (Tcl_GetIntFromObj(interp, objv[6], &x2) != TCL_OK)
	return TCL_ERROR;
    if (Tcl_GetIntFromObj(interp, objv[7], &y2) != TCL_OK)
	return TCL_ERROR;

    /* Call the appropriate Line function. */
    gdImageLine(im, x1, y1, x2, y2, color);

    return TCL_OK;
}

static int
tclGdRectCmd(Tcl_Interp * interp, GdData * gdData,
	     int argc, Tcl_Obj * CONST objv[])
{
    gdImagePtr im;
    int color, x1, y1, x2, y2;
    char *cmd;

    /* Get the image pointer. */
    im = *(gdImagePtr *) tclhandleXlate(gdData->handleTbl,
					Tcl_GetString(objv[2]));

    /* Get the color, x, y values. */
    if (tclGd_GetColor(interp, objv[3], &color) != TCL_OK)
	return TCL_ERROR;
    if (Tcl_GetIntFromObj(interp, objv[4], &x1) != TCL_OK)
	return TCL_ERROR;
    if (Tcl_GetIntFromObj(interp, objv[5], &y1) != TCL_OK)
	return TCL_ERROR;
    if (Tcl_GetIntFromObj(interp, objv[6], &x2) != TCL_OK)
	return TCL_ERROR;
    if (Tcl_GetIntFromObj(interp, objv[7], &y2) != TCL_OK)
	return TCL_ERROR;

    /* Call the appropriate rectangle function. */
    cmd = Tcl_GetString(objv[1]);
    if (cmd[0] == 'r')
	gdImageRectangle(im, x1, y1, x2, y2, color);
    else
	gdImageFilledRectangle(im, x1, y1, x2, y2, color);

    return TCL_OK;
}

static int
tclGdArcCmd(Tcl_Interp * interp, GdData * gdData,
	    int argc, Tcl_Obj * CONST objv[])
{
    gdImagePtr im;
    int color, cx, cy, width, height, start, end;
    char *cmd;

    /* Get the image pointer. */
    im = *(gdImagePtr *) tclhandleXlate(gdData->handleTbl,
					Tcl_GetString(objv[2]));

    /* Get the color, x, y values. */
    if (tclGd_GetColor(interp, objv[3], &color) != TCL_OK)
	return TCL_ERROR;
    if (Tcl_GetIntFromObj(interp, objv[4], &cx) != TCL_OK)
	return TCL_ERROR;
    if (Tcl_GetIntFromObj(interp, objv[5], &cy) != TCL_OK)
	return TCL_ERROR;
    if (Tcl_GetIntFromObj(interp, objv[6], &width) != TCL_OK)
	return TCL_ERROR;
    if (Tcl_GetIntFromObj(interp, objv[7], &height) != TCL_OK)
	return TCL_ERROR;
    if (Tcl_GetIntFromObj(interp, objv[8], &start) != TCL_OK)
	return TCL_ERROR;
    if (Tcl_GetIntFromObj(interp, objv[9], &end) != TCL_OK)
	return TCL_ERROR;

    /* Call the appropriate arc function. */
    cmd = Tcl_GetString(objv[1]);
    if (cmd[0] == 'a')                        /* arc */
	gdImageArc(im, cx, cy, width, height, start, end, color);
/* This one is not really useful as gd renderers it the same as fillpie */
/* It would be more useful if gd provided fill between arc and chord */
    else if (cmd[0] == 'f' && cmd[4] == 'a')  /* fill arc */
	gdImageFilledArc(im, cx, cy, width, height, start, end, color, gdArc);
/* this one is a kludge */
    else if (cmd[0] == 'o' && cmd[4] == 'a')  { /* open arc */
	gdImageArc(im, cx, cy, width, height, start, end, color);
	gdImageFilledArc(im, cx, cy, width, height, start, end, color, gdChord | gdNoFill);
    }
    else if (cmd[0] == 'c')                   /* chord */
	gdImageFilledArc(im, cx, cy, width, height, start, end, color, gdChord | gdNoFill);
    else if (cmd[0] == 'f' && cmd[4] == 'c')  /* fill chord */
	gdImageFilledArc(im, cx, cy, width, height, start, end, color, gdChord);
    else if (cmd[0] == 'o' && cmd[4] == 'c')  /* open chord */
	gdImageFilledArc(im, cx, cy, width, height, start, end, color, gdChord | gdEdged | gdNoFill);
    else if (cmd[0] == 'p' || (cmd[0] == 'f' && cmd[4] == 'p'))  /* pie or fill pie */
	gdImageFilledArc(im, cx, cy, width, height, start, end, color, gdPie);
    else if (cmd[0] == 'o' && cmd[4] == 'p')  /* open pie */
	gdImageFilledArc(im, cx, cy, width, height, start, end, color, gdPie | gdEdged | gdNoFill);

    return TCL_OK;
}

static int
tclGdPolygonCmd(Tcl_Interp * interp, GdData * gdData,
		int argc, Tcl_Obj * CONST objv[])
{
    gdImagePtr im;
    int color, npoints, i;
    Tcl_Obj **pointObjv = (Tcl_Obj **) (&objv[4]);
    gdPointPtr points = NULL;
    int retval = TCL_OK;
    char *cmd;

    /* Get the image pointer. */
    im = *(gdImagePtr *) tclhandleXlate(gdData->handleTbl,
					Tcl_GetString(objv[2]));

    /* Get the color, x, y values. */
    if (tclGd_GetColor(interp, objv[3], &color) != TCL_OK)
	return TCL_ERROR;

    /* Figure out how many points in the list and allocate memory. */
    npoints = argc - 4;
    /* If only one argument, treat it as a list. */
    if (npoints == 1)
	if (Tcl_ListObjGetElements(interp, objv[4],
				   &npoints, &pointObjv) != TCL_OK)
	    return TCL_ERROR;

    /* Error check size of point list. */
    if (npoints % 2 != 0) {
	Tcl_SetResult(interp, "Number of coordinates must be even",
		      TCL_STATIC);
	retval = TCL_ERROR;
	goto out;
    }

    /* Divide by 2 to get number of points, and final error check. */
    npoints /= 2;
    if (npoints < 3) {
	Tcl_SetResult(interp, "Must specify at least 3 points.",
		      TCL_STATIC);
	retval = TCL_ERROR;
	goto out;
    }

    points = (gdPointPtr) Tcl_Alloc(npoints * sizeof(gdPoint));
    if (points == NULL) {
	Tcl_SetResult(interp, "Memory allocation failed", TCL_STATIC);
	retval = TCL_ERROR;
	goto out;
    }

    /* Get the point values. */
    for (i = 0; i < npoints; i++)
	if (Tcl_GetIntFromObj(interp, pointObjv[i * 2], &points[i].x) !=
	    TCL_OK
	    || Tcl_GetIntFromObj(interp, pointObjv[i * 2 + 1],
				 &points[i].y) != TCL_OK) {
	    retval = TCL_ERROR;
	    break;
	}

    /* Call the appropriate polygon function. */
    cmd = Tcl_GetString(objv[1]);
    if (cmd[0] == 'p')
	gdImagePolygon(im, points, npoints, color);
    else
	gdImageFilledPolygon(im, points, npoints, color);

  out:
    /* Free the points. */
    if (points != NULL)
	Tcl_Free((char *) points);

    /* return TCL_OK; */
    return retval;
}

static int
tclGdFillCmd(Tcl_Interp * interp, GdData * gdData,
	     int argc, Tcl_Obj * CONST objv[])
{
    gdImagePtr im;
    int color, x, y, border;

    /* Get the image pointer. */
    im = *(gdImagePtr *) tclhandleXlate(gdData->handleTbl,
					Tcl_GetString(objv[2]));

    /* Get the color, x, y and possibly bordercolor values. */
    if (tclGd_GetColor(interp, objv[3], &color) != TCL_OK)
	return TCL_ERROR;
    if (Tcl_GetIntFromObj(interp, objv[4], &x) != TCL_OK)
	return TCL_ERROR;
    if (Tcl_GetIntFromObj(interp, objv[5], &y) != TCL_OK)
	return TCL_ERROR;

    /* Call the appropriate fill function. */
    if (argc - 2 == 5) {
	if (Tcl_GetIntFromObj(interp, objv[6], &border) != TCL_OK)
	    return TCL_ERROR;
	gdImageFillToBorder(im, x, y, border, color);
    } else {
	gdImageFill(im, x, y, color);
    }

    return TCL_OK;
}

static int
tclGdCopyCmd(Tcl_Interp * interp, GdData * gdData,
	     int argc, Tcl_Obj * CONST objv[])
{
    gdImagePtr imdest, imsrc;
    int destx, desty, srcx, srcy, destw, desth, srcw, srch;

    /* Get the image pointer. */
    imdest = *(gdImagePtr *) tclhandleXlate(gdData->handleTbl,
					    Tcl_GetString(objv[2]));
    imsrc = *(gdImagePtr *) tclhandleXlate(gdData->handleTbl,
					   Tcl_GetString(objv[3]));

    /* Get the x, y, etc. values. */
    if (Tcl_GetIntFromObj(interp, objv[4], &destx) != TCL_OK)
	return TCL_ERROR;
    if (Tcl_GetIntFromObj(interp, objv[5], &desty) != TCL_OK)
	return TCL_ERROR;
    if (Tcl_GetIntFromObj(interp, objv[6], &srcx) != TCL_OK)
	return TCL_ERROR;
    if (Tcl_GetIntFromObj(interp, objv[7], &srcy) != TCL_OK)
	return TCL_ERROR;
    if (Tcl_GetIntFromObj(interp, objv[8], &destw) != TCL_OK)
	return TCL_ERROR;
    if (Tcl_GetIntFromObj(interp, objv[9], &desth) != TCL_OK)
	return TCL_ERROR;

    /* Call the appropriate copy function. */
    if (argc - 2 == 10) {
	if (Tcl_GetIntFromObj(interp, objv[10], &srcw) != TCL_OK)
	    return TCL_ERROR;
	if (Tcl_GetIntFromObj(interp, objv[11], &srch) != TCL_OK)
	    return TCL_ERROR;

	gdImageCopyResized(imdest, imsrc, destx, desty, srcx, srcy,
			   destw, desth, srcw, srch);
    } else
	gdImageCopy(imdest, imsrc, destx, desty, srcx, srcy, destw, desth);

    return TCL_OK;
}

static int
tclGdGetCmd(Tcl_Interp * interp, GdData * gdData,
	    int argc, Tcl_Obj * CONST objv[])
{
    gdImagePtr im;
    int color, x, y;

    /* Get the image pointer. */
    im = *(gdImagePtr *) tclhandleXlate(gdData->handleTbl,
					Tcl_GetString(objv[2]));

    /* Get the x, y values. */
    if (Tcl_GetIntFromObj(interp, objv[3], &x) != TCL_OK)
	return TCL_ERROR;
    if (Tcl_GetIntFromObj(interp, objv[4], &y) != TCL_OK)
	return TCL_ERROR;

    /* Call the Get function. */
    color = gdImageGetPixel(im, x, y);
    Tcl_SetObjResult(interp, Tcl_NewIntObj(color));
    return TCL_OK;
}

static int
tclGdSizeCmd(Tcl_Interp * interp, GdData * gdData,
	     int argc, Tcl_Obj * CONST objv[])
{
    gdImagePtr im;
    char buf[30];

    /* Get the image pointer. */
    im = *(gdImagePtr *) tclhandleXlate(gdData->handleTbl,
					Tcl_GetString(objv[2]));

    sprintf(buf, "%d %d", gdImageSX(im), gdImageSY(im));
    Tcl_SetResult(interp, buf, TCL_VOLATILE);
    return TCL_OK;
}

static int
tclGdTextCmd(Tcl_Interp * interp, GdData * gdData,
	     int argc, Tcl_Obj * CONST objv[])
{
    /* gd gdhandle color fontname size angle x y string */
    gdImagePtr im;
    int color, x, y;
    double ptsize, angle;
    char *error, buf[32], *fontname, *handle;
    int i, brect[8], len;
    char *str;

    /* Get the image pointer. (an invalid or null arg[2] will result in string
       size calculation but no rendering */
    handle = Tcl_GetString(objv[2]);
    if (!handle || *handle == '\0') {
	im = (gdImagePtr) NULL;
    } else {
	im = *(gdImagePtr *) tclhandleXlate(gdData->handleTbl, handle);
    }

    /* Get the color, values. */
    if (tclGd_GetColor(interp, objv[3], &color) != TCL_OK) {
	return TCL_ERROR;
    }

    /* Get point size */
    if (Tcl_GetDoubleFromObj(interp, objv[5], &ptsize) != TCL_OK) {
	return TCL_ERROR;
    }

    /* Get rotation (radians) */
    if (Tcl_GetDoubleFromObj(interp, objv[6], &angle) != TCL_OK) {
	return TCL_ERROR;
    }

    /* get x, y position */
    if (Tcl_GetIntFromObj(interp, objv[7], &x) != TCL_OK) {
	return TCL_ERROR;
    }
    if (Tcl_GetIntFromObj(interp, objv[8], &y) != TCL_OK) {
	return TCL_ERROR;
    }

    str = Tcl_GetStringFromObj(objv[9], &len);
    fontname = Tcl_GetString(objv[4]);

    gdFTUseFontConfig(1);
    error =
	gdImageStringFT(im, brect, color, fontname, ptsize, angle, x, y, str);

    if (error) {
	Tcl_SetResult(interp, error, TCL_VOLATILE);
	return TCL_ERROR;
    }
    for (i = 0; i < 8; i++) {
	sprintf(buf, "%d", brect[i]);
	Tcl_AppendElement(interp, buf);
    }
    return TCL_OK;
}

/* 
 * Initialize the package.
 */
#ifdef __CYGWIN__
int Gdtclft_Init(Tcl_Interp * interp)
#else
#ifdef __WIN32__
EXPORT(int, Gdtclft_Init) (interp)
#else
int Gdtclft_Init(Tcl_Interp * interp)
#endif
#endif
{
    static GdData gdData;

#ifdef USE_TCL_STUBS
    if (Tcl_InitStubs(interp, TCL_VERSION, 0) == NULL) {
	return TCL_ERROR;
    }
#else
    if (Tcl_PkgRequire(interp, "Tcl", TCL_VERSION, 0) == NULL) {
	return TCL_ERROR;
    }
#endif
    if (Tcl_PkgProvide(interp, "Gdtclft", VERSION) != TCL_OK) {
	return TCL_ERROR;
    }

    GDHandleTable = gdData.handleTbl =
	tclhandleInit("gd", sizeof(gdImagePtr), 2);
    if (gdData.handleTbl == NULL) {
	Tcl_AppendResult(interp, "unable to create table for GD handles.",
			 0);
	return TCL_ERROR;
    }

    Tcl_CreateObjCommand(interp, "gd", gdCmd, (ClientData) & gdData,
			 (Tcl_CmdDeleteProc *) NULL);
    return TCL_OK;
}

#ifdef __CYGWIN__
int Gdtclft_SafeInit(Tcl_Interp * interp)
#else
#ifdef __WIN32__
EXPORT(int, Gdtclft_SafeInit) (interp)
#else
int Gdtclft_SafeInit(Tcl_Interp * interp)
#endif
#endif
{
    return Gdtclft_Init(interp);
}

#ifndef __CYGWIN__
#ifdef __WIN32__
/* Define DLL entry point, standard macro */

/*
 *----------------------------------------------------------------------
 *
 * DllEntryPoint --
 *
 *    This wrapper function is used by Windows to invoke the
 *    initialization code for the DLL.  If we are compiling
 *    with Visual C++, this routine will be renamed to DllMain.
 *    routine.
 *
 * Results:
 *    Returns TRUE;
 *
 * Side effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */

BOOL APIENTRY DllEntryPoint(hInst, reason, reserved)
HINSTANCE hInst;		/* Library instance handle. */
DWORD reason;			/* Reason this function is being called. */
LPVOID reserved;		/* Not used. */
{
    return TRUE;
}
#endif
#endif

#ifdef HAVE_GD_PNG
static int BufferSinkFunc(void *context, const char *buffer, int len)
{
    BuffSinkContext *p = context;
    char *bufend;
    if (p->buflen == 0) {
	p->buf = Tcl_Alloc(len + 1);
	memcpy(p->buf, buffer, len);
	p->buf[len] = '\0';
	p->buflen = len;
    } else {
	p->buf = Tcl_Realloc(p->buf, len + p->buflen + 1);
	bufend = p->buf + p->buflen;
	memmove(bufend, buffer, len);
	p->buf[len + p->buflen] = '\0';
	p->buflen += len;
    }
    return len;
}

static int
tclGdWriteBufCmd(Tcl_Interp * interp, GdData * gdData, int argc,
		 Tcl_Obj * CONST objv[])
{
    gdImagePtr im;
    Tcl_Obj *output;
    /* char *cmd; */
    char *result = NULL;

    BuffSinkContext bsc = {
	NULL,
	0
    };
    BuffSinkContext *res;
    gdSink buffsink;

    buffsink.sink = BufferSinkFunc;
    buffsink.context = (void *) &bsc;
    /* cmd = */ Tcl_GetString(objv[1]);
    /* Get the image pointer. */
    im = *(gdImagePtr *) tclhandleXlate(gdData->handleTbl,
					Tcl_GetString(objv[2]));

    gdImagePngToSink(im, &buffsink);

    /* I'm not so hot with lots of pointer indirection, so this
       makes things easier. - davidw */
    res = buffsink.context;
    result = res->buf;

    output = Tcl_NewByteArrayObj((unsigned char *) result, res->buflen);
    if (output == NULL)
	return TCL_ERROR;
    else
	Tcl_IncrRefCount(output);

    if (Tcl_ObjSetVar2(interp, objv[3], NULL, output, 0) == NULL)
	return TCL_ERROR;
    else
	return TCL_OK;
}
#endif
