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

/*
 * Glenn Fowler
 * AT&T Research
 *
 * expression library
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef GVDLL
#define _BLD_sfio 1
#endif

#include <exlib.h>
#include <string.h>

/*
 * allocate a new expression program environment
 */

Expr_t*
exopen(register Exdisc_t* disc)
{
	register Expr_t*	program;
	register Exid_t*	sym;
	int			debug;

	if (!(program = newof(0, Expr_t, 1, 0)))
		return 0;
	program->symdisc.key = offsetof(Exid_t, name);
	debug = getenv("VMDEBUG") != 0;
	if (!(program->symbols = dtopen(&program->symdisc, Dtset)) ||
	    !(program->tmp = sfstropen()) ||
	    !(program->vm = (debug ? vmopen(Vmdcsbrk, Vmdebug, VM_DBCHECK|VM_DBABORT) : vmopen(Vmdcheap, Vmbest, 0))) ||
	    !(program->ve = (debug ? vmopen(Vmdcsbrk, Vmdebug, VM_DBCHECK|VM_DBABORT) : vmopen(Vmdcheap, Vmbest, 0))))
	{
		exclose(program, 1);
		return 0;
	}
	program->id = "libexpr:expr";
	program->disc = disc;
	setcontext(program);
	program->file[0] = sfstdin;
	program->file[1] = sfstdout;
	program->file[2] = sfstderr;
	strcpy(program->main.name, "main");
	program->main.lex = PROCEDURE;
	program->main.index = PROCEDURE;
	dtinsert(program->symbols, &program->main);
	if (!(disc->flags & EX_PURE))
		for (sym = exbuiltin; *sym->name; sym++)
			dtinsert(program->symbols, sym);
	if ((sym = disc->symbols))
		for (; *sym->name; sym++)
			dtinsert(program->symbols, sym);
	return program;
}
