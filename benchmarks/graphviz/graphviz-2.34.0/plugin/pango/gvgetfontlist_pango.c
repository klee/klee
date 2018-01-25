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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* FIXME - the following declaration should be removed
 * when configure is coordinated with flags passed to the
 * compiler. On Linux, strcasestr is defined but needs a special
 * preprocessor constant to be defined. Configure sets the
 * HAVE_STRCASESTR, but the flag is not used during compilation,
 * so strcasestr is undeclared.
 */
char* strcasestr (const char *str, const char *pat);
#ifndef HAVE_STRCASESTR
char* strcasestr (const char *str, const char *pat)
{
    int slen, plen;
    char p0, pc;
    const char *endp, *sp, *pp;
    if (!(p0 = *pat)) return (char*)str;
    plen = strlen (pat++);
    slen = strlen (str);
    if (slen < plen) return NULL;
    endp = str + slen - plen;
    p0 = toupper (p0);
    do {
        while ((str <= endp) && (p0 != toupper(*str))) str++;
        if (str > endp) return NULL;
        pp = pat;
        sp = ++str;
        while ((pc = *pp++) && (toupper(pc) == toupper(*sp))) sp++;
    } while (pc);
    return (char*)(str-1);
}

#endif

#include "agxbuf.h"
#include "gvplugin_textlayout.h"
#ifdef HAVE_PANGOCAIRO
#include <pango/pangocairo.h>
#include "gvgetfontlist.h"
#endif

extern int Verbose;

#define FNT_BOLD	1<<0
#define FNT_BOOK	1<<1
#define FNT_CONDENSED	1<<2
#define FNT_DEMI	1<<3
#define FNT_EXTRALIGHT	1<<4
#define FNT_ITALIC	1<<5
#define FNT_LIGHT	1<<6
#define FNT_MEDIUM	1<<7
#define FNT_OBLIQUE	1<<8
#define FNT_REGULAR	1<<9
#define FNT_ROMAN	1<<9

#define PS_AVANTGARDE "AvantGarde"
#define PS_BOOKMAN "Bookman"
#define PS_COURIER "Courier"
#define PS_HELVETICA SAN_5
#define PS_NEWCENTURYSCHLBK "NewCenturySchlbk"
#define PS_PALATINO "Palatino"
#define PS_SYMBOL "Symbol"
#define PS_TIMES SER_3
#define PS_CHANCERY "ZapfChancery"
#define PS_DINGBATS "ZapfDingbats"

#define FNT_BOLD_ST	"BOLD"
#define FNT_BOOK_ST	"BOOK"
#define FNT_CONDENSED_ST	"CONDENSED"
#define FNT_DEMI_ST	"DEMI"
#define FNT_EXTRALIGHT_ST	"EXTRALIGHT"
#define FNT_ITALIC_ST	"ITALIC"
#define FNT_LIGHT_ST	"LIGHT"
#define FNT_MEDIUM_ST	"MEDIUM"
#define FNT_OBLIQUE_ST	"OBLIQUE"
#define FNT_REGULAR_ST	"REGULAR"
#define FNT_ROMAN_ST	"ROMAN"

#define SAN_0		"sans"
#define SAN_1		"URW Gothic L"
#define SAN_2		"Charcoal"
#define SAN_3		"Nimbus Sans L"
#define SAN_4		"Verdana"
#define SAN_5		"Helvetica"
#define SAN_6		"Bitstream Vera Sans"
#define SAN_7		"DejaVu Sans"
#define SAN_8		"Liberation Sans"
#define SAN_9		"Luxi Sans"
#define SAN_10		"FreeSans"
#define SAN_11		"Arial"

#define SER_0		"serif"
#define SER_1		"URW Bookman L"
#define SER_2		"Times New Roman"
#define SER_3		"Times"
#define SER_4		"Nimbus Roman No9 L"
#define SER_5		"Bitstream Vera Serif"
#define SER_6		"DejaVu Serif"
#define SER_7		"Liberation Serif"
#define SER_8		"Luxi Serif"
#define SER_9		"FreeSerif"
#define SER_10		"Century Schoolbook L"
#define SER_11		"Charcoal"
#define SER_12		"Georgia"
#define SER_13		"URW Palladio L"
#define SER_14		"Norasi"
#define SER_15		"Rekha"
#define SER_16		"URW Chancery L"

