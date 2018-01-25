/* $Id$Revision: */
/* vim:set shiftwidth=4 ts=8: */

/**********************************************************
*      See the LICENSE file for copyright infomation.     *
**********************************************************/

#ifndef INC_E_MISC_
#define INC_E_MISC_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <setjmp.h>

extern jmp_buf rb_jbuf;

/*  CONVENTIONS:  All data structures for red-black trees have the prefix */
/*                "rb_" to prevent name conflicts. */
/*                                                                      */
/*                Function names: Each word in a function name begins with */
/*                a capital letter.  An example funcntion name is  */
/*                CreateRedTree(a,b,c). Furthermore, each function name */
/*                should begin with a capital letter to easily distinguish */
/*                them from variables. */
/*                                                                     */
/*                Variable names: Each word in a variable name begins with */
/*                a capital letter EXCEPT the first letter of the variable */
/*                name.  For example, int newLongInt.  Global variables have */
/*                names beginning with "g".  An example of a global */
/*                variable name is gNewtonsConstant. */

void Assert(int assertion, char* error);
void * SafeMalloc(size_t size);

#ifdef __cplusplus
}
#endif

#endif

