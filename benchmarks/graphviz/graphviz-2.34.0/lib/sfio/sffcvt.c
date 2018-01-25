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

#include	"sfhdr.h"

#if __STD_C
char *sffcvt(double dval, int n_digit, int *decpt, int *sign)
#else
char *sffcvt(dval, n_digit, decpt, sign)
double dval;			/* value to convert */
int n_digit;			/* number of digits wanted */
int *decpt;			/* to return decimal point */
int *sign;			/* to return sign */
#endif
{
    return _sfcvt(&dval, n_digit, decpt, sign, 0);
}
