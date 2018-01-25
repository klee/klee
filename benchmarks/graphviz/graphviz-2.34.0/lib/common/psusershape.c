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

#ifndef WIN32
#include <unistd.h>
#endif

#include <sys/stat.h>

#include "render.h"
#include "gvio.h"

static int N_EPSF_files;
static Dict_t *EPSF_contents;

static void ps_image_free(Dict_t * dict, usershape_t * p, Dtdisc_t * disc)
{
    free(p->data);
}

static Dtdisc_t ImageDictDisc = {
    offsetof(usershape_t, name),/* key */
    -1,				/* size */
    0,				/* link offset */
    NIL(Dtmake_f),
    (Dtfree_f) ps_image_free,
    NIL(Dtcompar_f),
    NIL(Dthash_f),
    NIL(Dtmemory_f),
    NIL(Dtevent_f)
};

static usershape_t *user_init(const char *str)
{
    char *contents;
    char line[BUFSIZ];
    FILE *fp;
    struct stat statbuf;
    int saw_bb, must_inline, rc;
    int lx, ly, ux, uy;
    usershape_t *us;

    if (!EPSF_contents)
	EPSF_contents = dtopen(&ImageDictDisc, Dtoset);

    us = dtmatch(EPSF_contents, str);
    if (us)
	return us;

    if (!(fp = fopen(str, "r"))) {
	agerr(AGWARN, "couldn't open epsf file %s\n", str);
	return NULL;
    }
    /* try to find size */
    saw_bb = must_inline = FALSE;
    while (fgets(line, sizeof(line), fp)) {
	if (sscanf
	    (line, "%%%%BoundingBox: %d %d %d %d", &lx, &ly, &ux, &uy) == 4) {
	    saw_bb = TRUE;
	}
	if ((line[0] != '%') && strstr(line,"read")) must_inline = TRUE;
	if (saw_bb && must_inline) break;
    }

    if (saw_bb) {
	us = GNEW(usershape_t);
	us->x = lx;
	us->y = ly;
	us->w = ux - lx;
	us->y = uy - ly;
	us->name = str;
	us->macro_id = N_EPSF_files++;
	fstat(fileno(fp), &statbuf);
	contents = us->data = N_GNEW(statbuf.st_size + 1, char);
	fseek(fp, 0, SEEK_SET);
	rc = fread(contents, statbuf.st_size, 1, fp);
	contents[statbuf.st_size] = '\0';
	dtinsert(EPSF_contents, us);
	us->must_inline = must_inline;
    } else {
	agerr(AGWARN, "BoundingBox not found in epsf file %s\n", str);
	us = NULL;
    }
    fclose(fp);
    return us;
}

void epsf_init(node_t * n)
{
    epsf_t *desc;
    const char *str;
    usershape_t *us;
    int dx, dy;

    if ((str = safefile(agget(n, "shapefile")))) {
	us = user_init(str);
	if (!us)
	    return;
	dx = us->w;
	dy = us->h;
	ND_width(n) = PS2INCH(dx);
	ND_height(n) = PS2INCH(dy);
	ND_shape_info(n) = desc = NEW(epsf_t);
	desc->macro_id = us->macro_id;
	desc->offset.x = -us->x - (dx) / 2;
	desc->offset.y = -us->y - (dy) / 2;
    } else
	agerr(AGWARN, "shapefile not set or not found for epsf node %s\n", agnameof(n));
}

void epsf_free(node_t * n)
{

    if (ND_shape_info(n))
	free(ND_shape_info(n));
}


/* cat_libfile:
 * Write library files onto the given file pointer.
 * arglib is an NULL-terminated array of char*
 * Each non-trivial entry should be the name of a file to be included.
 * stdlib is an NULL-terminated array of char*
 * Each of these is a line of a standard library to be included.
 * If any item in arglib is the empty string, the stdlib is not used.
 * The stdlib is printed first, if used, followed by the user libraries.
 * We check that for web-safe file usage.
 */
