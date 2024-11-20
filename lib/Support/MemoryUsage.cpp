//===-- MemoryUsage.cpp ---------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "klee/System/MemoryUsage.h"

#include "klee/Config/config.h"
#include "klee/Support/ErrorHandling.h"

#ifdef HAVE_GPERFTOOLS_MALLOC_EXTENSION_H
#include "gperftools/malloc_extension.h"
#endif

#if defined(HAVE_MALLINFO) || defined(HAVE_MALLINFO2)
#include <malloc.h>
#endif
#ifdef HAVE_MALLOC_ZONE_STATISTICS
#include <malloc/malloc.h>
#endif

// ASan Support
//
// When building with ASan the `mallinfo()` function is intercepted and always
// reports zero so we can't use that to report KLEE's memory usage. Instead we
// will use ASan's public interface to query how much memory has been
// allocated.
//
// If and only if ASan is enabled the `KLEE_ASAN_BUILD` macro will be defined.

// Make sure we start with the macro being undefined.
#undef KLEE_ASAN_BUILD

#if defined(__has_feature)
#if __has_feature(address_sanitizer)
#define KLEE_ASAN_BUILD
#endif                              /* __has_feature(address_sanitizer) */
#elif defined(__SANITIZE_ADDRESS__) // no __has_feature support before GCC 14
#define KLEE_ASAN_BUILD
#endif /* defined(__has_feature) */

#ifdef KLEE_ASAN_BUILD
#if __has_include(<sanitizer/allocator_interface.h>)
#include <sanitizer/allocator_interface.h>
#else
// GCC doesn't ship `allocator_interface.h` so just provide the prototype here.
extern "C" {
size_t __sanitizer_get_current_allocated_bytes();
}
#endif /* has_include(<sanitizer/allocator_interface.h>) */
#endif /* KLEE_ASAN_BUILD */

using namespace klee;

size_t util::GetTotalMallocUsage() {
#ifdef KLEE_ASAN_BUILD
  // When building with ASan on Linux `mallinfo()` just returns 0 so use ASan
  // runtime function instead to get used memory.
  return __sanitizer_get_current_allocated_bytes();
#endif

#ifdef HAVE_GPERFTOOLS_MALLOC_EXTENSION_H
  size_t value = 0;
  MallocExtension::instance()->GetNumericProperty(
      "generic.current_allocated_bytes", &value);
  return value;
#elif defined(HAVE_MALLINFO2)
  // niy in tcmalloc
  struct mallinfo2 mi = ::mallinfo2();
  return mi.uordblks + mi.hblkhd;
#elif defined(HAVE_MALLINFO)
  struct mallinfo mi = ::mallinfo();
  // The malloc implementation in glibc (pmalloc2)
  // does not include mmap()'ed memory in mi.uordblks
  // but other implementations (e.g. tcmalloc) do.
#if defined(__GLIBC__)
  return (size_t)(unsigned)mi.uordblks + (unsigned)mi.hblkhd;
#else
  return (unsigned)mi.uordblks;
#endif

#elif defined(HAVE_MALLOC_ZONE_STATISTICS)

  // Memory usage on macOS

  malloc_statistics_t stats;
  malloc_zone_t **zones;
  unsigned int num_zones;

  if (malloc_get_all_zones(0, nullptr, (vm_address_t **)&zones, &num_zones) !=
      KERN_SUCCESS)
    klee_error("malloc_get_all_zones failed.");

  size_t total = 0;
  for (unsigned i = 0; i < num_zones; i++) {
    malloc_zone_statistics(zones[i], &stats);
    total += stats.size_in_use;
  }
  return total;

#else // HAVE_MALLINFO

#warning Cannot get malloc info on this platform
  return 0;

#endif
}
