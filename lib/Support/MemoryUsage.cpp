//===-- MemoryUsage.cpp ---------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "klee/Internal/System/MemoryUsage.h"

#include "klee/Config/config.h"
#include "../Core/Common.h"

#ifdef HAVE_MALLINFO
#include <malloc.h>
#endif
#ifdef HAVE_MALLOC_MALLOC_H
#include <malloc/malloc.h>
#endif

using namespace klee;

#ifdef OLDMEMUSAGE
#include <malloc.h>
size_t util::GetTotalMallocUsage() {
#ifdef HAVE_MALLINFO
  struct mallinfo mi = ::mallinfo();
  // The malloc implementation in glibc (pmalloc2)
  // does not include mmap()'ed memory in mi.uordblks
  // but other implementations (e.g. tcmalloc) do.
#if defined(__GLIBC__)
  return mi.uordblks + mi.hblkhd;
#else
  return mi.uordblks;
#endif

#elif defined(HAVE_MALLOC_ZONE_STATISTICS) && defined(HAVE_MALLOC_MALLOC_H)

  // Support memory usage on Darwin.
  malloc_statistics_t Stats;
  malloc_zone_statistics(malloc_default_zone(), &Stats);
  return Stats.size_in_use;

#else // HAVE_MALLINFO

#warning Cannot get malloc info on this platform
  return 0;

#endif
}

#else
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <dlfcn.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

static void *(*real_malloc)(size_t size) = 0;
static void *(*real_calloc)(size_t nmemb, size_t size);
static void *(*real_realloc)(void *ptr, size_t size);
static void *(*real_memalign)(size_t blocksize, size_t bytes);
static void *(*real_valloc)(size_t size);
static int (*real_posix_memalign)(void **memptr, size_t alignment, size_t size);
static void (*real_free)(void *ptr);
static void *(*temp_malloc)(size_t size);
static void *(*temp_calloc)(size_t nmemb, size_t size);
static void *(*temp_realloc)(void *ptr, size_t size);
static void *(*temp_memalign)(size_t blocksize, size_t bytes);
static void *(*temp_valloc)(size_t size);
static int (*temp_posix_memalign)(void **memptr, size_t alignment, size_t size);
static void (*temp_free)(void *ptr);

size_t allocatedMemory = 0;

size_t malloc_wrapper_getmemory() { return allocatedMemory; }

char tmpbuf[1024];
unsigned long tmppos = 0;
unsigned long tmpallocs = 0;

void __attribute__((constructor)) hookfns();

void *dummy_malloc(size_t size) {
  void *retptr = tmpbuf + tmppos;
  if (tmppos + size >= sizeof(tmpbuf))
    exit(1);
  tmppos += size;
  ++tmpallocs;
  return retptr;
}
void *dummy_calloc(size_t nmemb, size_t size) {
  char *ptr = (char *)dummy_malloc(nmemb * size);

  if (!ptr)
    return ptr;

  unsigned int i = 0;
  for (; i < nmemb * size; ++i)
    *((char *)(ptr + i)) = '\0';
  return ptr;
}
void dummy_free(void *ptr) {}

void __attribute__((constructor)) hookfns() {
  if (real_malloc != 0) return;
  real_malloc = dummy_malloc;
  real_calloc = dummy_calloc;
  real_realloc = 0;
  real_free = dummy_free;
  real_memalign = 0;
  real_valloc = 0;
  real_posix_memalign = 0;
  temp_malloc = (void *(*)(size_t))dlsym(RTLD_NEXT, "malloc");

  temp_calloc = (void *(*)(size_t, size_t))dlsym(RTLD_NEXT, "calloc");
  temp_realloc = (void *(*)(void *, size_t))dlsym(RTLD_NEXT, "realloc");
  temp_free = (void (*)(void *))dlsym(RTLD_NEXT, "free");
  temp_memalign = (void *(*)(size_t, size_t))dlsym(RTLD_NEXT, "memalign");
  temp_valloc = (void *(*)(size_t))dlsym(RTLD_NEXT, "valloc");
  temp_posix_memalign =
      (int (*)(void **, size_t, size_t))dlsym(RTLD_NEXT, "posix_memalign");
  if (!temp_malloc || !temp_calloc || !temp_realloc || !temp_memalign ||
      !temp_valloc || !temp_posix_memalign || !temp_free) {
    fprintf(stderr, "Error in `dlsym`: %s\n", dlerror());
    exit(1);
  }
  real_malloc = temp_malloc;
  real_calloc = temp_calloc;
  real_realloc = temp_realloc;
  real_free = temp_free;
  real_memalign = temp_memalign;
  real_valloc = temp_valloc;
  real_posix_memalign = temp_posix_memalign;
  //  fprintf(stderr, "Constructor\n");
}

