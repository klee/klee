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

/* Lefteris Koutsofios - AT&T Labs Research */

#include "common.h"
#include "g.h"
#include "mem.h"
#include "io.h"
#include "code.h"
#include "tbl.h"
#include "parse.h"
#include "exec.h"
#include "display.h"
#include "txtview.h"
#include "gfxview.h"
#ifdef FEATURE_DOT
#include "dot2l.h"
#endif
#ifdef FEATURE_GMAP
#include "gmap2l.h"
#endif
#include "internal.h"
#ifndef FEATURE_MS
#include <sys/time.h>
#endif

int Idump (int, Tonm_t *);
int Itypeof (int, Tonm_t *);
int Icopy (int, Tonm_t *);
int Iremove (int, Tonm_t *);
int Itablesize (int, Tonm_t *);
int Iopenio (int, Tonm_t *);
int Icloseio (int, Tonm_t *);
int Ireadline (int, Tonm_t *);
int Iread (int, Tonm_t *);
int Iwriteline (int, Tonm_t *);
int Iatan (int, Tonm_t *);
int Itan (int, Tonm_t *);
int Icos (int, Tonm_t *);
int Isin (int, Tonm_t *);
int Isqrt (int, Tonm_t *);
int Irandom (int, Tonm_t *);
int Intos (int, Tonm_t *);
int Iston (int, Tonm_t *);
int Isplit (int, Tonm_t *);
int Iconcat (int, Tonm_t *);
int Iquote (int, Tonm_t *);
int Ihtmlquote (int, Tonm_t *);
int Itoint (int, Tonm_t *);
int Istrlen (int, Tonm_t *);
int Iload (int, Tonm_t *);
int Irun (int, Tonm_t *);
int Imonitor (int, Tonm_t *);
int Iidlerun (int, Tonm_t *);
int Itime (int, Tonm_t *);
int Isleep (int, Tonm_t *);
int Iecho (int, Tonm_t *);
int Igetenv (int, Tonm_t *);
int Iputenv (int, Tonm_t *);
int Isystem (int, Tonm_t *);
int Iexit (int, Tonm_t *);
#ifdef FEATURE_DOT
int Iparsegraphlabel (int, Tonm_t *);
int Ireadgraph (int, Tonm_t *);
int Iwritegraph (int, Tonm_t *);
#endif
#ifdef _PACKAGE_ast
int Imatch (int, Tonm_t *);
#endif
#ifdef FEATURE_CS
int C2Lreadcsmessage (int, Tonm_t *);
#endif

#define MAXN 10000

