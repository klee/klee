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

#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>

#ifdef WIN32
#include <windows.h>
#define GLOB_NOSPACE    1   /* Ran out of memory.  */
#define GLOB_ABORTED    2   /* Read error.  */
#define GLOB_NOMATCH    3   /* No matches found.  */
#define GLOB_NOSORT     4
#define DMKEY "Software\\Microsoft" //key to look for library dir
#include "regex_win32.h"
#else
#include <regex.h>
#endif

#include "types.h"
#include "logic.h"
#include "memory.h"
#include "agxbuf.h"

#define _BLD_gvc 1
#include "utils.h"
#include "gvplugin_loadimage.h"

extern char *Gvimagepath;
extern char *HTTPServerEnVar;
extern shape_desc *find_user_shape(const char *);

static Dict_t *ImageDict;

typedef struct {
    char *template;
    int size;
    int type;
    char *stringtype;
} knowntype_t;

#define HDRLEN 20

#define PNG_MAGIC  "\x89PNG\x0D\x0A\x1A\x0A"
#define PS_MAGIC   "%!PS-Adobe-"
#define BMP_MAGIC  "BM"
#define GIF_MAGIC  "GIF8"
#define JPEG_MAGIC "\xFF\xD8\xFF\xE0"
#define PDF_MAGIC  "%PDF-"
#define EPS_MAGIC  "\xC5\xD0\xD3\xC6"
#define XML_MAGIC  "<?xml"
#define SVG_MAGIC  "<svg"
#define RIFF_MAGIC  "RIFF"
#define WEBP_MAGIC  "WEBP"

static knowntype_t knowntypes[] = {
    { PNG_MAGIC,  sizeof(PNG_MAGIC)-1,   FT_PNG,  "png",  },
    { PS_MAGIC,   sizeof(PS_MAGIC)-1,    FT_PS,   "ps",   },
    { BMP_MAGIC,  sizeof(BMP_MAGIC)-1,   FT_BMP,  "bmp",  },
    { GIF_MAGIC,  sizeof(GIF_MAGIC)-1,   FT_GIF,  "gif",  },
    { JPEG_MAGIC, sizeof(JPEG_MAGIC)-1,  FT_JPEG, "jpeg", },
    { PDF_MAGIC,  sizeof(PDF_MAGIC)-1,   FT_PDF,  "pdf",  },
    { EPS_MAGIC,  sizeof(EPS_MAGIC)-1,   FT_EPS,  "eps",  },
/*    { SVG_MAGIC,  sizeof(SVG_MAGIC)-1,  FT_SVG,  "svg",  },  - viewers expect xml preamble */
    { XML_MAGIC,  sizeof(XML_MAGIC)-1,   FT_XML,  "xml",  },
    { RIFF_MAGIC,  sizeof(RIFF_MAGIC)-1, FT_RIFF, "riff", },
};

static int imagetype (usershape_t *us)
{
    char header[HDRLEN];
    char line[200];
    int i;

    if (us->f && fread(header, 1, HDRLEN, us->f) == HDRLEN) {
        for (i = 0; i < sizeof(knowntypes) / sizeof(knowntype_t); i++) {
	    if (!memcmp (header, knowntypes[i].template, knowntypes[i].size)) {
	        us->stringtype = knowntypes[i].stringtype;
		us->type = knowntypes[i].type;
		if (us->type == FT_XML) {
		    /* check for SVG in case of XML */
		    while (fgets(line, sizeof(line), us->f) != NULL) {
		        if (!memcmp(line, SVG_MAGIC, sizeof(SVG_MAGIC)-1)) {
    			    us->stringtype = "svg";
			    return (us->type = FT_SVG);
		        }
		    }
		}
	    	else if (us->type == FT_RIFF) {
		    /* check for WEBP in case of RIFF */
		    if (!memcmp(header+8, WEBP_MAGIC, sizeof(WEBP_MAGIC)-1)) {
    			us->stringtype = "webp";
			return (us->type = FT_WEBP);
		    }
		}
		return us->type;
	    }
        }
    }

    us->stringtype = "(lib)";
    us->type = FT_NULL;

    return FT_NULL;
}
    