// Define size for the header
// MSB used if aligned
// preceeding field points to begin of array
// TODO draw picture
#define HSIZE sizeof(size_t)*2
#define HEADER(ptr) ((size_t*)(ptr) - 1)
#define GET_SIZE(ptr) (*HEADER(ptr))

inline void * get_orig_ptr(void *maybe_aligned_ptr) {
  if (!maybe_aligned_ptr)
    return 0;

  return HEADER(HEADER(maybe_aligned_ptr));
}

inline void * update_meta(void *ptr, size_t new_size) {
  if (!ptr)
    return ptr;

  // save size information
  *((size_t *)ptr+1) = new_size;
  // save ptr information
  *((size_t *)ptr) = (size_t) ptr;

  // update gloabl size counter
  __sync_fetch_and_add(&allocatedMemory, new_size);

  // Return pointer referencing after header
  return (char*)ptr + HSIZE;
}

inline void * update_meta_aligned(void *ptr, size_t new_size, size_t alignment) {
  if (!ptr)
    return ptr;

  // update gloabl size counter
  __sync_fetch_and_add(&allocatedMemory, new_size);

  // take pointer starting after alignment
  char * new_ptr = (char*)ptr +alignment;

  // save size information
  *((size_t *)new_ptr-1) = new_size;

  // save ptr information
  *((size_t *)new_ptr-2) = (size_t) ptr;

  // Return pointer referencing after header
  return new_ptr;
}

void *malloc(size_t size) {
  // XXX make a dynamic library out of it or call constructor explicitly
  if (!real_malloc)
    hookfns();

  size_t new_size = size + HSIZE;
  void * ptr = real_malloc(new_size);

  void * new_ptr = update_meta(ptr, new_size);
  return new_ptr;
}

void *calloc(size_t nmemb, size_t size) {
  // XXX make a dynamic library out of it or call constructor explicitly
  if (!real_malloc)
    hookfns();

  size_t new_size = size + HSIZE;

  void *ptr = real_calloc(nmemb, new_size);

  return update_meta(ptr, new_size);
}

void *realloc(void *ptr, size_t size) {
  size_t new_size = size + HSIZE;
  size_t old_size = (ptr ? GET_SIZE(ptr) : 0);

  void *new_ptr = real_realloc(
	get_orig_ptr(ptr), new_size);

  return update_meta(new_ptr, new_size - old_size);
}

void free(void *ptr) {
  if (!ptr)
    return;

  size_t oldSize = GET_SIZE(ptr);
  __sync_fetch_and_sub(&allocatedMemory, oldSize);

  real_free(get_orig_ptr(ptr));
}

void *memalign(size_t alignment, size_t bytes) {
  alignment = alignment>HSIZE ? alignment : HSIZE;
  void * new_ptr = real_memalign(alignment, bytes + alignment);
  if (!new_ptr)
    return 0;

  void * updated_ptr = update_meta_aligned(new_ptr, bytes + alignment, alignment);
  return updated_ptr;
}

int posix_memalign(void **memptr, size_t alignment, size_t size) {
  alignment = alignment > HSIZE ? alignment : HSIZE;
  int failed = real_posix_memalign(memptr, alignment, size + alignment);

  if (!failed) {
     *memptr = update_meta_aligned(*memptr, size + alignment, alignment);
  }
  return failed;
}

void *valloc(size_t size) {
  size_t new_size = size + HSIZE;
  void *ptr = real_valloc(new_size);
  
  return update_meta(ptr, new_size);
}

size_t util::GetTotalMallocUsage() { return malloc_wrapper_getmemory(); }
#endif
