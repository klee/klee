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

#ifndef _EXEC_H
#define _EXEC_H
typedef struct Tonm_t lvar_t;

extern Tobj root, null;
extern Tobj rtno;
extern int Erun;
extern int Eerrlevel, Estackdepth, Eshowbody, Eshowcalls, Eoktorun;

void Einit (void);
void Eterm (void);
Tobj Eunit (Tobj);
Tobj Efunction (Tobj, char *);
#endif /* _EXEC_H */

#ifdef __cplusplus
}
#endif

