#include <errno.h>

#undef errno
int errno __attribute__((weak));

int __attribute__((weak)) *__errno_location(void)
{
	return &errno;
}
