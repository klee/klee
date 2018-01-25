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

#ifndef HTMLLEX_H
#define HTMLLEX_H

#include <agxbuf.h>

    extern int initHTMLlexer(char *, agxbuf *, int);
    extern int htmllex(void);
    extern int htmllineno(void);
    extern int clearHTMLlexer(void);
    void htmlerror(const char *);

#endif

#ifdef __cplusplus
}
#endif
