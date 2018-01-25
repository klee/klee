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

/* this is to get the following win32 DLL junk to work.
 * if ever tempted to remove this, first please read:
 * http://joel.editthispage.com/stories/storyReader$47
 */
#ifndef GLOBALS_H
#define GLOBALS_H

#ifdef _UWIN
#ifndef _POSIX_			/* ncc doesn't define _POSIX_ */
/* i.e. if this is the win32 build using nmake with CC=ncc (native C) */
/* this was the easiest way to get some simple libc interfaces. */
#include "C:\Program Files\UWIN\usr\include\astwin32.h"
#undef _UWIN			/* don't assume ANY _UWIN features in the execution environment */
#endif				/* _POSIX_ */
#endif				/* _UWIN */

#ifndef __CYGWIN__
#if defined(_BLD_dotneato) && defined(_DLL)
#   define external __EXPORT__
#endif
#if !defined(_BLD_dotneato) && defined(__IMPORT__)
#   define external __IMPORT__
#endif
#if defined(GVDLL)
#if !defined(_BLD_gvc)
#define extern	__declspec(dllimport)
#else
#define extern __declspec(dllexport)
#endif
#endif
#endif
/*visual studio*/
#ifdef WIN32_DLL
#ifndef GVC_EXPORTS
#define extern __declspec(dllimport)
#endif
#endif
/*end visual studio*/

#ifndef external
#   define external   extern
#endif
#ifndef EXTERN
#define EXTERN extern
#endif

#ifdef __cplusplus
extern "C" {
#endif

    EXTERN char *Version;
    EXTERN char **Files;	/* from command line */
    EXTERN const char **Lib;		/* from command line */
    EXTERN char *CmdName;
    EXTERN char *specificFlags;
    EXTERN char *specificItems;
    EXTERN char *Gvfilepath;  /* Per-process path of files allowed in image attributes (also ps libs) */
    EXTERN char *Gvimagepath; /* Per-graph path of files allowed in image attributes  (also ps libs) */

    EXTERN unsigned char Verbose;
    EXTERN unsigned char Reduce;
    EXTERN int MemTest;
    EXTERN char *HTTPServerEnVar;
    EXTERN char *Output_file_name;
    EXTERN int graphviz_errors;
    EXTERN int Nop;
    EXTERN double PSinputscale;
    EXTERN int Syntax_errors;
    EXTERN int Show_cnt;
    EXTERN char** Show_boxes;	/* emit code for correct box coordinates */
    EXTERN int CL_type;		/* NONE, LOCAL, GLOBAL */
    EXTERN unsigned char Concentrate;	/* if parallel edges should be merged */
    EXTERN double Epsilon;	/* defined in input_graph */
    EXTERN int MaxIter;
    EXTERN int Ndim;
    EXTERN int State;		/* last finished phase */
    EXTERN int EdgeLabelsDone;	/* true if edge labels have been positioned */
    EXTERN double Initial_dist;
    EXTERN double Damping;
    EXTERN int Y_invert;	/* invert y in dot & plain output */
    EXTERN int GvExitOnUsage;   /* gvParseArgs() should exit on usage or error */

    EXTERN Agsym_t
	*G_activepencolor, *G_activefillcolor,
	*G_selectedpencolor, *G_selectedfillcolor,
	*G_visitedpencolor, *G_visitedfillcolor,
	*G_deletedpencolor, *G_deletedfillcolor,
	*G_ordering, *G_peripheries, *G_penwidth,
	*G_gradientangle, *G_margin;
    EXTERN Agsym_t
	*N_height, *N_width, *N_shape, *N_color, *N_fillcolor,
	*N_activepencolor, *N_activefillcolor,
	*N_selectedpencolor, *N_selectedfillcolor,
	*N_visitedpencolor, *N_visitedfillcolor,
	*N_deletedpencolor, *N_deletedfillcolor,
	*N_fontsize, *N_fontname, *N_fontcolor, *N_margin,
	*N_label, *N_xlabel, *N_nojustify, *N_style, *N_showboxes,
	*N_sides, *N_peripheries, *N_ordering, *N_orientation,
	*N_skew, *N_distortion, *N_fixed, *N_imagescale, *N_layer,
	*N_group, *N_comment, *N_vertices, *N_z,
	*N_penwidth, *N_gradientangle;
    EXTERN Agsym_t
	*E_weight, *E_minlen, *E_color, *E_fillcolor,
	*E_activepencolor, *E_activefillcolor,
	*E_selectedpencolor, *E_selectedfillcolor,
	*E_visitedpencolor, *E_visitedfillcolor,
	*E_deletedpencolor, *E_deletedfillcolor,
	*E_fontsize, *E_fontname, *E_fontcolor,
	*E_label, *E_xlabel, *E_dir, *E_style, *E_decorate,
	*E_showboxes, *E_arrowsz, *E_constr, *E_layer,
	*E_comment, *E_label_float,
	*E_samehead, *E_sametail,
	*E_arrowhead, *E_arrowtail,
	*E_headlabel, *E_taillabel,
	*E_labelfontsize, *E_labelfontname, *E_labelfontcolor,
	*E_labeldistance, *E_labelangle,
	*E_tailclip, *E_headclip,
	*E_penwidth;

    extern struct fdpParms_s* fdp_parms;

#undef external
#undef EXTERN
#ifdef extern
#undef extern
#endif

#ifdef __cplusplus
}
#endif
#endif
