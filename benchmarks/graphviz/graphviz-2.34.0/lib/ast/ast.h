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

#ifdef __cplusplus
extern "C" {
#endif

#ifndef AST_H
#define AST_H

#include <sfio.h>
#include <stdlib.h>
#include <stddef.h>
#include <align.h>
#ifdef sun
#include <string.h>
#endif

#define NiL 0
#ifndef PATH_MAX
#define PATH_MAX    1024
#endif
#ifndef CHAR_BIT
#define CHAR_BIT    8
#endif

#define PATH_PHYSICAL   01
#define PATH_DOTDOT 02
#define PATH_EXISTS 04
#define PATH_VERIFIED(n) (((n)&01777)<<5)

#define PATH_REGULAR    010
#define PATH_EXECUTE      001
#define PATH_READ 004
#define PATH_WRITE        002
#define PATH_ABSOLUTE     020

/*
 * strgrpmatch() flags
 */

#define STR_MAXIMAL 01		/* maximal match        */
#define STR_LEFT    02		/* implicit left anchor     */
#define STR_RIGHT   04		/* implicit right anchor    */
#define STR_ICASE   010		/* ignore case          */

/*
 * fmtquote() flags
 */

#define FMT_ALWAYS  0x01	/* always quote         */
#define FMT_ESCAPED 0x02	/* already escaped      */
#define FMT_SHELL   0x04	/* escape $ ` too       */
#define FMT_WIDE    0x08	/* don't escape 8 bit chars */

#define CC_bel      0007	/* bel character        */
#define CC_esc      0033	/* esc character        */
#define CC_vt       0013	/* vt character         */

#define elementsof(x)   (sizeof(x)/sizeof(x[0]))
#define newof(p,t,n,x)  ((p)?(t*)realloc((char*)(p),sizeof(t)*(n)+(x)):(t*)calloc(1,sizeof(t)*(n)+(x)))
#define oldof(p,t,n,x)  ((p)?(t*)realloc((char*)(p),sizeof(t)*(n)+(x)):(t*)malloc(sizeof(t)*(n)+(x)))
#define streq(a,b)  (*(a)==*(b)&&!strcmp(a,b))
#define strneq(a,b,n)     (*(a)==*(b)&&!strncmp(a,b,n))
#define memzero(b,n)    memset(b,0,n)

    extern char *pathpath(char *, const char *, const char *, int);
    extern char *pathfind(const char *, const char *, const char *, char *,
			  size_t);
    extern char *pathaccess(char *, const char *, const char *,
			    const char *, int);
    extern char *pathbin(void);
    extern char *pathcat(char *, const char *, int, const char *,
			 const char *);
    extern int pathgetlink(const char *, char *, int);
    extern int pathexists(char *, int);

    extern int chresc(const char *, char **);
    extern int chrtoi(const char *);
    extern char *fmtesq(const char *, const char *);
    extern char *fmtesc(const char *as);
    extern char *fmtbuf(size_t n);
    extern char *fmtquote(const char *, const char *, const char *, size_t,
			  int);

    extern int astquery(int, const char *, ...);

    extern int strmatch(const char *, const char *);
    extern int strgrpmatch(const char *, const char *, int *, int, int);
    extern int stresc(char *);
    extern long strton(const char *, char **, char *, int);
    extern char *strcopy(char *s, const char *t);

#endif

#ifdef __cplusplus
}
#endif
