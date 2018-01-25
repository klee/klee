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

#ifndef _BFS_H_
#define _BFS_H_

#include "defs.h"

#ifdef __cplusplus
    class Queue {
      private:
	int *data;
	int queueSize;
	int end;
	int start;
      public:
	 Queue(int size) {
	    data = new int[size];
	     queueSize = size;
	     start = 0;
	     end = 0;
	} ~Queue() {
	    delete[]data;
	} void initQueue(int startVertex) {
	    data[0] = startVertex;
	    start = 0;
	    end = 1;
	}

	bool dequeue(int &vertex) {

	    if (start >= end)
		return false;	/* underflow */

	    vertex = data[start++];
	    return true;

	}

	bool enqueue(int vertex) {
	    if (end >= queueSize)
		return false;	/* overflow */
	    data[end++] = vertex;
	    return true;
	}
    };


    void bfs(int vertex, vtx_data * graph, int n, DistType * dist,
	     Queue & Q);
    void bfs_bounded(int vertex, vtx_data * graph, int n, DistType * dist,
		     Queue & Q, int bound, int *visited_nodes,
		     int &num_visited_nodes);
#else
    typedef struct {
	int *data;
	int queueSize;
	int end;
	int start;
    } Queue;

    extern void mkQueue(Queue *, int);
    extern void freeQueue(Queue *);
    extern void initQueue(Queue *, int startVertex);
    extern boolean deQueue(Queue *, int *);
    extern boolean enQueue(Queue *, int);

    extern void bfs(int, vtx_data *, int, DistType *, Queue *);
    extern int bfs_bounded(int, vtx_data *, int, DistType *, Queue *, int,
			   int *);
#endif

#endif

#ifdef __cplusplus
}
#endif
