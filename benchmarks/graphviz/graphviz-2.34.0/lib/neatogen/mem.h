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



#ifndef MEMORY_H
#define MEMORY_H

#ifndef NULL
#define NULL 0
#endif

    /* Support for freelists */

    typedef struct freelist {
	struct freenode *head;	/* List of free nodes */
	struct freeblock *blocklist;	/* List of malloced blocks */
	int nodesize;		/* Size of node */
    } Freelist;

    extern void *getfree(Freelist *);
    extern void freeinit(Freelist *, int);
    extern void makefree(void *, Freelist *);

#endif


#ifdef __cplusplus
}
#endif