#define MON_0		"monospace"
#define MON_1		"Nimbus Mono L"
#define MON_2		"Inconsolata"
#define MON_3		"Courier New"
#define MON_4		"Bitstream Vera Sans Mono"
#define MON_5		"DejaVu Sans Mono"
#define MON_6		"Liberation Mono"
#define MON_7		"Luxi Mono"
#define MON_8		"FreeMono"

#define SYM_0		"fantasy"
#define SYM_1		"Impact"
#define SYM_2		"Copperplate Gothic Std"
#define SYM_3		"Cooper Std"
#define SYM_4		"Bauhaus Std"

#define DING_0		"fantasy"
#define DING_1		"Dingbats"
#define DING_2		"Impact"
#define DING_3		"Copperplate Gothic Std"
#define DING_4		"Cooper Std"
#define DING_5		"Bauhaus Std"


typedef struct {
    int flag;
    char* name;
} face_t;
static face_t facelist[] = {
    { FNT_BOLD, FNT_BOLD_ST}, 
    { FNT_BOOK, FNT_BOOK_ST}, 
    { FNT_CONDENSED, FNT_CONDENSED_ST}, 
    { FNT_DEMI, FNT_DEMI_ST},
    { FNT_EXTRALIGHT, FNT_EXTRALIGHT_ST}, 
    { FNT_ITALIC, FNT_ITALIC_ST}, 
    { FNT_LIGHT, FNT_LIGHT_ST}, 
    { FNT_MEDIUM, FNT_MEDIUM_ST}, 
    { FNT_OBLIQUE, FNT_OBLIQUE_ST}, 
    { FNT_REGULAR, FNT_REGULAR_ST}, 
    { FNT_ROMAN, FNT_ROMAN_ST},
};
#define FACELIST_SZ (sizeof(facelist)/sizeof(face_t))

/* This is where the hierarchy of equivalent fonts is established. The order can be changed
   here or new equivalent fonts can be added here. Each font family used by the Graphviz
   PS fonts is set up.
*/
static const char *PS_AVANT_E[] = { 
    SAN_1, SAN_2, SAN_3, SAN_4, SAN_5, SAN_6, SAN_7, SAN_8, SAN_9, SAN_10
};
#define PS_AVANT_E_SZ  (sizeof(PS_AVANT_E) / sizeof(char *))

static const char *PS_BOOKMAN_E[] = { 
    SER_1, SER_2, SER_3, SER_4, SER_5, SER_6, SER_7, SER_8, SER_9 
};
#define PS_BOOKMAN_E_SZ (sizeof(PS_BOOKMAN_E) / sizeof(char *))

static const char *PS_COURIER_E[] = { 
    MON_1, MON_2, MON_3, MON_4, MON_5, MON_6, MON_7, MON_8 
};
#define PS_COURIER_E_SZ (sizeof(PS_COURIER_E) / sizeof(char *))

static const char *PS_HELVETICA_E[] = { 
    SAN_3, SAN_11, SAN_4, SAN_6, SAN_7, SAN_8, SAN_9, SAN_10 
};
#define PS_HELVETICA_E_SZ (sizeof(PS_HELVETICA_E) / sizeof(char *))

static const char *PS_NEWCENT_E[] = { 
    SER_10, SER_2, SER_3, SER_4, SER_12, SER_5, SER_6, SER_7, SER_8, SER_9
};
#define PS_NEWCENT_E_SZ (sizeof(PS_NEWCENT_E) / sizeof(char *))

static const char *PS_PALATINO_E[] = { 
    SER_13, SER_2, SER_3, SER_4, SER_14, SER_15, SER_5, SER_6, SER_7, SER_8, SER_9
};
#define PS_PALATINO_E_SZ (sizeof(PS_PALATINO_E) / sizeof(char *))

static const char *PS_TIMES_E[] = { 
    SER_4, SER_2, SER_11, SER_5, SER_6, SER_7, SER_8, SER_9 
};
#define PS_TIMES_E_SZ (sizeof(PS_TIMES_E) / sizeof(char *))

static const char *PS_SYMBOL_E[] = { SYM_1, SYM_2, SYM_3, SYM_4 };
#define PS_SYMBOL_E_SZ (sizeof(PS_SYMBOL_E) / sizeof(char *))

static const char *PS_CHANCERY_E[] = { 
    SER_16, SER_11, SER_2, SER_3, SER_4, SER_5, SER_6, SER_7, SER_8, SER_9
};
#define PS_CHANCERY_E_SZ (sizeof(PS_CHANCERY_E) / sizeof(char *))

