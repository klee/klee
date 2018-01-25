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

#include "exlib.h"
#include <string.h>

int
exrewind(Expr_t* ex)
{
	register int	n;

	if (ex->linewrap)
	{
		exerror("too much pushback");
		return -1;
	}
	if (!ex->input->pushback && !(ex->input->pushback = oldof(0, char, sizeof(ex->line), 3)))
	{
		exnospace();
		return -1;
	}
	if ((n = ex->linep - ex->line))
		memcpy(ex->input->pushback, ex->line, n);
	if (ex->input->peek)
	{
		ex->input->pushback[n++] = ex->input->peek;
		ex->input->peek = 0;
	}
	ex->input->pushback[n++] = ' ';
	ex->input->pushback[n] = 0;
	ex->input->pp = ex->input->pushback;
	ex->input->nesting = ex->nesting;
	setcontext(ex);
	return 0;
}

void
exstatement(Expr_t* ex)
{
	ex->nesting = ex->input->nesting;
	setcontext(ex);
}
