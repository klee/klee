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
// Unfortunately the interface is dependent on the compiler version. It is also
// unfortunate that the way to detect compilation with ASan differs between
// compilers. The preprocessor code below tries to determine if ASan is enabled
// and if so which interface should be used.
//
// If ASan is enabled the `KLEE_ASAN_BUILD` macro will be defined other it will
// be undefined. If `KLEE_ASAN_BUILD` is defined then the
// `ASAN_GET_ALLOCATED_MEM_FUNCTION` macro will defined to the name of the ASan
// function that can be called to get memory allocation

// Make sure we start with the macro being undefined.
#undef KLEE_ASAN_BUILD

// Clang and ASan
#if defined(__has_feature)
#  if __has_feature(address_sanitizer)
#     if __has_include("sanitizer/allocator_interface.h")
#       include <sanitizer/allocator_interface.h>
        // Modern interface
#       define ASAN_GET_ALLOCATED_MEM_FUNCTION __sanitizer_get_current_allocated_bytes
#     else
#       include <sanitizer/asan_interface.h>
        // Deprecated interface.
#       define ASAN_GET_ALLOCATED_MEM_FUNCTION __asan_get_current_allocated_bytes
#     endif /* has_include("sanitizer/allocator_interface.h") */
#    define KLEE_ASAN_BUILD
#  endif /* __has_feature(address_sanitizer) */
#endif /* defined(__has_feature) */

// For GCC and ASan
#ifndef KLEE_ASAN_BUILD
#  if defined(__SANITIZE_ADDRESS__)
     // HACK: GCC doesn't ship `allocator_interface.h`  or `asan_interface.h` so
     // just provide the proto-types here.
     extern "C" {
       size_t __sanitizer_get_current_allocated_bytes();
       size_t __asan_get_current_allocated_bytes(); // Deprecated.
     }
     // HACK: Guess which function to use based on GCC version
#    if __GNUC__ > 4
       // Tested with gcc 5.2, 5.4, and 6.2.1
       // Modern interface
#      define ASAN_GET_ALLOCATED_MEM_FUNCTION __sanitizer_get_current_allocated_bytes
#    else
       // Tested with gcc 4.8 and 4.9
       // Deprecated interface
#      define ASAN_GET_ALLOCATED_MEM_FUNCTION __asan_get_current_allocated_bytes
#    endif
#    define KLEE_ASAN_BUILD
#  endif /* defined(__SANITIZE_ADDRESS__) */
#endif /* ndef KLEE_ASAN_BUILD */

using namespace klee;

size_t util::GetTotalMallocUsage() {
#ifdef KLEE_ASAN_BUILD
  // When building with ASan on Linux `mallinfo()` just returns 0 so use ASan runtime
  // function instead to get used memory.
  return ASAN_GET_ALLOCATED_MEM_FUNCTION();
#endif

#ifdef HAVE_GPERFTOOLS_MALLOC_EXTENSION_H
  size_t value = 0;
  MallocExtension::instance()->GetNumericProperty(
      "generic.current_allocated_bytes", &value);
  return value;
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

  // Support memory usage on Darwin.
  malloc_statistics_t Stats;
  malloc_zone_statistics(malloc_default_zone(), &Stats);
  return Stats.size_in_use;

#else // HAVE_MALLINFO

#warning Cannot get malloc info on this platform
  return 0;

#endif
}
