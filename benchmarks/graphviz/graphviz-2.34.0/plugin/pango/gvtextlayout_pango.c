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

#include <stdlib.h>
#include <string.h>
#include "gvplugin_render.h"
#include "agxbuf.h"
#include "utils.h"
#include "gvplugin_textlayout.h"

#ifdef HAVE_PANGOCAIRO
#include <pango/pangocairo.h>
#include "gvgetfontlist.h"
#ifdef HAVE_PANGO_FC_FONT_LOCK_FACE
#include <pango/pangofc-font.h>
#endif

#define N_NEW(n,t)      (t*)malloc((n)*sizeof(t))

static void pango_free_layout (void *layout)
{
    g_object_unref((PangoLayout*)layout);
}

static char* pango_psfontResolve (PostscriptAlias* pa)
{
    static char buf[1024];
    strcpy(buf, pa->family);
    strcat(buf, ",");
    if (pa->weight) {
        strcat(buf, " ");
        strcat(buf, pa->weight);
    }
    if (pa->stretch) {
        strcat(buf, " ");
        strcat(buf, pa->stretch);
    }
    if (pa->style) {
        strcat(buf, " ");
        strcat(buf, pa->style);
    }
    return buf;
}

#define FONT_DPI 96.

#define ENABLE_PANGO_MARKUP
#ifdef ENABLE_PANGO_MARKUP
#define FULL_MARKUP "<span weight=\"bold\" style=\"italic\" underline=\"single\"><sup><sub></sub></sup></span>"
#endif

