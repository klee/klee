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


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#ifndef HAVE_STRERROR
#include <errno.h>

extern int sys_nerr;
extern char *sys_errlist[];

char *strerror(int errorNumber)
{
    if (errorNumber > 0 && errorNumber < sys_nerr) {
	return sys_errlist[errorNumber];
    } else {
	return "";
    }
}
#endif
