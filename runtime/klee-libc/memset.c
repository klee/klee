//===-- memset.c ----------------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include <stdlib.h>

void *memset(void * dst, int s, size_t count) {
    char * a = dst;
    while (count-- > 0)
      *a++ = s;
    return dst;
}
