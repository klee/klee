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

#ifndef TEXTPARA_H
#define TEXTPARA_H

#ifdef __cplusplus
extern "C" {
#endif

/* Bold, Italic, Underline */
#define HTML_BF 1
#define HTML_IF 2
#define HTML_UL 4
#define HTML_SUP 8
#define HTML_SUB 16

    /* font information
     * If name or color is NULL, or size < 0, that attribute
     * is unspecified. 
     */
    typedef struct {
	char*  name;
	char*  color;
        int    flags:7;  /* HTML_UL, HTML_IF, HTML_BF */
	int    cnt;   /* reference count */
	double size;
    } htmlfont_t;

    typedef struct _PostscriptAlias {
        char* name;
        char* family;
        char* weight;
        char* stretch;
        char* style;
        int xfig_code;
	char* svg_font_family;
	char* svg_font_weight;
	char* svg_font_style;
    } PostscriptAlias;

    /* atomic unit of text emitted using a single htmlfont_t */
    typedef struct {
	char *str;      /* stored in utf-8 */
	PostscriptAlias *postscript_alias;
	void *layout;
	void (*free_layout) (void *layout);   /* FIXME - this is ugly */
	htmlfont_t *font;
	char *fontname; /* FIXME - use htmlfont_t */
	double fontsize; /* FIXME - use htmlfont_t */
	double size, yoffset_layout, yoffset_centerline;
	double width, height; /* FIXME */
	char just;	/* 'l' 'n' 'r' */ /* FIXME */
    } textpara_t;

#ifdef __cplusplus
}
#endif
#endif