static boolean get_int_lsb_first (FILE *f, unsigned int sz, unsigned int *val)
{
    int ch, i;

    *val = 0;
    for (i = 0; i < sz; i++) {
	ch = fgetc(f);
	if (feof(f))
	    return FALSE;
	*val |= (ch << 8*i);
    }
    return TRUE;
}
	
static boolean get_int_msb_first (FILE *f, unsigned int sz, unsigned int *val)
{
    int ch, i;

    *val = 0;
    for (i = 0; i < sz; i++) {
	ch = fgetc(f);
	if (feof(f))
	    return FALSE;
        *val <<= 8;
	*val |= ch;
    }
    return TRUE;
}

static unsigned int svg_units_convert(double n, char *u)
{
    if (strcmp(u, "in") == 0)
	return ROUND(n * POINTS_PER_INCH);
    if (strcmp(u, "px") == 0)
        return ROUND(n * POINTS_PER_INCH / 96);
    if (strcmp(u, "pc") == 0)
        return ROUND(n * POINTS_PER_INCH / 6); 
    if (strcmp(u, "pt") == 0 || strcmp(u, "\"") == 0)   /* ugly!!  - if there are no inits then the %2s get the trailing '"' */
        return ROUND(n);
    if (strcmp(u, "cm") == 0)
        return ROUND(n * POINTS_PER_CM);
    if (strcmp(u, "mm") == 0)
        return ROUND(n * POINTS_PER_MM);
    return 0;
}

static char* svg_attr_value_re = "([a-z][a-zA-Z]*)=\"([^\"]*)\"";
static regex_t re, *pre = NULL;

static void svg_size (usershape_t *us)
{
    unsigned int w = 0, h = 0;
    double n, x0, y0, x1, y1;
    char u[10];
    char *attribute, *value, *re_string;
    char line[200];
    boolean wFlag = FALSE, hFlag = FALSE;
#define RE_NMATCH 4
    regmatch_t re_pmatch[RE_NMATCH];

    /* compile on first use */
    if (! pre) {
        if (regcomp(&re, svg_attr_value_re, REG_EXTENDED) != 0) {
	    agerr(AGERR,"cannot compile regular expression %s", svg_attr_value_re);
    	}
	pre = &re;
    }

    fseek(us->f, 0, SEEK_SET);
    while (fgets(line, sizeof(line), us->f) != NULL && (!wFlag || !hFlag)) {
	re_string = line;
	while (regexec(&re, re_string, RE_NMATCH, re_pmatch, 0) == 0) {
	    re_string[re_pmatch[1].rm_eo] = '\0';
	    re_string[re_pmatch[2].rm_eo] = '\0';
	    attribute = re_string + re_pmatch[1].rm_so;
	    value = re_string + re_pmatch[2].rm_so;
    	    re_string += re_pmatch[0].rm_eo + 1;

	    if (strcmp(attribute,"width") == 0) {
	        if (sscanf(value, "%lf%2s", &n, u) == 2) {
	            w = svg_units_convert(n, u);
	            wFlag = TRUE;
		}
		else if (sscanf(value, "%lf", &n) == 1) {
	            w = svg_units_convert(n, "pt");
	            wFlag = TRUE;
		}
		if (hFlag)
		    break;
	    }
	    else if (strcmp(attribute,"height") == 0) {
	        if (sscanf(value, "%lf%2s", &n, u) == 2) {
	            h = svg_units_convert(n, u);
	            hFlag = TRUE;
		}
	        else if (sscanf(value, "%lf", &n) == 1) {
	            h = svg_units_convert(n, "pt");
	            hFlag = TRUE;
		}
                if (wFlag)
		    break;
	    }
	    else if (strcmp(attribute,"viewBox") == 0
	      && sscanf(value, "%lf %lf %lf %lf", &x0,&y0,&x1,&y1) == 4) {
		w = x1 - x0 + 1;
		h = y1 - y0 + 1;
	        wFlag = TRUE;
	        hFlag = TRUE;
	        break;
	    }
	}
    }
    us->dpi = 72;
    us->w = w;
    us->h = h;
}

