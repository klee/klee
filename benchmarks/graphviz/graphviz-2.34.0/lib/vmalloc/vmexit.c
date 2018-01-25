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

#include	"vmhdr.h"

/*
**	Any required functions for process exiting.
**	Written by Kiem-Phong Vo, kpv@research.att.com (05/25/93).
*/
#if _PACKAGE_ast || _lib_atexit
int Vm_atexit_already_defined;
#else

#if _lib_onexit

#if __STD_C
int atexit(void (*exitf) (void))
#else
int atexit(exitf)
void (*exitf) ();
#endif
{
    return onexit(exitf);
}

#else				/*!_lib_onexit */

typedef struct _exit_s {
    struct _exit_s *next;
    void (*exitf) _ARG_((void));
} Exit_t;
static Exit_t *Exit;

#if __STD_C
atexit(void (*exitf) (void))
#else
atexit(exitf)
void (*exitf) ();
#endif
{
    Exit_t *e;

    if (!(e = (Exit_t *) malloc(sizeof(Exit_t))))
	return -1;
    e->exitf = exitf;
    e->next = Exit;
    Exit = e;
    return 0;
}

#if __STD_C
void exit(int type)
#else
void exit(type)
int type;
#endif
{
    Exit_t *e;

    for (e = Exit; e; e = e->next)
	(*e->exitf) ();

#if _exit_cleanup
    _cleanup();
#endif

    _exit(type);
}

#endif				/* _lib_onexit || _lib_on_exit */

#endif				/*!PACKAGE_ast */
