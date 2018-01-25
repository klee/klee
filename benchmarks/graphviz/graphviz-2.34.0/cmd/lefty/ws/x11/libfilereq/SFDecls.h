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

/* SFDecls.h */

/* We don't rely on compiler doing the right thing
 * in absence of declarations.
 * C compilers should never have accepted this braindamage.
 * KG <garloff@suse.de>, 2002-01-28
 */

/* SelFile.c */
FILE * SFopenFile (char *, char *, char *, char *);
void SFtextChanged (void);

/* Draw.c */
void SFinitFont (void);
void SFcreateGC (void);
void SFclearList (int, int);
void SFdrawList (int, int);
void SFdrawLists (int);

/* Path.c */
int SFchdir (char *path);
void SFupdatePath (void);
void SFsetText (char *path);

/* Dir.c */
int SFcompareEntries (const void *vp, const void *vq);
int SFgetDir (SFDir *dir);

#ifdef __cplusplus
}
#endif