static void png_size (usershape_t *us)
{
    unsigned int w, h;

    us->dpi = 0;
    fseek(us->f, 16, SEEK_SET);
    if (get_int_msb_first(us->f, 4, &w) && get_int_msb_first(us->f, 4, &h)) {
        us->w = w;
        us->h = h;
    }
}

static void webp_size (usershape_t *us)
{
    unsigned int w, h;

    us->dpi = 0;
    fseek(us->f, 15, SEEK_SET);
    if (fgetc(us->f) == 'X') { //VP8X
        fseek(us->f, 24, SEEK_SET);
        if (get_int_lsb_first(us->f, 4, &w) && get_int_lsb_first(us->f, 4, &h)) {
            us->w = w;
            us->h = h;
        }
    }
    else { //VP8
        fseek(us->f, 26, SEEK_SET);
        if (get_int_lsb_first(us->f, 2, &w) && get_int_lsb_first(us->f, 2, &h)) {
            us->w = w;
            us->h = h;
        }
    }
}

static void gif_size (usershape_t *us)
{
    unsigned int w, h;

    us->dpi = 0;
    fseek(us->f, 6, SEEK_SET);
    if (get_int_lsb_first(us->f, 2, &w) && get_int_lsb_first(us->f, 2, &h)) {
        us->w = w;
        us->h = h;
    }
}

static void bmp_size (usershape_t *us) {
    unsigned int size_x_msw, size_x_lsw, size_y_msw, size_y_lsw;

    us->dpi = 0;
    fseek (us->f, 16, SEEK_SET);
    if ( get_int_lsb_first (us->f, 2, &size_x_msw) &&
         get_int_lsb_first (us->f, 2, &size_x_lsw) &&
         get_int_lsb_first (us->f, 2, &size_y_msw) &&
         get_int_lsb_first (us->f, 2, &size_y_lsw) ) {
        us->w = size_x_msw << 16 | size_x_lsw;
        us->h = size_y_msw << 16 | size_y_lsw;
    }
}

static void jpeg_size (usershape_t *us) {
    unsigned int marker, length, size_x, size_y, junk;

    /* These are the markers that follow 0xff in the file.
     * Other markers implicitly have a 2-byte length field that follows.
     */
    static unsigned char standalone_markers [] = {
        0x01,                       /* Temporary */
        0xd0, 0xd1, 0xd2, 0xd3,     /* Reset */
            0xd4, 0xd5, 0xd6,
            0xd7,
        0xd8,                       /* Start of image */
        0xd9,                       /* End of image */
        0
    };

    us->dpi = 0;
    while (TRUE) {
        /* Now we must be at a 0xff or at a series of 0xff's.
         * If that is not the case, or if we're at EOF, then there's
         * a parsing error.
         */
        if (! get_int_msb_first (us->f, 1, &marker))
            return;

        if (marker == 0xff)
            continue;

        /* Ok.. marker now read. If it is not a stand-alone marker,
         * then continue. If it's a Start Of Frame (0xc?), then we're there.
         * If it's another marker with a length field, then skip ahead
         * over that length field.
         */

        /* A stand-alone... */
        if (strchr ((char*)standalone_markers, marker))
            continue;

        /* Incase of a 0xc0 marker: */
        if (marker == 0xc0) {
            /* Skip length and 2 lengths. */
            if ( get_int_msb_first (us->f, 3, &junk)   &&
                 get_int_msb_first (us->f, 2, &size_x) &&
                 get_int_msb_first (us->f, 2, &size_y) ) {

            /* Store length. */
                us->h = size_x;
                us->w = size_y;
            }
	    return;
        }

        /* Incase of a 0xc2 marker: */
        if (marker == 0xc2) {
            /* Skip length and one more byte */
            if (! get_int_msb_first (us->f, 3, &junk))
                return;

            /* Get length and store. */
            if ( get_int_msb_first (us->f, 2, &size_x) &&
                 get_int_msb_first (us->f, 2, &size_y) ) {
                us->h = size_x;
                us->w = size_y;
            }
	    return;
        }

        /* Any other marker is assumed to be followed by 2 bytes length. */
        if (! get_int_msb_first (us->f, 2, &length))
            return;

        fseek (us->f, length - 2, SEEK_CUR);
    }
}