Ifunc_t Ifuncs[] = {
    { "createwidget",  GFXcreatewidget,  2,    2 },
    { "setwidgetattr", GFXsetwidgetattr, 2,    2 },
    { "getwidgetattr", GFXgetwidgetattr, 2,    2 },
    { "destroywidget", GFXdestroywidget, 1,    1 },
    { "clear",         GFXclear,         1,    1 },
    { "setgfxattr",    GFXsetgfxattr,    2,    2 },
    { "getgfxattr",    GFXgetgfxattr,    2,    2 },
    { "arrow",         GFXarrow,         4,    5 },
    { "line",          GFXline,          4,    5 },
    { "box",           GFXbox,           3,    4 },
    { "polygon",       GFXpolygon,       3,    4 },
    { "splinegon",     GFXsplinegon,     3,    4 },
    { "arc",           GFXarc,           4,    5 },
    { "text",          GFXtext,          7,    8 },
    { "textsize",      GFXtextsize,      4,    4 },
    { "createbitmap",  GFXcreatebitmap,  2,    2 },
    { "destroybitmap", GFXdestroybitmap, 1,    1 },
    { "readbitmap",    GFXreadbitmap,    2,    2 },
    { "writebitmap",   GFXwritebitmap,   2,    2 },
    { "bitblt",        GFXbitblt,        6,    7 },
    { "clearpick",     GFXclearpick,     2,    2 },
    { "setpick",       GFXsetpick,       3,    3 },
    { "displaymenu",   GFXdisplaymenu,   2,    2 },
    { "colormap",      GFXcolormap,      1,    1 },
    { "txtview",       TXTmode,          1,    1 },
    { "ask",           TXTask,           1,    3 },
    { "dump",          Idump,            0, MAXN },
    { "typeof",        Itypeof,          1,    1 },
    { "copy",          Icopy,            1,    1 },
    { "remove",        Iremove,          1,    2 },
    { "tablesize",     Itablesize,       1,    1 },
    { "openio",        Iopenio,          3,    4 },
    { "closeio",       Icloseio,         1,    2 },
    { "readline",      Ireadline,        1,    1 },
    { "read",          Iread,            1,    1 },
    { "writeline",     Iwriteline,       2,    2 },
    { "atan",          Iatan,            2,    2 },
    { "tan",           Itan,             1,    1 },
    { "cos",           Icos,             1,    1 },
    { "sin",           Isin,             1,    1 },
    { "sqrt",          Isqrt,            1,    1 },
    { "random",        Irandom,          1,    1 },
    { "ntos",          Intos,            1,    1 },
    { "ston",          Iston,            1,    1 },
    { "split",         Isplit,           2,    3 },
    { "concat",        Iconcat,          1, MAXN },
    { "quote",         Iquote,           1,    3 },
    { "htmlquote",     Ihtmlquote,       1,    1 },
    { "toint",         Itoint,           1,    1 },
    { "strlen",        Istrlen,          1,    1 },
    { "load",          Iload,            1,    1 },
    { "run",           Irun,             1,    1 },
    { "monitor",       Imonitor,         2,    2 },
    { "idlerun",       Iidlerun,         1,    1 },
    { "time",          Itime,            0,    0 },
    { "sleep",         Isleep,           1,    1 },
    { "echo",          Iecho,            1, MAXN },
    { "getenv",        Igetenv,          1,    1 },
    { "putenv",        Iputenv,          2,    2 },
    { "system",        Isystem,          1, MAXN },
    { "exit",          Iexit,            0,    0 },
#ifdef FEATURE_DOT
    { "parsegraphlabel", Iparsegraphlabel, 2, 2 },
    { "readgraph",       Ireadgraph,       1, 2 },
    { "writegraph",      Iwritegraph,      3, 3 },
#endif
#ifdef _PACKAGE_ast
    { "match", Imatch, 2, 2 },
#endif
#ifdef FEATURE_CS
    { "readcsmessage", C2Lreadcsmessage, 1, 1 },
#endif
#ifdef FEATURE_GMAP
    { "createwindow",     G2Lcreatewindow,     2, 2 },
    { "destroywindow",    G2Ldestroywindow,    1, 1 },
    { "setwindowattr",    G2Lsetwindowattr,    2, 2 },
    { "getwindowattr",    G2Lgetwindowattr,    2, 2 },
    { "createchannel",    G2Lcreatechannel,    2, 2 },
    { "destroychannel",   G2Ldestroychannel,   1, 1 },
    { "setchannelattr",   G2Lsetchannelattr,   2, 2 },
    { "getchannelattr",   G2Lgetchannelattr,   2, 2 },
    { "getchannelcoord",  G2Lgetchannelcoord,  2, 3 },
    { "loadgeometry",     G2Lloadgeometry,     1, 1 },
    { "unloadgeometry",   G2Lunloadgeometry,   1, 1 },
    { "setgeometryattr",  G2Lsetgeometryattr,  2, 2 },
    { "getgeometryattr",  G2Lgetgeometryattr,  2, 2 },
    { "getgeometryitems", G2Lgetgeometryitems, 3, 3 },
    { "insertgeom2chan",  G2Linsertgeom2chan,  2, 2 },
    { "removegeom2chan",  G2Lremovegeom2chan,  1, 1 },
    { "loadvalue",        G2Lloadvalue,        1, 1 },
    { "unloadvalue",      G2Lunloadvalue,      1, 1 },
    { "setvalueattr",     G2Lsetvalueattr,     2, 2 },
    { "getvalueattr",     G2Lgetvalueattr,     2, 2 },
    { "getvalueitems",    G2Lgetvalueitems,    2, 2 },
    { "insertval2geom",   G2Linsertval2geom,   3, 3 },
    { "removeval2geom",   G2Lremoveval2geom,   1, 1 },
    { "setval2geomattr",  G2Lsetval2geomattr,  2, 2 },
    { "getval2geomattr",  G2Lgetval2geomattr,  2, 2 },
    { "updatewindows",    G2Lupdatewindows,    0, 0 },
#endif
    { 0, 0, 0, 0 }
};
int Ifuncn;

