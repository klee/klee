#include "stddef.h"

extern void klee_make_symbolic(void *array, size_t nbytes, const char *name);
extern void *malloc(size_t size);
extern void *calloc(size_t num, size_t size);
extern void *realloc(void *ptr, size_t new_size);

void *__klee_wrapped_malloc(size_t size) {
  char retNull;
  klee_make_symbolic(&retNull, sizeof(retNull), "retNullMalloc");
  if (retNull) {
    return 0;
  }
  void *array = malloc(size);
  return array;
}

void *__klee_wrapped_calloc(size_t num, size_t size) {
  char retNull;
  klee_make_symbolic(&retNull, sizeof(retNull), "retNullCalloc");
  if (retNull) {
    return 0;
  }
  void *array = calloc(num, size);
  return array;
}

void *__klee_wrapped_realloc(void *ptr, size_t new_size) {
  char retNull;
  klee_make_symbolic(&retNull, sizeof(retNull), "retNullRealloc");
  if (retNull) {
    return 0;
  }
  void *array = realloc(ptr, new_size);
  return array;
}