static boolean pango_textlayout(textpara_t * para, char **fontpath)
{
    static char buf[1024];  /* returned in fontpath, only good until next call */
    static PangoFontMap *fontmap;
    static PangoContext *context;
    static PangoFontDescription *desc;
    static char *fontname;
    static double fontsize;
    static gv_font_map* gv_fmap;
    char *fnt, *psfnt = NULL;
    PangoLayout *layout;
    PangoRectangle logical_rect;
    cairo_font_options_t* options;
    PangoFont *font;
#ifdef ENABLE_PANGO_MARKUP
    PangoAttrList *attrs;
    GError *error = NULL;
    int flags;
#endif
    char *text;
    double textlayout_scale;

    if (!context) {
	fontmap = pango_cairo_font_map_new();
	gv_fmap = get_font_mapping(fontmap);
	context = pango_cairo_font_map_create_context (PANGO_CAIRO_FONT_MAP(fontmap));
	options=cairo_font_options_create();
	cairo_font_options_set_antialias(options,CAIRO_ANTIALIAS_GRAY);
	cairo_font_options_set_hint_style(options,CAIRO_HINT_STYLE_FULL);
	cairo_font_options_set_hint_metrics(options,CAIRO_HINT_METRICS_ON);
	cairo_font_options_set_subpixel_order(options,CAIRO_SUBPIXEL_ORDER_BGR);
	pango_cairo_context_set_font_options(context, options);
	pango_cairo_context_set_resolution(context, FONT_DPI);
	cairo_font_options_destroy(options);
	g_object_unref(fontmap);
    }

    if (!fontname || strcmp(fontname, para->fontname) != 0 || fontsize != para->fontsize) {
	fontname = para->fontname;
	fontsize = para->fontsize;
	pango_font_description_free (desc);

	if (para->postscript_alias) {
	    psfnt = fnt = gv_fmap[para->postscript_alias->xfig_code].gv_font;
	    if(!psfnt)
		psfnt = fnt = pango_psfontResolve (para->postscript_alias);
	}
	else
	    fnt = fontname;

	desc = pango_font_description_from_string(fnt);
        /* all text layout is done at a scale of FONT_DPI (nominaly 96.) */
        pango_font_description_set_size (desc, (gint)(fontsize * PANGO_SCALE));

        if (fontpath && (font = pango_font_map_load_font(fontmap, context, desc))) {  /* -v support */
	    const char *fontclass;

	    fontclass = G_OBJECT_CLASS_NAME(G_OBJECT_GET_CLASS(font));

	    buf[0] = '\0';
	    if (psfnt) {
		strcat(buf, "(ps:pango  ");
		strcat(buf, psfnt);
		strcat(buf, ") ");
	    }
	    strcat(buf, "(");
	    strcat(buf, fontclass);
	    strcat(buf, ") ");
#ifdef HAVE_PANGO_FC_FONT_LOCK_FACE
	    if (strcmp(fontclass, "PangoCairoFcFont") == 0) {
	        FT_Face face;
	        PangoFcFont *fcfont;
	        FT_Stream stream;
	        FT_StreamDesc streamdesc;
	        fcfont = PANGO_FC_FONT(font);
	        face = pango_fc_font_lock_face(fcfont);
	        if (face) {
		    strcat(buf, "\"");
		    strcat(buf, face->family_name);
		    strcat(buf, ", ");
		    strcat(buf, face->style_name);
		    strcat(buf, "\" ");
    
		    stream = face->stream;
		    if (stream) {
			streamdesc = stream->pathname;
			if (streamdesc.pointer)
			    strcat(buf, (char*)streamdesc.pointer);
		        else
			    strcat(buf, "*no pathname available*");
		    }
		    else
			strcat(buf, "*no stream available*");
		}
	        pango_fc_font_unlock_face(fcfont);
	    }
	    else
#endif
	    {
    		PangoFontDescription *tdesc;
		char *tfont;
		
	        tdesc = pango_font_describe(font);
	        tfont = pango_font_description_to_string(tdesc);
	        strcat(buf, "\"");
	        strcat(buf, tfont);
	        strcat(buf, "\" ");
	        g_free(tfont);
	    }
            *fontpath = buf;
        }
    }

#ifdef ENABLE_PANGO_MARKUP
    if ((para->font) && (flags = para->font->flags)) {
	unsigned char buf[BUFSIZ];
	agxbuf xb;

	agxbinit(&xb, BUFSIZ, buf);
	agxbput(&xb,"<span");

	if (flags & HTML_BF)
	    agxbput(&xb," weight=\"bold\"");
	if (flags & HTML_IF)
	    agxbput(&xb," style=\"italic\"");
	if (flags & HTML_UL)
	    agxbput(&xb," underline=\"single\"");
	agxbput (&xb,">");

	if (flags & HTML_SUP)
	    agxbput(&xb,"<sup>");
	if (flags & HTML_SUB)
	    agxbput(&xb,"<sub>");

	agxbput (&xb,xml_string(para->str));

	if (flags & HTML_SUB)
	    agxbput(&xb,"</sub>");
	if (flags & HTML_SUP)
	    agxbput(&xb,"</sup>");

	agxbput (&xb,"</span>");
	if (!pango_parse_markup (agxbuse(&xb), -1, 0, &attrs, &text, NULL, &error)) {
	    fprintf (stderr, "Error - pango_parse_markup: %s\n", error->message);
	    text = para->str;
	    attrs = NULL;
	}
	agxbfree (&xb);
    }
    else {
	text = para->str;
	attrs = NULL;
    }
#else
    text = para->str;
#endif

    layout = pango_layout_new (context);
    para->layout = (void *)layout;    /* layout free with textpara - see labels.c */
    para->free_layout = pango_free_layout;    /* function for freeing pango layout */

    pango_layout_set_text (layout, text, -1);
    pango_layout_set_font_description (layout, desc);
#ifdef ENABLE_PANGO_MARKUP
    if (attrs)
	pango_layout_set_attributes (layout, attrs);
#endif

    pango_layout_get_extents (layout, NULL, &logical_rect);

    /* if pango doesn't like the font then it sets width=0 but height = garbage */
    if (logical_rect.width == 0)
	logical_rect.height = 0;

    textlayout_scale = POINTS_PER_INCH / (FONT_DPI * PANGO_SCALE);
    para->width = (int)(logical_rect.width * textlayout_scale + 1);    /* round up so that width/height are never too small */
    para->height = (int)(logical_rect.height * textlayout_scale + 1);

    /* FIXME  -- Horrible kluge !!! */

    /* For now we are using pango for single line blocks only.
     * The logical_rect.height seems to be too high from the font metrics on some platforms.
     * Use an assumed height based on the point size.
     */

    para->height = (int)(para->fontsize * 1.1 + .5);

    /* The y offset from baseline to 0,0 of the bitmap representation */
#if defined PANGO_VERSION_MAJOR && (PANGO_VERSION_MAJOR >= 1)
    para->yoffset_layout = pango_layout_get_baseline (layout) * textlayout_scale;
#else
    {
	/* do it the hard way on rhel5/centos5 */
	PangoLayoutIter *iter = pango_layout_get_iter (layout);
	para->yoffset_layout = pango_layout_iter_get_baseline (iter) * textlayout_scale;
    }
#endif

    /* The distance below midline for y centering of text strings */
    para->yoffset_centerline = 0.2 * para->fontsize;

    if (logical_rect.width == 0)
	return FALSE;
    return TRUE;
}

static gvtextlayout_engine_t pango_textlayout_engine = {
    pango_textlayout,
};
#endif

gvplugin_installed_t gvtextlayout_pango_types[] = {
#ifdef HAVE_PANGOCAIRO
    {0, "textlayout", 10, &pango_textlayout_engine, NULL},
#endif
    {0, NULL, 0, NULL, NULL}
};
