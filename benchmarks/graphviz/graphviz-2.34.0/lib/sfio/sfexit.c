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

#include	"sfhdr.h"

/*
**	Any required functions for process exiting.
**	Written by Kiem-Phong Vo
*/

#if PACKAGE_ast
int _AST_already_has_them;
#else

#if !_lib_atexit
#if _lib_onexit

#if __STD_C
int atexit(void (*exitf) (void))
#else
int atexit(exitf)
void (*exitf) ();
#endif
{
#if _lib_onexit
    return onexit(exitf);
#endif
}

#else				/* !_lib_onexit */

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
    int rv;

    vtmtxlock(_Sfmutex);

    if (!(e = (Exit_t *) malloc(sizeof(Exit_t))))
	rv = -1;
    else {
	e->exitf = exitf;
	e->next = Exit;
	Exit = e;
	rv = 0;
    }

    vtmtxunlock(_Sfmutex);

    return rv;
}

#if _exit_cleanup
/* since exit() calls _cleanup(), we only need to redefine _cleanup() */
#if __STD_C
_cleanup(void)
#else
_cleanup()
#endif
{
    Exit_t *e;
    for (e = Exit; e; e = e->next)
	(*e->exitf) ();
    if (_Sfcleanup)
	(*_Sfcleanup) ();
}

#else

/* in this case, we have to redefine exit() itself */
#if __STD_C
exit(int type)
#else
exit(type)
int type;
#endif
{
    Exit_t *e;
    for (e = Exit; e; e = e->next)
	(*e->exitf) ();
    _exit(type);
    return type;
}
#endif				/* _exit_cleanup */

#endif				/* _lib_onexit */

#endif				/* !_lib_atexit */

#if _lib_waitpid
int _Sf_no_need_for_waitpid;
#else

/* we need to supply our own waitpid here so that sfpclose() can wait
** for the right process to die.
*/
typedef struct _wait_ {
    int pid;
    int status;
    struct _wait_ *next;
} Waitpid_t;

static Waitpid_t *Wait;

#ifndef WIN32
#if __STD_C
waitpid(int pid, int *status, int options)
#else
waitpid(pid, status, options)
int pid;
int *status;
int options;
#endif
{
    int id, ps;
    Waitpid_t *w;
    Waitpid_t *last;

    /* we don't know options */
    if (options != 0)
	return -1;

    vtmtxlock(_Sfmutex);

    for (w = Wait, last = NIL(Waitpid_t *); w; last = w, w = w->next) {
	if (pid > 0 && pid != w->pid)
	    continue;

	if (last)
	    last->next = w->next;
	else
	    Wait = w->next;
	if (status)
	    *status = w->status;
	pid = w->pid;
	free(w);

	vtmtxunlock(_Sfmutex);
	return pid;
    }

    while ((id = wait(&ps)) >= 0) {
	if (pid <= 0 || id == pid) {
	    if (status)
		*status = ps;

	    vtmtxunlock(_Sfmutex);
	    return pid;
	}

	if (!(w = (Waitpid_t *) malloc(sizeof(Waitpid_t))))
	    continue;

	w->pid = id;
	w->status = ps;
	w->next = Wait;
	Wait = w;
    }

    vtmtxunlock(_Sfmutex);
    return -1;
}

#endif /*_lib_waitpid*/

#endif				/*!PACKAGE_ast */
#endif
