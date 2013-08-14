//===-- memset.c ----------------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include <stdlib.h>
__attribute__ ((weak)) void *memset(void * dst, int s, size_t count) {
    volatile char * a = dst;
    while (count-- > 0)
      *a++ = s;
    return dst;
}
