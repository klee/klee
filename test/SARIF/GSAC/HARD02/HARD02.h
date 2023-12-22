/***************************************************************************************
 *    Title: GSAC
 *    Author: https://github.com/GSACTech
 *    Date: 2023
 *    Code version: 1.0
 *    Availability: https://github.com/GSACTech/contest
 *
 ***************************************************************************************/

#ifndef HARD02_H
#define HARD02_H

#include <stddef.h>

#define EXPORT_CALL

#ifndef __FILE_defined
#define __FILE_defined 1

struct _IO_FILE;

typedef struct _IO_FILE FILE;

#endif

extern FILE *stderr;
#define stderr stderr

#define FILENAMESIZE 256

static FILE *logFile = NULL;

void EXPORT_CALL lou_logFile(const char *fileName);

#endif // HARD02_H