static char *bufp;
static int bufn;
#define BUFINCR 10240
#define BUFSIZE sizeof (char)
static void growbufp (int);

void Iinit (void) {
    int i;

    if (!(bufp = malloc (BUFINCR * BUFSIZE)))
        panic1 (POS, "Iinit", "buf malloc failed");
    bufn = BUFINCR;
    for (i = 0; Ifuncs[i].name; i++)
        Efunction (Pfunction (Ifuncs[i].name, i), Ifuncs[i].name);
    Ifuncn = sizeof (Ifuncs) / sizeof (Ifunc_t) - 1;
}

void Iterm (void) {
    int i;

    for (i = 0; i < Ifuncn; i++)
        Tdels (root, Ifuncs[i].name);
    Ifuncn = 0;
    free (bufp), bufp = NULL, bufn = 0;
}

int Igetfunc (char *name) {
    int i = 0;

    while (Ifuncs[i].name && strcmp (Ifuncs[i].name, name) != 0)
        i++;
    return (Ifuncs[i].name) ? i : -1;
}

/* display.c functions */

int Idump (int argc, lvar_t *argv) {
    int i;

    if (argc == 0)
        Dtrace (root, 0);
    else
        for (i = 0; i < argc; i++)
            Dtrace (argv[i].o, 0);
    return L_SUCCESS;
}

/* tbl.c functions */

int Itypeof (int argc, lvar_t *argv) {
    if (T_ISTABLE (argv[0].o))
        rtno = Tstring ("table");
    else if (T_ISSTRING (argv[0].o))
        rtno = Tstring ("string");
    else if (T_ISNUMBER (argv[0].o))
        rtno = Tstring ("number");
    return L_SUCCESS;
}

int Icopy (int argc, lvar_t *argv) {
    rtno = Tcopy (argv[0].o);
    return L_SUCCESS;
}

int Iremove (int argc, lvar_t *argv) {
    Tobj tblo, keyo;

    if (argc == 2)
        tblo = argv[1].o, keyo = argv[0].o;
    else
        tblo = root, keyo = argv[0].o;
    if (T_ISTABLE (tblo) && (T_ISNUMBER (keyo) || T_ISSTRING (keyo)))
        Tdelo (tblo, keyo);
    return L_SUCCESS;
}

int Itablesize (int argc, lvar_t *argv) {
    Tobj vo;

    if (Tgettype ((vo = argv[0].o)) != T_TABLE)
        return L_FAILURE;
    rtno = Tinteger (((Ttable_t *) vo)->n);
    return L_SUCCESS;
}

/* file.c functions */

int Iopenio (int argc, lvar_t *argv) {
    int rtn;

    if (argc == 3)
        rtn = IOopen (
            Tgetstring (argv[0].o),
            Tgetstring (argv[1].o), Tgetstring (argv[2].o), NULL
        );
    else
        rtn = IOopen (
            Tgetstring (argv[0].o),
            Tgetstring (argv[1].o), Tgetstring (argv[2].o),
            Tgetstring (argv[3].o)
        );
    rtno = NULL;
    if (rtn == -1)
        return L_SUCCESS;
    rtno = Tinteger (rtn);
    return L_SUCCESS;
}

int Icloseio (int argc, lvar_t *argv) {
    if (argc == 1)
        IOclose ((int) Tgetnumber (argv[0].o), NULL);
    else
        IOclose ((int) Tgetnumber (argv[0].o), Tgetstring (argv[1].o));
    return L_SUCCESS;
}

int Ireadline (int argc, lvar_t *argv) {
    char *s;
    int m, n;

    s = bufp, n = bufn;
    while ((m = IOreadline ((int) Tgetnumber (argv[0].o), s, n)) != -1) {
        if (m < n - 1)
            break;
        m += (s - bufp);
        growbufp (bufn + BUFINCR);
        s = bufp + m, n = bufn - m;
    }
    if (m != -1)
        rtno = Tstring (bufp);
    else
        rtno = NULL;
    return L_SUCCESS;
}

