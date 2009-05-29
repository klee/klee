//===-- KTest.cpp ---------------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "klee/Internal/ADT/KTest.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define KTEST_VERSION 3
#define KTEST_MAGIC_SIZE 5
#define KTEST_MAGIC "KTEST"

// for compatibility reasons
#define BOUT_MAGIC "BOUT\n"

/***/

static int read_uint32(FILE *f, unsigned *value_out) {
  unsigned char data[4];
  if (fread(data, 4, 1, f)!=1)
    return 0;
  *value_out = (((((data[0]<<8) + data[1])<<8) + data[2])<<8) + data[3];
  return 1;
}

static int write_uint32(FILE *f, unsigned value) {
  unsigned char data[4];
  data[0] = value>>24;
  data[1] = value>>16;
  data[2] = value>> 8;
  data[3] = value>> 0;
  return fwrite(data, 1, 4, f)==4;
}

static int read_string(FILE *f, char **value_out) {
  unsigned len;
  if (!read_uint32(f, &len))
    return 0;
  *value_out = (char*) malloc(len+1);
  if (!*value_out)
    return 0;
  if (fread(*value_out, len, 1, f)!=1)
    return 0;
  (*value_out)[len] = 0;
  return 1;
}

static int write_string(FILE *f, const char *value) {
  unsigned len = strlen(value);
  if (!write_uint32(f, len))
    return 0;
  if (fwrite(value, len, 1, f)!=1)
    return 0;
  return 1;
}

/***/


unsigned kTest_getCurrentVersion() {
  return KTEST_VERSION;
}


static int kTest_checkHeader(FILE *f) {
  char header[KTEST_MAGIC_SIZE];
  if (fread(header, KTEST_MAGIC_SIZE, 1, f)!=1)
    return 0;
  if (memcmp(header, KTEST_MAGIC, KTEST_MAGIC_SIZE) &&
      memcmp(header, BOUT_MAGIC, KTEST_MAGIC_SIZE))
    return 0;
  return 1;
}

int kTest_isKTestFile(const char *path) {
  FILE *f = fopen(path, "rb");
  int res;

  if (!f)
    return 0;
  res = kTest_checkHeader(f);
  fclose(f);
  
  return res;
}

KTest *kTest_fromFile(const char *path) {
  FILE *f = fopen(path, "rb");
  KTest *res = 0;
  unsigned i, version;

  if (!f) 
    goto error;
  if (!kTest_checkHeader(f)) 
    goto error;

  res = (KTest*) calloc(1, sizeof(*res));
  if (!res) 
    goto error;

  if (!read_uint32(f, &version)) 
    goto error;
  
  if (version > kTest_getCurrentVersion())
    goto error;

  res->version = version;

  if (!read_uint32(f, &res->numArgs)) 
    goto error;
  res->args = (char**) calloc(res->numArgs, sizeof(*res->args));
  if (!res->args) 
    goto error;
  
  for (i=0; i<res->numArgs; i++)
    if (!read_string(f, &res->args[i]))
      goto error;

  if (version >= 2) {
    if (!read_uint32(f, &res->symArgvs)) 
      goto error;
    if (!read_uint32(f, &res->symArgvLen)) 
      goto error;
  }

  if (!read_uint32(f, &res->numObjects))
    goto error;
  res->objects = (KTestObject*) calloc(res->numObjects, sizeof(*res->objects));
  if (!res->objects)
    goto error;
  for (i=0; i<res->numObjects; i++) {
    KTestObject *o = &res->objects[i];
    if (!read_string(f, &o->name))
      goto error;
    if (!read_uint32(f, &o->numBytes))
      goto error;
    o->bytes = (unsigned char*) malloc(o->numBytes);
    if (fread(o->bytes, o->numBytes, 1, f)!=1)
      goto error;
  }

  fclose(f);

  return res;
 error:
  if (res) {
    if (res->args) {
      for (i=0; i<res->numArgs; i++)
        if (res->args[i])
          free(res->args[i]);
      free(res->args);
    }
    if (res->objects) {
      for (i=0; i<res->numObjects; i++) {
        KTestObject *bo = &res->objects[i];
        if (bo->name)
          free(bo->name);
        if (bo->bytes)
          free(bo->bytes);
      }
      free(res->objects);
    }
    free(res);
  }

  if (f) fclose(f);

  return 0;
}

int kTest_toFile(KTest *bo, const char *path) {
  FILE *f = fopen(path, "wb");
  unsigned i;

  if (!f) 
    goto error;
  if (fwrite(KTEST_MAGIC, strlen(KTEST_MAGIC), 1, f)!=1)
    goto error;
  if (!write_uint32(f, KTEST_VERSION))
    goto error;
      
  if (!write_uint32(f, bo->numArgs))
    goto error;
  for (i=0; i<bo->numArgs; i++) {
    if (!write_string(f, bo->args[i]))
      goto error;
  }

  if (!write_uint32(f, bo->symArgvs))
    goto error;
  if (!write_uint32(f, bo->symArgvLen))
    goto error;
  
  if (!write_uint32(f, bo->numObjects))
    goto error;
  for (i=0; i<bo->numObjects; i++) {
    KTestObject *o = &bo->objects[i];
    if (!write_string(f, o->name))
      goto error;
    if (!write_uint32(f, o->numBytes))
      goto error;
    if (fwrite(o->bytes, o->numBytes, 1, f)!=1)
      goto error;
  }

  fclose(f);

  return 1;
 error:
  if (f) fclose(f);
  
  return 0;
}

unsigned kTest_numBytes(KTest *bo) {
  unsigned i, res = 0;
  for (i=0; i<bo->numObjects; i++)
    res += bo->objects[i].numBytes;
  return res;
}

void kTest_free(KTest *bo) {
  unsigned i;
  for (i=0; i<bo->numArgs; i++)
    free(bo->args[i]);
  free(bo->args);
  for (i=0; i<bo->numObjects; i++) {
    free(bo->objects[i].name);
    free(bo->objects[i].bytes);
  }
  free(bo->objects);
  free(bo);
}
