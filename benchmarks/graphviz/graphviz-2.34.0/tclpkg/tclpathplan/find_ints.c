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

#include "simple.h"
#include <stdio.h>
#include <stdlib.h>

void find_intersection(struct vertex *l, struct vertex *m,
		       struct intersection ilist[], struct data *input);
static int gt(struct vertex **i, struct vertex **j);

void find_ints(struct vertex vertex_list[],
	struct polygon polygon_list[],
	struct data *input,
	struct intersection ilist[])
{
    int i, j, k;
    struct active_edge_list all;
    struct active_edge *new, *tempa;
    struct vertex *pt1, *pt2, *templ, **pvertex;
/*      char *malloc(); */

    input->ninters = 0;
    all.first = all.final = NIL;
    all.number = 0;

    pvertex = (struct vertex **)
	malloc((input->nvertices) * sizeof(struct vertex *));

    for (i = 0; i < input->nvertices; i++)
	pvertex[i] = vertex_list + i;

/* sort vertices by x coordinate	*/
    qsort(pvertex, input->nvertices, sizeof(struct vertex *),
    	  (int (*)(const void *, const void *))gt);

/* walk through the vertices in order of increasing x coordinate	*/
    for (i = 0; i < input->nvertices; i++) {
	pt1 = pvertex[i];
	templ = pt2 = prior(pvertex[i]);
	for (k = 0; k < 2; k++) {	/* each vertex has 2 edges */
	    switch (gt(&pt1, &pt2)) {

	    case -1:		/* forward edge, test and insert      */

		for (tempa = all.first, j = 0; j < all.number;
		     j++, tempa = tempa->next)
		    find_intersection(tempa->name, templ, ilist, input);	/* test */

		new =
		    (struct active_edge *)
		    malloc(sizeof(struct active_edge));
		if (all.number == 0) {
		    all.first = new;
		    new->last = NIL;
		} /* insert */
		else {
		    all.final->next = new;
		    new->last = all.final;
		}

		new->name = templ;
		new->next = NIL;
		templ->active = new;
		all.final = new;
		all.number++;

		break;		/* end of case -1       */

	    case 1:		/* backward edge, delete        */

		if ((tempa = templ->active) == NIL) {
		    fprintf(stderr,
			    "\n***ERROR***\n trying to delete a non line\n");
		    exit(1);
		}
		if (all.number == 1)
		    all.final = all.first = NIL;	/* delete the line */
		else if (tempa == all.first) {
		    all.first = all.first->next;
		    all.first->last = NIL;
		} else if (tempa == all.final) {
		    all.final = all.final->last;
		    all.final->next = NIL;
		} else {
		    tempa->last->next = tempa->next;
		    tempa->next->last = tempa->last;
		}
		free((char *) tempa);
		all.number--;
		templ->active = NIL;
		break;		/* end of case 1        */

	    }			/* end switch   */

	    pt2 = after(pvertex[i]);
	    templ = pvertex[i];	/*second neighbor */
	}			/* end k for loop       */
    }				/* end i for loop       */
}

static int gt(struct vertex **i, struct vertex **j)
{				/* i > j if i.x > j.x or i.x = j.x and i.y > j.y  */
    double t;
    if ((t = (*i)->pos.x - (*j)->pos.x) != 0.)
	return ((t > 0.) ? 1 : -1);
    if ((t = (*i)->pos.y - (*j)->pos.y) == 0.)
	return (0);
    else
	return ((t > 0.) ? 1 : -1);
}
