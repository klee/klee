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

#include <exlib.h>

/*
 * library error handler
 */

void
exerror(const char* format, ...)
{
	Sfio_t*	sp;

	if (expr.program->disc->errorf && !expr.program->errors && (sp = sfstropen()))
	{
		va_list	ap;
		char*	s;
		char	buf[64];

		expr.program->errors = 1;
		excontext(expr.program, buf, sizeof(buf));
		sfputr(sp, buf, -1);
		sfputr(sp, "\n -- ", -1);
		va_start(ap, format);
		sfvprintf(sp, format, ap);
		va_end(ap);
		if (!(s = sfstruse(sp)))
			s = "out of space";
		(*expr.program->disc->errorf)(expr.program, expr.program->disc, (expr.program->disc->flags & EX_FATAL) ? 3 : 2, "%s", s);
		sfclose(sp);
	}
	else if (expr.program->disc->flags & EX_FATAL)
		exit(1);
}

void 
exwarn(const char *format, ...)
{
	Sfio_t *sp;

	if (expr.program->disc->errorf && (sp = sfstropen())) {
		va_list ap;
		char *s;
		char buf[64];

		excontext(expr.program, buf, sizeof(buf));
		sfputr(sp, buf, -1);
		sfputr(sp, "\n -- ", -1);
		va_start(ap, format);
		sfvprintf(sp, format, ap);
		va_end(ap);
		s = sfstruse(sp);
		(*expr.program->disc->errorf) (expr.program, expr.program->disc,
				       ERROR_WARNING, "%s", s);
	sfclose(sp);
	}
}