static void ps_size (usershape_t *us)
{
    char line[BUFSIZ];
    boolean saw_bb;
    int lx, ly, ux, uy;
    char* linep;

    us->dpi = POINTS_PER_INCH;
    fseek(us->f, 0, SEEK_SET);
    saw_bb = FALSE;
    while (fgets(line, sizeof(line), us->f)) {
	/* PostScript accepts \r as EOL, so using fgets () and looking for a
	 * bounding box comment at the beginning doesn't work in this case. 
	 * As a heuristic, we first search for a bounding box comment in line.
	 * This obviously fails if not all of the numbers make it into the
	 * current buffer. This shouldn't be a problem, as the comment is
	 * typically near the beginning, and so should be read within the first
	 * BUFSIZ bytes (even on Windows where this is 512).
	 */
	if (!(linep = strstr (line, "%%BoundingBox:")))
	    continue;
        if (sscanf (linep, "%%%%BoundingBox: %d %d %d %d", &lx, &ly, &ux, &uy) == 4) {
            saw_bb = TRUE;
	    break;
        }
    }
    if (saw_bb) {
	us->x = lx;
	us->y = ly;
        us->w = ux - lx;
        us->h = uy - ly;
    }
}

#define KEY "/MediaBox"

typedef struct {
    char* s;
    char* buf;
    FILE* fp;
} stream_t;

static unsigned char
nxtc (stream_t* str)
{
    if (fgets(str->buf, BUFSIZ, str->fp)) {
	str->s = str->buf;
	return *(str->s);
    }
    return '\0';
	
}

#define strc(x) (*(x->s)?*(x->s):nxtc(x))
#define stradv(x) (x->s++)

static void
skipWS (stream_t* str)
{
    unsigned char c;
    while ((c = strc(str))) {
	if (isspace(c)) stradv(str);
	else return;
    }
}

static int
scanNum (char* tok, double* dp)
{
    char* endp;
    double d = strtod(tok, &endp);

    if (tok == endp) return 1;
    *dp = d;
    return 0;
}

static void
getNum (stream_t* str, char* buf)
{
    int len = 0;
    char c;
    skipWS(str);
    while ((c = strc(str)) && (isdigit(c) || (c == '.'))) {
	buf[len++] = c;
	stradv(str);
	if (len == BUFSIZ-1) break;
    }
    buf[len] = '\0';

    return;
}

static int
boxof (stream_t* str, boxf* bp)
{
	char tok[BUFSIZ];

	skipWS(str);
	if (strc(str) != '[') return 1;
	stradv(str);
	getNum(str, tok);
	if (scanNum(tok,&bp->LL.x)) return 1;
	getNum(str, tok);
	if (scanNum(tok,&bp->LL.y)) return 1;
	getNum(str, tok);
	if (scanNum(tok,&bp->UR.x)) return 1;
	getNum(str, tok);
	if (scanNum(tok,&bp->UR.y)) return 1;
	return 0;
}

static int
bboxPDF (FILE* fp, boxf* bp)
{
	stream_t str;
	char* s;
	char buf[BUFSIZ];
	while (fgets(buf, BUFSIZ, fp)) {
		if ((s = strstr(buf,KEY))) {
			str.buf = buf;
			str.s = s+(sizeof(KEY)-1);
			str.fp = fp;
			return boxof(&str,bp);
		} 
	} 
	return 1;
}

static void pdf_size (usershape_t *us)
{
    boxf bb;

    us->dpi = POINTS_PER_INCH;
    fseek(us->f, 0, SEEK_SET);
    if ( ! bboxPDF (us->f, &bb)) {
	us->x = bb.LL.x;
	us->y = bb.LL.y;
        us->w = bb.UR.x - bb.LL.x;
        us->h = bb.UR.y - bb.LL.y;
    }
}

static void usershape_close (Dict_t * dict, Void_t * p, Dtdisc_t * disc)
{
    usershape_t *us = (usershape_t *)p;

    if (us->f)
	fclose(us->f);
    if (us->data && us->datafree)
	us->datafree(us);
    free (us);
}

static Dtdisc_t ImageDictDisc = {
    offsetof(usershape_t, name), /* key */
    -1,                         /* size */
    0,                          /* link offset */
    NIL(Dtmake_f),
    usershape_close,
    NIL(Dtcompar_f),
    NIL(Dthash_f),
    NIL(Dtmemory_f),
    NIL(Dtevent_f)
};

