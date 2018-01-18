#ifndef __UTIL_H__
#define __UTIL_H__

#include <assert.h>

typedef char *string;
typedef char bool;

#define TRUE 1
#define FALSE 0

void *checked_malloc(int);
string String(char *);

#endif