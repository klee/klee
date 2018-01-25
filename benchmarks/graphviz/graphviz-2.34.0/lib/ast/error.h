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

/*
 * standalone mini error interface
 */

#ifndef _ERROR_H
#define _ERROR_H

#include <stdarg.h>
#include <errno.h>

    typedef struct Error_info_s {
	int errors;
	int indent;
	int line;
	int warnings;
	int trace;
	char *file;
	char *id;
    } Error_info_t;

#ifndef ERROR_catalog
#define ERROR_catalog(t)        t
#endif

#define ERROR_INFO	0	/* info message -- no err_id    */
#define ERROR_WARNING	1	/* warning message              */
#define ERROR_ERROR	2	/* error message -- no err_exit */
#define ERROR_FATAL	3	/* error message with err_exit  */
#define ERROR_PANIC	ERROR_LEVEL	/* panic message with err_exit  */

#define ERROR_LEVEL	0x00ff	/* level portion of status      */
#define ERROR_SYSTEM	0x0100	/* report system errno message  */
#define ERROR_USAGE	0x0800	/* usage message                */

#define error_info	_err_info
#define error		_err_msg
#define errorv		_err_msgv

    extern Error_info_t error_info;

    extern void setTraceLevel (int);
    extern void setErrorLine (int);
    extern void setErrorFileLine (char*, int);
    extern void setErrorId (char*);
    extern void setErrorErrors (int);
    extern int  getErrorErrors (void);

    extern void error(int, ...);
    extern void errorf(void *, void *, int, ...);
    extern void errorv(const char *, int, va_list);

#endif

#ifdef __cplusplus
}
#endif
