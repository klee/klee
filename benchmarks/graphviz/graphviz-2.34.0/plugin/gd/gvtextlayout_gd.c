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
#include <stdlib.h>
#include <string.h>
#include "gvplugin_textlayout.h"

#ifdef HAVE_LIBGD
#include "gd.h"

#if defined(HAVE_LIBGD) && defined(HAVE_GD_FREETYPE)

/* fontsize at which text is omitted entirely */
#define FONTSIZE_MUCH_TOO_SMALL 0.15
/* fontsize at which text is rendered by a simple line */
#define FONTSIZE_TOO_SMALL 1.5

#ifndef HAVE_GD_FONTCONFIG
/* gd_alternate_fontlist;
 * Sometimes fonts are stored under a different name,
 * especially on Windows. Without fontconfig, we provide
 * here some rudimentary name mapping.
 */
char *gd_alternate_fontlist(char *font)
{
    static char *fontbuf;
    static int fontbufsz;
    char *p, *fontlist;
    int len;

    len = strlen(font) + 1;
    if (len > fontbufsz) {
	fontbufsz = 2 * len;
	if (fontbuf == NULL)
	    fontbuf = malloc(fontbufsz);
	else
	    fontbuf = realloc(fontbuf, fontbufsz);
    }

    /* fontbuf to contain font without style descriptions like -Roman or -Italic */
    strcpy(fontbuf, font);
    if ((p = strchr(fontbuf, '-')) || (p = strchr(fontbuf, '_')))
	*p = 0;

    fontlist = fontbuf;
    if ((strcasecmp(font, "times-bold") == 0)
	|| (strcasecmp(fontbuf, "timesbd") == 0)
	|| (strcasecmp(fontbuf, "timesb") == 0))
	fontlist = "timesbd;Timesbd;TIMESBD;timesb;Timesb;TIMESB";

    else if ((strcasecmp(font, "times-italic") == 0)
	     || (strcasecmp(fontbuf, "timesi") == 0))
	fontlist = "timesi;Timesi;TIMESI";

    else if ((strcasecmp(font, "timesnewroman") == 0)
	     || (strcasecmp(font, "timesnew") == 0)
	     || (strcasecmp(font, "timesroman") == 0)
	     || (strcasecmp(fontbuf, "times") == 0))
	fontlist = "times;Times;TIMES";

    else if ((strcasecmp(font, "arial-bold") == 0)
	     || (strcasecmp(fontbuf, "arialb") == 0))
	fontlist = "arialb;Arialb;ARIALB";

    else if ((strcasecmp(font, "arial-italic") == 0)
	     || (strcasecmp(fontbuf, "ariali") == 0))
	fontlist = "ariali;Ariali;ARIALI";

    else if (strcasecmp(fontbuf, "helvetica") == 0)
	fontlist = "helvetica;Helvetica;HELVETICA;arial;Arial;ARIAL";

    else if (strcasecmp(fontbuf, "arial") == 0)
	fontlist = "arial;Arial;ARIAL";

    else if (strcasecmp(fontbuf, "courier") == 0)
	fontlist = "courier;Courier;COURIER;cour";

    return fontlist;
}
#endif				/* HAVE_GD_FONTCONFIG */

/* gd_psfontResolve:
 *  * Construct alias for postscript fontname.
 *   * NB. Uses a static array - non-reentrant.
 *    */

#define ADD_ATTR(a) \
  if (a) { \
        strcat(buf, comma ? " " : ", "); \
        comma = 1; \
        strcat(buf, a); \
  }

char* gd_psfontResolve (PostscriptAlias* pa)
{
    static char buf[1024];
    int comma=0;
    strcpy(buf, pa->family);

    ADD_ATTR(pa->weight);
    ADD_ATTR(pa->stretch);
    ADD_ATTR(pa->style);
   
    return buf;
}

static boolean gd_textlayout(textpara_t * para, char **fontpath)
{
    char *err;
    char *fontlist;
    int brect[8];
    gdFTStringExtra strex;
    double fontsize;

    strex.fontpath = NULL;
    strex.flags = gdFTEX_RETURNFONTPATHNAME | gdFTEX_RESOLUTION;
    strex.hdpi = strex.vdpi = POINTS_PER_INCH;

    if (strstr(para->fontname, "/"))
	strex.flags |= gdFTEX_FONTPATHNAME;
    else
	strex.flags |= gdFTEX_FONTCONFIG;

    para->width = 0.0;
    para->height = 0.0;
    para->yoffset_layout = 0.0;

    para->layout = NULL;
    para->free_layout = NULL;

    fontsize = para->fontsize;
    para->yoffset_centerline = 0.1 * fontsize;

    if (para->fontname) {
	if (fontsize <= FONTSIZE_MUCH_TOO_SMALL) {
	    return TRUE; /* OK, but ignore text entirely */
	} else if (fontsize <= FONTSIZE_TOO_SMALL) {
	    /* draw line in place of text */
	    /* fake a finite fontsize so that line length is calculated */
	    fontsize = FONTSIZE_TOO_SMALL;
	}
	/* call gdImageStringFT with null *im to get brect and to set font cache */
#ifdef HAVE_GD_FONTCONFIG
	gdFTUseFontConfig(1);  /* tell gd that we really want to use fontconfig, 'cos it s not the default */
	if (para->postscript_alias)
	    fontlist = gd_psfontResolve (para->postscript_alias);
	else
	    fontlist = para->fontname;
#else
	fontlist = gd_alternate_fontlist(para->fontname);
#endif

	err = gdImageStringFTEx(NULL, brect, -1, fontlist,
				fontsize, 0, 0, 0, para->str, &strex);

	if (err) {
	    agerr(AGERR,"%s\n", err);
	    return FALSE; /* indicate error */
	}

	if (fontpath)
	    *fontpath = strex.fontpath;
	else
	    free (strex.fontpath); /* strup'ed in libgd */

	if (para->str && para->str[0]) {
	    /* can't use brect on some archtectures if strlen 0 */
	    para->width = (double) (brect[4] - brect[0]);
	    /* 1.2 specifies how much extra space to leave between lines;
             * see LINESPACING in const.h.
             */
	    para->height = (int)(para->fontsize * 1.2);
	}
    }
    return TRUE;
}

static gvtextlayout_engine_t gd_textlayout_engine = {
    gd_textlayout,
};
#endif
#endif

gvplugin_installed_t gvtextlayout_gd_types[] = {
#if defined(HAVE_LIBGD) && defined(HAVE_GD_FREETYPE)
    {0, "textlayout", 2, &gd_textlayout_engine, NULL},
#endif
    {0, NULL, 0, NULL, NULL}
};