static const char *PS_DINGBATS_E[] = { DING_1, SYM_1, SYM_2, SYM_3, SYM_4 };
#define PS_DINGBATS_E_SZ (sizeof(PS_DINGBATS_E) / sizeof(char *))

typedef struct {
    char *generic_name;
    char *fontname;
    int eq_sz;
    const char **equiv;
} fontdef_t;

/* array of recognized Graphviz PS font names */
static fontdef_t gv_ps_fontdefs[] = {
  { SAN_0, PS_AVANTGARDE, PS_AVANT_E_SZ, PS_AVANT_E},
  { SER_0, PS_BOOKMAN, PS_BOOKMAN_E_SZ, PS_BOOKMAN_E},
  { MON_0, PS_COURIER, PS_COURIER_E_SZ, PS_COURIER_E},
  { SAN_0, PS_HELVETICA, PS_HELVETICA_E_SZ, PS_HELVETICA_E},
  { SER_0, PS_NEWCENTURYSCHLBK, PS_NEWCENT_E_SZ, PS_NEWCENT_E},
  { SER_0, PS_PALATINO, PS_PALATINO_E_SZ, PS_PALATINO_E},
  { SYM_0, PS_SYMBOL, PS_SYMBOL_E_SZ, PS_SYMBOL_E},
  { SER_0, PS_TIMES, PS_TIMES_E_SZ, PS_TIMES_E},
  { SER_0, PS_CHANCERY, PS_CHANCERY_E_SZ, PS_CHANCERY_E},
  { DING_0, PS_DINGBATS, PS_DINGBATS_E_SZ, PS_DINGBATS_E},
};
#define GV_FONT_LIST_SIZE (sizeof(gv_ps_fontdefs)/sizeof(fontdef_t))

typedef struct {
    char *gv_ps_fontname;
    char *fontname;
    int faces;
} availfont_t;

#define NEW(t)          (t*)malloc(sizeof(t))
#define N_NEW(n,t)      (t*)malloc((n)*sizeof(t))

static PostscriptAlias postscript_alias[] = {
#include "ps_font_equiv.h"
};

/* Frees memory used by the available system font definitions */
static void gv_flist_free_af(availfont_t* gv_af_p)
{
    int i;

    for (i = 0; i < GV_FONT_LIST_SIZE; i++) {
	if (gv_af_p[i].fontname)
	    free(gv_af_p[i].fontname);
    }
    free(gv_af_p);
}

static int get_faces(PangoFontFamily * family)
{
    PangoFontFace **faces;
    PangoFontFace *face;
    int i, j, n_faces;
    const char *name;
    int availfaces = 0;
    /* Get the faces (Bold, Italic, etc.) for the current font family */
    pango_font_family_list_faces(family, &faces, &n_faces);
    for (i = 0; i < n_faces; i++) {
	face = faces[i];
	name = pango_font_face_get_face_name(face);

	/* if the family face type is one of the known types, logically OR the known type value
	   to the available faces integer */
	for (j = 0; j < FACELIST_SZ; j++) {
	    if (strcasestr(name, facelist[j].name)) {
		availfaces |= facelist[j].flag;
		break;
	    }
	}
    }
    g_free(faces);
    return availfaces;
}

#ifdef DEBUG
static void 
display_available_fonts(availfont_t* gv_af_p)
{
    int i, j, faces;

/* Displays the Graphviz PS font name, system available font name and associated faces */
    for (j = 0; j < GV_FONT_LIST_SIZE; j++) {
	if ((gv_af_p[j].faces == 0) || (gv_af_p[j].fontname == NULL)) {
	    fprintf (stderr, "ps font = %s not available\n", gv_ps_fontdefs[j].fontname);
	    continue;
	}
	fprintf (stderr, "ps font = %s available %d font = %s\n",
	    gv_ps_fontdefs[j].fontname, gv_af_p[j].faces, gv_af_p[j].fontname);
	faces = gv_af_p[j].faces;
	for (i = 0; i < FACELIST_SZ; i++) {
	    if (faces & facelist[i].flag)
		fprintf (stderr, "\t%s\n", facelist[i].name);
	}
    }
}
#endif

/* Construct the list of font faces */
static char *get_avail_faces(int faces, agxbuf* xb)
{
    int i;
    for (i = 0; i < FACELIST_SZ; i++) {
	if (faces & facelist[i].flag) {
	    agxbput (xb, facelist[i].name); 
	    agxbputc(xb, ' ');
	}
    }
    return agxbuse (xb);
}


