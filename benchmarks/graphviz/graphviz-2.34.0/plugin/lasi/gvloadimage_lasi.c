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

#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#if HAVE_SYS_MMAN_H
#include <sys/mman.h>
#endif
#ifdef _MSC_VER
#include <io.h>
#endif

#include "gvplugin_loadimage.h"
#include "agxbuf.h"
#include "utils.h"
#include "gvio.h"

extern shape_desc *find_user_shape(char *name);

typedef enum {
    FORMAT_PS_PS,
} format_type;

static void ps_freeimage(usershape_t *us)
{
#if HAVE_SYS_MMAN_H
    munmap(us->data, us->datasize);
#else
    free(us->data);
#endif
}

/* usershape described by a postscript file */
static void lasi_loadimage_ps(GVJ_t * job, usershape_t *us, boxf b, boolean filled)
{
    assert(job);
    assert(us);
    assert(us->name);

    if (us->data) {
        if (us->datafree != ps_freeimage) {
            us->datafree(us);        /* free incompatible cache data */
            us->data = NULL;
            us->datafree = NULL;
            us->datasize = 0;
        }
    }

    if (!us->data) { /* read file into cache */
        int fd;
	struct stat statbuf;

	if (!gvusershape_file_access(us))
	    return;
	fd = fileno(us->f);
        switch (us->type) {
            case FT_PS:
            case FT_EPS:
		fstat(fd, &statbuf);
		us->datasize = statbuf.st_size;
#if HAVE_SYS_MMAN_H
		us->data = mmap(0, statbuf.st_size, PROT_READ, MAP_SHARED, fd, 0);
#else
		us->data = malloc(statbuf.st_size);
		read(fd, us->data, statbuf.st_size);
#endif
		us->must_inline = TRUE;
                break;
            default:
                break;
        }
        if (us->data)
            us->datafree = ps_freeimage;
	gvusershape_file_release(us);
    }

    if (us->data) {
        gvprintf(job, "gsave %g %g translate newpath\n",
		b.LL.x - (double)(us->x), b.LL.y - (double)(us->y));
        if (us->must_inline)
            epsf_emit_body(job, us);
        else
            gvprintf(job, "user_shape_%d\n", us->macro_id);
        gvprintf(job, "grestore\n");
    }
}

static gvloadimage_engine_t engine_ps = {
    lasi_loadimage_ps
};

gvplugin_installed_t gvloadimage_lasi_types[] = {
    {FORMAT_PS_PS, "eps:lasi", -5, &engine_ps, NULL},
    {FORMAT_PS_PS, "ps:lasi", -5, &engine_ps, NULL},
    {0, NULL, 0, NULL, NULL}
};