int Iread (int argc, lvar_t *argv) {
    if (IOread ((int) Tgetnumber (argv[0].o), bufp, bufn) > 0)
        rtno = Tstring (bufp);
    else
        rtno = NULL;
    return L_SUCCESS;
}

int Iwriteline (int argc, lvar_t *argv) {
    IOwriteline ((int) Tgetnumber (argv[0].o), Tgetstring (argv[1].o));
    return L_SUCCESS;
}

/* math functions */

int Iatan (int argc, lvar_t *argv) {
    double x, y;

    y = Tgetnumber (argv[0].o), x = Tgetnumber (argv[1].o);
    rtno = Treal (180 * atan2 (y, x) / M_PI);
    return L_SUCCESS;
}

int Itan (int argc, lvar_t *argv) {
    rtno = Treal (tan (M_PI * Tgetnumber (argv[0].o) / 180.0));
    return L_SUCCESS;
}

int Icos (int argc, lvar_t *argv) {
    rtno = Treal (cos (M_PI * Tgetnumber (argv[0].o) / 180.0));
    return L_SUCCESS;
}

int Isin (int argc, lvar_t *argv) {
    rtno = Treal (sin (M_PI * Tgetnumber (argv[0].o) / 180.0));
    return L_SUCCESS;
}

int Isqrt (int argc, lvar_t *argv) {
    rtno = Treal (sqrt (Tgetnumber (argv[0].o)));
    return L_SUCCESS;
}

#if !defined(HAVE_LRAND48) || defined(FEATURE_MS)
#define lrand48 rand
#endif

int Irandom (int argc, lvar_t *argv) {
    rtno = Treal (
        (Tgetnumber (argv[0].o) *
        (lrand48 () & 0xffff)) / (double) (0xffff)
    );
    return L_SUCCESS;
}

/* conversion functions */

int Intos (int argc, lvar_t *argv) {
    double d;

    d = Tgetnumber (argv[0].o);
    if ((long) d == d)
        sprintf (bufp, "%ld", (long) d);
    else
        sprintf (bufp, "%f", d);
    rtno = Tstring (bufp);
    return L_SUCCESS;
}

int Iston (int argc, lvar_t *argv) {
    rtno = Treal ((double) atof (Tgetstring (argv[0].o)));
    return L_SUCCESS;
}

int Isplit (int argc, lvar_t *argv) {
    Tobj so, fo;
    char *sp, *sp2, *s;
    char fc, tc, qmode;
    long rtnm, rtni;
    int bufi, qflag;

    if (
        Tgettype ((so = argv[0].o)) != T_STRING ||
        Tgettype ((fo = argv[1].o)) != T_STRING
    )
        return L_FAILURE;
    qflag = (argc == 3) ? FALSE : TRUE;
    sp = Tgetstring (so);
    s = Tgetstring (fo);
    if (s[0] == '\\' && s[1] == 'n')
        fc = '\n';
    else
        fc = s[0];
    rtno = Ttable (4);
    rtnm = Mpushmark (rtno);
    rtni = 0;
    if (s[0] == 0) {
        for (sp2 = sp; *sp2; sp2++) {
            tc = *(sp2 + 1), *(sp2 + 1) = '\000';
            Tinsi (rtno, rtni++, Tstring (sp2));
            *(sp2 + 1) = tc;
        }
    } else if (qflag && (fc == ' ' || fc == '	')) {
        while (*sp == fc)
            sp++;
        while (*sp) {
            bufi = 0;
            qmode = 0;
            for (sp2 = sp; *sp2; sp2++) {
                if (bufi == bufn)
                    growbufp (bufn + BUFINCR);
                if (*sp2 == '"' || *sp2 == '\'') {
                    if (qmode) {
                        if (qmode == *sp2)
                            qmode = 0;
                        else
                            bufp[bufi++] = *sp2;
                    } else
                        qmode = *sp2;
                } else if (*sp2 == fc && !qmode)
                    break;
                else
                    bufp[bufi++] = *sp2;
            }
            if (bufi == bufn)
                growbufp (bufn + BUFINCR);
            bufp[bufi] = 0;
            Tinsi (rtno, rtni++, Tstring (bufp));
            while (*sp2 == fc)
                sp2++;
            sp = sp2;
        }
    } else {
        while (*sp) {
            for (sp2 = sp; *sp2 && *sp2 != fc; sp2++)
                ;
            tc = *sp2, *sp2 = '\000';
            Tinsi (rtno, rtni++, Tstring (sp));
            *sp2 = tc;
            if (*sp2) {
                sp2++;
                if (!*sp2)
                    Tinsi (rtno, rtni++, Tstring (""));
            }
            sp = sp2;
        }
    }
    Mpopmark (rtnm);
    return L_SUCCESS;
}

