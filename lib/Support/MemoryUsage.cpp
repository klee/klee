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

#ifdef HAVE_GPERFTOOLS_MALLOC_EXTENSION_H
#include "gperftools/malloc_extension.h"
#endif

#ifdef HAVE_MALLINFO
#include <malloc.h>
#endif
#ifdef HAVE_MALLOC_MALLOC_H
#include <malloc/malloc.h>
#endif

// Clang and ASan
#if defined(__has_feature)
#if __has_feature(address_sanitizer)
#include <sanitizer/allocator_interface.h>
#define KLEE_ASAN_BUILD
#endif
#endif

// For GCC and ASan
#ifndef KLEE_ASAN_BUILD
#if defined(__SANITIZE_ADDRESS__)
// HACK: GCC doesn't ship `allocator_interface.h` so just provide the
// proto-type here.
extern "C" {
  size_t __sanitizer_get_current_allocated_bytes();
}
#define KLEE_ASAN_BUILD
#endif
#endif

using namespace klee;

size_t util::GetTotalMallocUsage() {
#ifdef KLEE_ASAN_BUILD
  // When building with ASan on Linux `mallinfo()` just returns 0 so use ASan runtime
  // function instead to get used memory.
  return  __sanitizer_get_current_allocated_bytes();
#endif

#ifdef HAVE_GPERFTOOLS_MALLOC_EXTENSION_H
  size_t value = 0;
  MallocExtension::instance()->GetNumericProperty(
      "generic.current_allocated_bytes", &value);
  return value;
#elif HAVE_MALLINFO
  struct mallinfo mi = ::mallinfo();
  // The malloc implementation in glibc (pmalloc2)
  // does not include mmap()'ed memory in mi.uordblks
  // but other implementations (e.g. tcmalloc) do.
#if defined(__GLIBC__)
  return (size_t)(unsigned)mi.uordblks + (unsigned)mi.hblkhd;
#else
  return (unsigned)mi.uordblks;
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