void cat_libfile(GVJ_t * job, const char **arglib, const char **stdlib)
{
    FILE *fp;
    const char **s, *bp, *p, *path;
    int i;
    boolean use_stdlib = TRUE;

    /* check for empty string to turn off stdlib */
    if (arglib) {
        for (i = 0; use_stdlib && ((p = arglib[i])); i++) {
            if (*p == '\0')
                use_stdlib = FALSE;
        }
    }
    if (use_stdlib)
        for (s = stdlib; *s; s++) {
            gvputs(job, *s);
            gvputs(job, "\n");
        }
    if (arglib) {
        for (i = 0; (p = arglib[i]) != 0; i++) {
            if (*p == '\0')
                continue;       /* ignore empty string */
            path = safefile(p);    /* make sure filename is okay */
	    if (!path) {
		agerr(AGWARN, "can't find library file %s\n", p);
	    }
            else if ((fp = fopen(path, "r"))) {
                while ((bp = Fgets(fp)))
                    gvputs(job, bp);
                gvputs(job, "\n"); /* append a newline just in case */
		fclose (fp);
            } else
                agerr(AGWARN, "can't open library file %s\n", path);
        }
    }
}

#define FILTER_EPSF 1
#ifdef FILTER_EPSF
/* this removes EPSF DSC comments that, when nested in another
 * document, cause errors in Ghostview and other Postscript
 * processors (although legal according to the Adobe EPSF spec).
 *
 * N.B. PostScript lines can end with \n, \r or \r\n.
 */
void epsf_emit_body(GVJ_t *job, usershape_t *us)
{
    char *p;
    char c;
    p = us->data;
    while (*p) {
	/* skip %%EOF lines */
	if ((p[0] == '%') && (p[1] == '%')
		&& (!strncasecmp(&p[2], "EOF", 3)
		|| !strncasecmp(&p[2], "BEGIN", 5)
		|| !strncasecmp(&p[2], "END", 3)
		|| !strncasecmp(&p[2], "TRAILER", 7)
	)) {
	    /* check for *p since last line might not end in '\n' */
	    while ((c = *p) && (c != '\r') && (c != '\n')) p++;
	    if ((*p == '\r') && (*(p+1) == '\n')) p += 2;
	    else if (*p) p++;
	    continue;
	}
	/* output line */
	while ((c = *p) && (c != '\r') && (c != '\n')) {
	    gvputc(job, c);
	    p++;
	}
	if ((*p == '\r') && (*(p+1) == '\n')) p += 2;
	else if (*p) p++;
	gvputc(job, '\n');
    }
}
#else
void epsf_emit_body(GVJ_t *job, usershape_t *us)
{
	gvputs(job, us->data);
}
#endif

void epsf_define(GVJ_t *job)
{
    usershape_t *us;

    if (!EPSF_contents)
	return;
    for (us = dtfirst(EPSF_contents); us; us = dtnext(EPSF_contents, us)) {
	if (us->must_inline)
	    continue;
	gvprintf(job, "/user_shape_%d {\n", us->macro_id);
	gvputs(job, "%%BeginDocument:\n");
	epsf_emit_body(job, us);
	gvputs(job, "%%EndDocument\n");
	gvputs(job, "} bind def\n");
    }
}

enum {ASCII, LATIN1, NONLATIN};

/* charsetOf:
 * Assuming legal utf-8 input, determine if
 * the character value range is ascii, latin-1 or otherwise.
 */
static int
charsetOf (char* s)
{
    int r = ASCII;
    unsigned char c;

    while ((c = *(unsigned char*)s++)) {
	if (c < 0x7F) 
	    continue;
	else if ((c & 0xFC) == 0xC0) {
	    r = LATIN1;
	    s++; /* eat second byte */
	}
	else return NONLATIN;
    }
    return r;
}

char *ps_string(char *ins, int latin)
{
    char *s;
    char *base;
    static agxbuf  xb;
    static int warned;
    int rc;

    if (latin)
        base = utf8ToLatin1 (ins);
    else {
	switch (charsetOf (ins)) {
	case ASCII :
	    base = ins;
	    break;
	case LATIN1 :
	    base = utf8ToLatin1 (ins);
	    break;
	case NONLATIN :
	    if (!warned) {
		agerr (AGWARN, "UTF-8 input uses non-Latin1 characters which cannot be handled by this PostScript driver\n");
		warned = 1;
	    }
	    base = ins;
	    break;
	default:
	    base = ins;
	    break;
	}
    }

    if (xb.buf == NULL)
        agxbinit (&xb, 0, NULL);

    rc = agxbputc (&xb, LPAREN);
    s = base;
    while (*s) {
        if ((*s == LPAREN) || (*s == RPAREN) || (*s == '\\'))
            rc = agxbputc (&xb, '\\');
        rc = agxbputc (&xb, *s++);
    }
    agxbputc (&xb, RPAREN);
    if (base != ins) free (base);
    s = agxbuse(&xb);
    return s;
}