int Iconcat (int argc, lvar_t *argv) {
    Tobj ao;
    char buf2[50];
    char *s;
    int i, n, bufi;

    for (bufi = 0, i = 0; i < argc; i++) {
        ao = argv[i].o;
        switch (Tgettype (argv[i].o)) {
        case T_STRING:
            if (bufi + (n = strlen (Tgetstring (ao)) + 1) > bufn)
                growbufp (bufi + n);
            for (s = Tgetstring (ao); *s; s++)
                bufp[bufi++] = *s;
            break;
        case T_INTEGER:
            if (bufi + 50 > bufn)
                growbufp (bufi + 50);
            sprintf (buf2, "%ld", Tgetinteger (ao));
            for (s = buf2; *s; s++)
                bufp[bufi++] = *s;
            break;
        case T_REAL:
            if (bufi + 50 > bufn)
                growbufp (bufi + 50);
            sprintf (buf2, "%f", Tgetreal (ao));
            for (s = buf2; *s; s++)
                bufp[bufi++] = *s;
            break;
        }
    }
    bufp[bufi] = '\000';
    rtno = Tstring (bufp);
    return L_SUCCESS;
}

int Iquote (int argc, lvar_t *argv) {
    Tobj so, ao=NULL, qo=NULL;
    char *s=NULL, *s1, *s2, *qs, *as;
    char buf2[50];
    int n, bufi;

    if (
        (Tgettype ((so = argv[0].o)) != T_STRING && !T_ISNUMBER (so)) ||
        (argc > 1 && Tgettype ((qo = argv[1].o)) != T_STRING) ||
        (argc > 2 && Tgettype ((ao = argv[2].o)) != T_STRING)
    )
        return L_FAILURE;
    switch (Tgettype (so)) {
    case T_STRING:
        s = Tgetstring (so);
        break;
    case T_INTEGER:
        sprintf (buf2, "%ld", Tgetinteger (so));
        s = &buf2[0];
        break;
    case T_REAL:
        sprintf (buf2, "%f", Tgetreal (so));
        s = &buf2[0];
        break;
    }
    if (argc > 1)
        qs = Tgetstring (qo);
    else
        qs = "'\"";
    if (argc > 2)
        as = Tgetstring (ao);
    else
        as = NULL;
    bufi = 0;
    if ((n = strlen (s) + 3) * 2 > bufn)
        growbufp (n * 2); /* the *2 is max for chars to quote */
    if (as)
        bufp[bufi++] = *as;
    for (s1 = s; *s1; s1++) {
        for (s2 = qs; *s2; s2++)
            if (*s1 == *s2) {
                bufp[bufi++] = '\\', bufp[bufi++] = *s1;
                break;
            }
        if (!*s2) {
            switch (*s1) {
            case '\n': bufp[bufi++] = '\\', bufp[bufi++] = 'n'; break;
            case '\r': bufp[bufi++] = '\\', bufp[bufi++] = 'r'; break;
            default: bufp[bufi++] = *s1; break;
            }
        }
    }
    if (as)
        bufp[bufi++] = *as;
    bufp[bufi] = '\000';
    rtno = Tstring (bufp);
    return L_SUCCESS;
}