usershape_t *gvusershape_find(char *name)
{
    usershape_t probe;

    if (!ImageDict)
	return NULL;

    probe.name = name;
    return (dtsearch(ImageDict, &probe));
}

#define MAX_USERSHAPE_FILES_OPEN 50
boolean gvusershape_file_access(usershape_t *us)
{
    static int usershape_files_open_cnt;
    const char *fn;

    assert(us);
    assert(us->name);

    if (us->f)
	fseek(us->f, 0, SEEK_SET);
    else {
        if ((fn = safefile(us->name))) {
#ifndef WIN32
	    us->f = fopen(fn, "r");
#else
	    us->f = fopen(fn, "rb");
#endif
	    if (us->f == NULL) {
		agerr(AGWARN, "%s while opening %s\n", strerror(errno), fn);
		return FALSE;
	    }
	    if (usershape_files_open_cnt >= MAX_USERSHAPE_FILES_OPEN)
		us->nocache = TRUE;
	    else
	        usershape_files_open_cnt++;
	}
    }
    return TRUE;
}

void gvusershape_file_release(usershape_t *us)
{
    if (us->nocache) {
	if (us->f) {
	    fclose(us->f);
	    us->f = NULL;
	}
    }
}

static usershape_t *gvusershape_open (char *name)
{
    usershape_t *us;

    if (!ImageDict)
        ImageDict = dtopen(&ImageDictDisc, Dttree);

    if (! (us = gvusershape_find(name))) {
        if (! (us = zmalloc(sizeof(usershape_t))))
	    return NULL;

	us->name = name;
	if (!gvusershape_file_access(us)) 
	    return NULL;

        switch(imagetype(us)) {
	    case FT_NULL:
		if (!(us->data = (void*)find_user_shape(us->name)))
		    agerr(AGWARN, "\"%s\" was not found as a file or as a shape library member\n", us->name);
		    free(us);
		    return NULL;
		break;
	    case FT_GIF:
	        gif_size(us);
	        break;
	    case FT_PNG:
	        png_size(us);
	        break;
	    case FT_BMP:
	        bmp_size(us);
	        break;
	    case FT_JPEG:
	        jpeg_size(us);
	        break;
	    case FT_PS:
	        ps_size(us);
	        break;
	    case FT_WEBP:
	        webp_size(us);
	        break;
	    case FT_SVG:
	        svg_size(us);
	        break;
	    case FT_PDF:
		pdf_size(us);
		break;
	    case FT_EPS:   /* no eps_size code available */
	    default:
	        break;
        }
        dtinsert(ImageDict, us);
    }

    gvusershape_file_release(us);

    return us;
}

/* gvusershape_size_dpi:
 * Return image size in points.
 */
point 
gvusershape_size_dpi (usershape_t* us, pointf dpi)
{
    point rv;

    if (!us) {
        rv.x = rv.y = -1;
    }
    else {
	if (us->dpi != 0) {
	    dpi.x = dpi.y = us->dpi;
	}
	rv.x = us->w * POINTS_PER_INCH / dpi.x;
	rv.y = us->h * POINTS_PER_INCH / dpi.y;
    }
    return rv;
}

/* gvusershape_size:
 * Loads user image from file name if not already loaded.
 * Return image size in points.
 */
point gvusershape_size(graph_t * g, char *name)
{
    point rv;
    pointf dpi;
    static char* oldpath;
    usershape_t* us;

    /* no shape file, no shape size */
    if (!name || (*name == '\0')) {
        rv.x = rv.y = -1;
	return rv;
    }

    if (!HTTPServerEnVar && (oldpath != Gvimagepath)) {
	oldpath = Gvimagepath;
	if (ImageDict) {
	    dtclose(ImageDict);
	    ImageDict = NULL;
	}
    }

    if ((dpi.y = GD_drawing(g)->dpi) >= 1.0)
	dpi.x = dpi.y;
    else
	dpi.x = dpi.y = (double)DEFAULT_DPI;

    us = gvusershape_open (name);
    rv = gvusershape_size_dpi (us, dpi);
    return rv;
}
