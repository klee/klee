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

#ifndef WIN32

#include        <unistd.h>
#include	<sys/types.h>
#include	<sys/times.h>
#include	<sys/param.h>



#ifndef HZ
#define HZ 60
#endif
typedef struct tms mytime_t;
#define GET_TIME(S) times(&(S))
#define DIFF_IN_SECS(S,T) ((S.tms_utime + S.tms_stime - T.tms_utime - T.tms_stime)/(double)HZ)

#else

#include	<time.h>
#include "render.h"
#include    "utils.h"

typedef clock_t mytime_t;
#define GET_TIME(S) S = clock()
#define DIFF_IN_SECS(S,T) ((S - T) / (double)CLOCKS_PER_SEC)

#endif


static mytime_t T;

void start_timer(void)
{
    GET_TIME(T);
}

double elapsed_sec(void)
{
    mytime_t S;
    double rv;

    GET_TIME(S);
    rv = DIFF_IN_SECS(S, T);
    return rv;
}