int Ihtmlquote (int argc, lvar_t *argv) {
    Tobj so;
    char *s, *s1;
    int n, bufi;

    if (Tgettype ((so = argv[0].o)) != T_STRING)
        return L_FAILURE;
    s = Tgetstring (so);
    bufi = 0;
    if ((n = strlen (s) + 1) * 4 > bufn)
        growbufp (n * 4); /* the *4 is max for chars to quote */
    for (s1 = s; *s1; s1++) {
        switch (*s1) {
        case '/':
            bufp[bufi++] = '%';
            bufp[bufi++] = '2';
            bufp[bufi++] = 'F';
            break;
        case '%':
            bufp[bufi++] = '%';
            bufp[bufi++] = '2';
            bufp[bufi++] = '5';
            break;
        case ' ':
            bufp[bufi++] = '+';
            break;
        default:
            bufp[bufi++] = *s1;
            break;
        }
    }
    bufp[bufi] = '\000';
    rtno = Tstring (bufp);
    return L_SUCCESS;
}

int Itoint (int argc, lvar_t *argv) {
    rtno = Tinteger ((long) Tgetnumber (argv[0].o));
    return L_SUCCESS;
}

int Istrlen (int argc, lvar_t *argv) {
    rtno = Tinteger (strlen (Tgetstring (argv[0].o)));
    return L_SUCCESS;
}

/* script loading functions */

int Iload (int argc, lvar_t *argv) {
    Psrc_t src;
    char *fn;
    FILE *fp;
    Tobj co;

    if ((fn = Tgetstring (argv[0].o))) {
        if (fn[0] == '-' && fn[1] == '\000')
            fp = stdin;
        else {
            fp = NULL;
            if ((fn = buildpath (fn, FALSE)))
                fp = fopen (fn, "r");
        }
        if (fp) {
            src.flag = FILESRC, src.s = NULL, src.fp = fp;
            src.tok = -1, src.lnum = 1;
            while ((co = Punit (&src)))
                Eoktorun = TRUE, Eunit (co);
            if (fp != stdin)
                fclose (fp);
        } else
            return L_FAILURE;
    }
    return L_SUCCESS;
}

int Irun (int argc, lvar_t *argv) {
    Psrc_t src;
    char *s;
    Tobj co;

    if ((s = Tgetstring (argv[0].o))) {
        src.flag = CHARSRC, src.s = s, src.fp = NULL;
        src.tok = -1, src.lnum = 1;
        while ((co = Punit (&src)))
            Eoktorun = TRUE, Eunit (co);
    }
    return L_SUCCESS;
}

/* mode setting functions */

int Imonitor (int argc, lvar_t *argv) {
    Tobj mo, io;
    char *ms;
    int ioi;

    if (
        Tgettype ((mo = argv[0].o)) != T_STRING ||
        (Tgettype ((io = argv[1].o)) != T_INTEGER && Tgettype (io) != T_REAL)
    )
        return L_FAILURE;
    ms = Tgetstring (mo), ioi = Tgetnumber (io);
    if (ioi < 0 || ioi >= ion)
        return L_FAILURE;
    if (strcmp (ms, "on") == 0)
        IOmonitor (ioi, inputfds);
    else if (strcmp (ms, "off") == 0)
        IOunmonitor (ioi, inputfds);
    else
        return L_FAILURE;
    return L_SUCCESS;
}

int Iidlerun (int argc, lvar_t *argv) {
    Tobj mo;
    char *ms;
    int mode;

    if (Tgettype ((mo = argv[0].o)) != T_STRING)
        return L_SUCCESS;
    ms = Tgetstring (mo);
    if (strcmp (ms, "on") == 0)
        mode = 1;
    else if (strcmp (ms, "off") == 0)
        mode = 0;
    else
        return L_FAILURE;
    idlerunmode = mode;
    return L_SUCCESS;
}

/* system functions */

int Itime (int argc, lvar_t *argv) {
#ifndef FEATURE_MS
    struct timeval tz;

    gettimeofday (&tz, NULL);
    rtno = Treal (tz.tv_sec + tz.tv_usec / 1000000.0);
#else
    rtno = Treal (0);
#endif
    return L_SUCCESS;
}

int Isleep (int argc, lvar_t *argv) {
#ifndef FEATURE_MS
    struct timeval tz;
    float f;

    Gsync ();
    f = Tgetnumber (argv[0].o);
    tz.tv_sec = f;
    tz.tv_usec = (f - tz.tv_sec) * 1000000;
    if (select (0, NULL, NULL, NULL, &tz) == -1)
        return L_FAILURE;
#endif
    return L_SUCCESS;
}

