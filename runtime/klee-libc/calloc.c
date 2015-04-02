/*===-- calloc.c ----------------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===*/

#include <stdlib.h>
#include <string.h>

/* DWD - I prefer to be internal */
#if 0
void *calloc(size_t nmemb, size_t size) {
	unsigned nbytes = nmemb * size;
	void *addr = malloc(nbytes);
	if(addr)
		memset(addr, 0, nbytes);
	return addr;
}

/* Always reallocate. */
void *realloc(void *ptr, size_t nbytes) {
	if(!ptr)
		return malloc(nbytes);

	if(!nbytes) {
		free(ptr);
		return 0;
	}

        unsigned copy_nbytes = klee_get_obj_size(ptr);
	/* printf("REALLOC: current object = %d bytes!\n", copy_nbytes); */

	void *addr = malloc(nbytes);
	if(addr) {
	        /* shrinking */
		if(copy_nbytes > nbytes)
			copy_nbytes = nbytes;
		/* printf("REALLOC: copying = %d bytes!\n", copy_nbytes); */
		memcpy(addr, ptr, copy_nbytes);
		free(ptr);
	} 
	return addr;
}
#endif
