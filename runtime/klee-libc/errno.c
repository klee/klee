#include "errno.h"

#ifdef KLEE_FREESTANDING_ERRNO
int errno;
#endif // KLEE_FREESTANDING_ERRNO
