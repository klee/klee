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

#ifndef _PARSE_H
#define _PARSE_H
typedef struct Psrc_t {
    int flag;
    char *s;
    FILE *fp;
    int tok;
    int lnum;
} Psrc_t;

void Pinit (void);
void Pterm (void);
Tobj Punit (Psrc_t *);
Tobj Pfcall (Tobj, Tobj);
Tobj Pfunction (char *, int);
#endif /* _PARSE_H */

#ifdef __cplusplus
}
#endif