int Iecho (int argc, lvar_t *argv) {
    int i;

    rtno = NULL;
    for (i = 0; i < argc; i++) {
        switch (Tgettype (argv[i].o)) {
        case T_STRING:  printf ("%s", Tgetstring (argv[i].o));   break;
        case T_INTEGER: printf ("%ld", Tgetinteger (argv[i].o)); break;
        case T_REAL:    printf ("%f", Tgetreal (argv[i].o));     break;
        case T_TABLE:   printf ("[\n"), Dtrace (argv[i].o, 4);   break;
        }
    }
    printf ("\n");
    return L_SUCCESS;
}

int Igetenv (int argc, lvar_t *argv) {
    char *s;

    if (!T_ISSTRING (argv[0].o))
        return L_FAILURE;
    rtno = NULL;
    if (!(s = getenv (Tgetstring (argv[0].o))) || !*s)
        return L_SUCCESS;
    rtno = Tstring (s);
    return L_SUCCESS;
}

int Iputenv (int argc, lvar_t *argv) {
    if (!T_ISSTRING (argv[0].o) || !T_ISSTRING (argv[1].o))
        return L_FAILURE;
    bufp[0] = 0;
    strcat (bufp, Tgetstring (argv[0].o));
    strcat (bufp, "=");
    strcat (bufp, Tgetstring (argv[1].o));
    putenv (bufp);
    return L_SUCCESS;
}

int Isystem (int argc, lvar_t *argv) {
    int i;

    bufp[0] = 0;
    strcat (bufp, Tgetstring (argv[0].o));
    for (i = 1; i < argc; i++)
        strcat (bufp, " "), strcat (bufp, Tgetstring (argv[i].o));
#ifndef FEATURE_MS
    system (bufp);
#else
    {
        UINT handle;
        handle = WinExec (bufp, SW_SHOW);
    }
#endif
    return L_SUCCESS;
}

int Iexit (int argc, lvar_t *argv) {
    longjmp (exitljbuf, 1);
    return L_SUCCESS; /* NOT REACHED */
}

#ifdef FEATURE_DOT
/* dot related functions */

int Iparsegraphlabel (int argc, lvar_t *argv) {
    rtno = D2Lparsegraphlabel (argv[0].o, argv[1].o);
    return L_SUCCESS;
}

int Ireadgraph (int argc, lvar_t *argv) {
    int ioi;

    if ((ioi = Tgetnumber (argv[0].o)) < 0 || ioi >= ion)
        return L_FAILURE;
    if (argc == 2 && !T_ISTABLE (argv[1].o))
        return L_FAILURE;
    if (!(rtno = D2Lreadgraph (ioi, (argc == 2 ? argv[1].o : NULL))))
        return L_FAILURE;
    return L_SUCCESS;
}

int Iwritegraph (int argc, lvar_t *argv) {
    int ioi;

    if (
        !T_ISNUMBER (argv[0].o) || !T_ISTABLE (argv[1].o) ||
        !T_ISINTEGER (argv[2].o)
    )
        return L_FAILURE;
    if ((ioi = Tgetnumber (argv[0].o)) < 0 || ioi >= ion)
        return L_FAILURE;
    D2Lwritegraph (ioi, argv[1].o, Tgetinteger (argv[2].o));
    return L_SUCCESS;
}
#endif

#ifdef _PACKAGE_ast
/* ast related functions */

int Imatch (int argc, lvar_t *argv) {
    if (!T_ISSTRING (argv[0].o) || !T_ISSTRING (argv[1].o))
        return L_FAILURE;
    rtno = Tinteger (strmatch (Tgetstring (argv[0].o), Tgetstring (argv[1].o)));
    return L_SUCCESS;
}
#endif

static void growbufp (int newsize) {
    if (!(bufp = realloc (
        bufp, ((newsize + BUFINCR - 1) / BUFINCR) * BUFINCR * BUFSIZE
    )))
        panic1 (POS, "growbufp", "buf realloc failed");
    bufn = ((newsize + BUFINCR - 1) / BUFINCR) * BUFINCR;
}
