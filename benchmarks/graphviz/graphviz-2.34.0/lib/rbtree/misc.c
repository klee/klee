/* $Id$Revision: */
/* vim:set shiftwidth=4 ts=8: */

/**********************************************************
*      See the LICENSE file for copyright infomation.     *
**********************************************************/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "misc.h"
#include <stdio.h>

jmp_buf rb_jbuf;

/***********************************************************************/
/*  FUNCTION:  void Assert(int assertion, char* error)  */
/**/
/*  INPUTS: assertion should be a predicated that the programmer */
/*  assumes to be true.  If this assumption is not true the message */
/*  error is printed and the program exits. */
/**/
/*  OUTPUT: None. */
/**/
/*  Modifies input:  none */
/**/
/*  Note:  If DEBUG_ASSERT is not defined then assertions should not */
/*         be in use as they will slow down the code.  Therefore the */
/*         compiler will complain if an assertion is used when */
/*         DEBUG_ASSERT is undefined. */
/***********************************************************************/


void Assert(int assertion, char* error) {
  if(!assertion) {
    fprintf(stderr, "Assertion Failed: %s\n",error);
    longjmp(rb_jbuf, 1);
  }
}



/***********************************************************************/
/*  FUNCTION:  SafeMalloc */
/**/
/*    INPUTS:  size is the size to malloc */
/**/
/*    OUTPUT:  returns pointer to allocated memory if succesful */
/**/
/*    EFFECT:  mallocs new memory.  If malloc fails, prints error message */
/*             and terminates program. */
/**/
/*    Modifies Input: none */
/**/
/***********************************************************************/

void * SafeMalloc(size_t size) {
  void * result;

  if ( (result = malloc(size)) ) { /* assignment intentional */
    return(result);
  } else {
    fprintf(stderr, "memory overflow: malloc failed in SafeMalloc.");
    /* printf("  Exiting Program.\n"); */
    longjmp(rb_jbuf, 2);
    return(0);
  }
}
/*  NullFunction does nothing it is included so that it can be passed */
/*  as a function to RBTreeCreate when no other suitable function has */
/*  been defined */

void NullFunction(void * junk) { ; }
