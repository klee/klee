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

#if defined(HAVE_MALLINFO) || defined(HAVE_MALLOC_INFO)
#include <malloc.h>
#include <stdio.h>
#include <string.h>
#endif
#ifdef HAVE_MALLOC_MALLOC_H
#include <malloc/malloc.h>
#endif

using namespace klee;

size_t util::GetTotalMallocUsage() {
#ifdef HAVE_MALLOC_INFO
  char *mib;
  size_t misize;
  FILE* mistrm = ::open_memstream(&mib, &misize);
  if (mistrm && ::malloc_info(0, mistrm) == 0) {
    fclose(mistrm);
    size_t fastAvailable = 0, restAvailable = 0, allocated = 0, mmapped = 0;
    if (strstr(mib, "<malloc version=\"1\">")) {
      const char fastAvailableTag[] = "<total type=\"fast\" count=";
      const char restAvailableTag[] = "<total type=\"rest\" count=";
      const char allocatedTag[] = "<system type=\"current\"";
      const char mmappedTag[] = "<total type=\"mmap\" count=\"";

      for (char *needle = mib; needle; needle = strstr(needle+1, fastAvailableTag))
        sscanf(needle, "%*s %*s %*s size=\"%lu\"", &fastAvailable);
      for (char *needle = mib; needle; needle = strstr(needle+1, restAvailableTag))
        sscanf(needle, "%*s %*s %*s size=\"%lu\"", &restAvailable);
      for (char *needle = mib; needle; needle = strstr(needle+1, allocatedTag))
        sscanf(needle, "%*s %*s size=\"%lu\"", &allocated);
      for (char *needle = mib; needle; needle = strstr(needle+1, mmappedTag))
        sscanf(needle, "%*s %*s %*s size=\"%lu\"", &mmapped);
    }
    free(mib);
#if defined(HAVE_MALLINFO) && defined(__GLIBC__)
    if (!mmapped) {
      struct mallinfo mi = ::mallinfo();
      mmapped = mi.hblkhd;
    }
#endif
    return allocated+mmapped-restAvailable-fastAvailable;
  }
  if (mistrm) {
    fclose(mistrm);
  }
  return 0;
#elif defined HAVE_MALLINFO
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