/* This function creates an array of font definitions. Each entry corresponds to one of 
   the Graphviz PS fonts.  The font definitions contain the generic font name and a list 
   of equivalent fonts that can be used in place of the PS font if the PS font is not 
   available on the system
*/
static availfont_t *gv_get_ps_fontlist(PangoFontMap * fontmap)
{
    PangoFontFamily **families;
    PangoFontFamily *family;
    fontdef_t* gv_ps_fontdef;
    int n_families;
    int i, j, k, array_sz, availfaces;
    availfont_t *gv_af_p, *gv_afs;
    const char *name;
    char *family_name;

    /* Get a list of font families installed on the system */
    pango_font_map_list_families(fontmap, &families, &n_families);

    /* Setup a pointer to available font structs */
    gv_af_p = N_NEW(GV_FONT_LIST_SIZE, availfont_t);

    for (j = 0; j < GV_FONT_LIST_SIZE; j++) {
	/* get the Graphviz PS font information and create the
	   available font definition structs */
	gv_afs = gv_af_p+j;
	gv_ps_fontdef = gv_ps_fontdefs+j;
	gv_afs->gv_ps_fontname = gv_ps_fontdef->fontname;
	family_name = NULL;
	/* Search the installed system font families for the current 
	   Graphvis PS font family name, i.e. AvantGarde */
	for (i = 0; i < n_families; i++) {
	    family = families[i];
	    name = pango_font_family_get_name(family);
	    /* if a match is found get the installed font faces */
	    if (strcasecmp(gv_ps_fontdef->fontname, name) == 0) {
		family_name = strdup(name);
		availfaces = get_faces(family);
	    }
	    if (family_name)
		break;
	}
	/* if a match is not found on the primary Graphviz font family,
	   search for a match on the equivalent font family names */
	if (!family_name) {
	    array_sz = gv_ps_fontdef->eq_sz;
	    for (k = 0; k < array_sz; k++) {
		for (i = 0; i < n_families; i++) {
		    family = families[i];
		    name = pango_font_family_get_name(family);
		    if (strcasecmp(gv_ps_fontdef->equiv[k], name) == 0) {
			family_name = strdup(name);
			availfaces = get_faces(family);
			break;
		    }
		}
		if (family_name)
		    break;
	    }
	}
	/* if a match is not found on the equivalent font family names, search
	   for a match on the generic family name assigned to the Graphviz PS font */
	if (!family_name) {
	    for (i = 0; i < n_families; i++) {
		family = families[i];
		name = pango_font_family_get_name(family);
		if (strcasecmp(gv_ps_fontdef->generic_name, name) == 0) {
		    family_name = strdup(name);
		    availfaces = get_faces(family);
		    break;
		}
	    }
	}
	/* if not match is found on the generic name, set the available font
	   name to NULL */
	if (family_name && availfaces) {
	    gv_afs->fontname = family_name;
	    gv_afs->faces = availfaces;
	} else {
	    gv_afs->fontname = NULL;
	    gv_afs->faces = 0;
	}
    }
    g_free(families);
#ifdef DEBUG
    display_available_fonts(gv_af_p);
#endif
/* Free the Graphviz PS font definitions */
    return (gv_af_p);
}

static void copyUpper (agxbuf* xb, char* s)
{
    int c;

    while ((c = *s++))
	(void)agxbputc (xb, toupper(c));
}

