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

/* Lefteris Koutsofios - AT&T Labs Research */

#ifndef _COMMON_H
#define _COMMON_H

/* some config and conversion definitions from graphviz distribution */
#ifdef HAVE_CONFIG_H
#include "config.h"
/* we set some parameters which graphviz assumes.
 * At some point, we could consider allowing these to be set in configure
 * See Makefile.old for others.
 */
#define FEATURE_MINTSIZE 1 
#define FEATURE_DOT 1
#if defined(WIN32) || defined(MSWIN32)
#ifndef FEATURE_WIN32
#define FEATURE_WIN32 1
#endif
#ifndef FEATURE_MS
#define FEATURE_MS 1
#endif
#else
#define FEATURE_X11 1
#endif
#endif

#ifdef _PACKAGE_ast
#include <ast.h>
#define HAVE_STRERROR 1
#else
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifndef MSWIN32
#include <sys/time.h>
#endif
#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif
#endif

#include <math.h>
#include <stdio.h>
#include <setjmp.h>
#include <ctype.h>

#ifdef FEATURE_WIN32
#include <windows.h>
#include <commdlg.h>
#endif

#ifdef FEATURE_MS
#include <malloc.h>
#define printf gprintf
#endif

#define POS __FILE__, __LINE__

#ifndef TRUE
#define TRUE  1
#define FALSE 0
#endif

#ifndef L_SUCCESS
#define L_SUCCESS 1
#define L_FAILURE 0
#endif

#define CHARSRC 0
#define FILESRC 1

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

extern int warnflag;
extern char *leftypath, *leftyoptions, *shellpath;
extern jmp_buf exitljbuf;
extern int idlerunmode;
extern fd_set inputfds;

int init (char *);
void term (void);
char *buildpath (char *, int);
char *buildcommand (char *, char *, int, int, char *);
void warning (char *, int, char *, char *, ...);
void panic1 (char *, int, char *, char *, ...);
void panic2 (char *, int, char *, char *, ...);
#endif /* _COMMON_H */

#ifdef __cplusplus
}
#endif

