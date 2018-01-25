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

#include <php.h>
#include "gvc.h"
#include "gvplugin.h"
#include "gvcjob.h"
#include "gvcint.h"

static size_t gv_string_writer (GVJ_t *job, const char *s, size_t len)
{
    return PHPWRITE(s, len);
}

static size_t gv_channel_writer (GVJ_t *job, const char *s, size_t len)
{
    return PHPWRITE(s, len);
}

void gv_string_writer_init(GVC_t *gvc)
{
    gvc->write_fn = gv_string_writer;
}

void gv_channel_writer_init(GVC_t *gvc)
{
    gvc->write_fn = gv_channel_writer;
}