/* Returns the font corresponding to a Graphviz PS font. 
   AvantGarde-Book may return URW Gothic L, book
   Returns NULL if no appropriate font found.
*/
static char *gv_get_font(availfont_t* gv_af_p,
		  PostscriptAlias * ps_alias, agxbuf* xb, agxbuf *xb2)
{
    char *avail_faces;
    int i;

    for (i = 0; i < GV_FONT_LIST_SIZE; i++) {
	/* Searches the array of available system fonts for the one that
	   corresponds to the current Graphviz PS font name. Sets up the
	   font string with the available font name and the installed font 
	   faces that match what are required by the Graphviz PS font.
	 */
	if (gv_af_p[i].faces && strstr(ps_alias->name, gv_af_p[i].gv_ps_fontname)) {
	    agxbput(xb2, gv_af_p[i].fontname);
	    agxbput(xb2, ", ");
	    avail_faces = get_avail_faces(gv_af_p[i].faces, xb);
	    if (ps_alias->weight) {
		if (strcasestr(avail_faces, ps_alias->weight)) {
		    agxbputc(xb2, ' ');
		    copyUpper(xb2, ps_alias->weight);
		}
	    } else if (strcasestr(avail_faces, "REGULAR")) {
		agxbputc(xb2, ' ');
		agxbput(xb2, "REGULAR");
	    } else if (strstr(avail_faces, "ROMAN")) {
		agxbputc(xb2, ' ');
		agxbput(xb2, "ROMAN");
	    }
	    if (ps_alias->stretch) {
		if (strcasestr(avail_faces, ps_alias->stretch)) {
		    agxbputc(xb2, ' ');
		    copyUpper(xb2, ps_alias->stretch);
		}
	    }
	    if (ps_alias->style) {
		if (strcasestr(avail_faces, ps_alias->style)) {
		    agxbputc(xb2, ' ');
		    copyUpper(xb2, ps_alias->style);
		} else if (!strcasecmp(ps_alias->style, "ITALIC")) {
                    /* try to use ITALIC in place of OBLIQUE & visa versa */
		    if (strcasestr(avail_faces, "OBLIQUE")) {
			agxbputc(xb2, ' ');
			agxbput(xb2, "OBLIQUE");
		    }
		} else if (!strcasecmp(ps_alias->style, "OBLIQUE")) {
		    if (strcasestr(avail_faces, "ITALIC")) {
			agxbputc(xb2, ' ');
			agxbput(xb2, "ITALIC");
		    }
		}
	    }
	    return strdup(agxbuse(xb2));
	}
    }
    return NULL;
}

static void
printFontMap (gv_font_map*gv_fmap, int sz)
{
    int j;
    char* font;

    for (j = 0; j < sz; j++) {
	font = gv_fmap[j].gv_font;
	if (!font)
	    fprintf (stderr, " [%d] %s => <Not available>\n", j, gv_fmap[j].gv_ps_fontname);
	else
	    fprintf (stderr, " [%d] %s => \"%s\"\n", j, gv_fmap[j].gv_ps_fontname, font);
    }
}

/* Sets up a structure array that contains the Graphviz PS font name
   and the corresponding installed font string.  
*/
gv_font_map* get_font_mapping(PangoFontMap * fontmap)
{
    PostscriptAlias *ps_alias;
    availfont_t *gv_af_p;
    int j, ps_fontnames_sz = sizeof(postscript_alias) / sizeof(PostscriptAlias);
    gv_font_map* gv_fmap = N_NEW(ps_fontnames_sz, gv_font_map);
    agxbuf xb;
    agxbuf xb2;
    unsigned char buf[BUFSIZ];
    unsigned char buf2[BUFSIZ];

    agxbinit(&xb, BUFSIZ, buf);
    agxbinit(&xb2, BUFSIZ, buf2);
    gv_af_p = gv_get_ps_fontlist(fontmap);	// get the available installed fonts
    /* add the Graphviz PS font name and available system font string to the array */
    for (j = 0; j < ps_fontnames_sz; j++) {
	ps_alias = &postscript_alias[j];
	gv_fmap[ps_alias->xfig_code].gv_ps_fontname = ps_alias->name;
	gv_fmap[ps_alias->xfig_code].gv_font = gv_get_font(gv_af_p, ps_alias, &xb, &xb2);
    }
    gv_flist_free_af(gv_af_p);
    agxbfree(&xb);
    agxbfree(&xb2);
#ifndef WIN32
    if (Verbose > 1)
	printFontMap (gv_fmap, ps_fontnames_sz);
#endif
    return gv_fmap;
}

/* Returns a list of the fonts that are available for use

*/

void get_font_list(char **fonts[], int *cnt){

PangoFontMap *fontmap;
availfont_t *gv_af_p;
int j, i;
char **fontlist;
fontlist = N_NEW(GV_FONT_LIST_SIZE,char *);
fontmap = pango_cairo_font_map_new();
gv_af_p = gv_get_ps_fontlist(fontmap);	// get the available installed fonts
g_object_unref(fontmap);
/* load array with available font names */
i=0;
for (j = 0; j < GV_FONT_LIST_SIZE; j++) {
	*(fontlist + j) = 0;
	if ((gv_af_p[j].faces == 0) || (gv_af_p[j].fontname == NULL)) {
	    continue;
	}
	*(fontlist + i++) = strdup(gv_af_p[j].fontname);
}
/* Free unused array elements */
for(j=i;j<GV_FONT_LIST_SIZE;j++){
    free(*(fontlist + j));
}
/* Free available fonts structure */
gv_flist_free_af(gv_af_p);

*cnt = i;
*fonts = fontlist;
return;
}
