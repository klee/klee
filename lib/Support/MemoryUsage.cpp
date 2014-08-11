//===-- MemoryUsage.cpp ---------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "klee/Internal/System/MemoryUsage.h"
#include <malloc.h>
#include <iostream>
#include <fstream>
#include <sys/types.h>
#include <unistd.h>
#include <string>

using namespace klee;

size_t util::GetTotalMallocUsage() {
    double residentSet = 0.0;

    long rss;
    std::string ignore;
    std::ifstream ifs("/proc/self/stat", std::ios_base::in);
    ifs >> ignore >> ignore >> ignore >> ignore >> ignore >> ignore >> ignore 
        >> ignore >> ignore >> ignore >> ignore >> ignore >> ignore >> ignore 
        >> ignore >> ignore >> ignore >> ignore >> ignore >> ignore >> ignore 
        >> ignore >> ignore >> rss;

    long pageSizeKb = sysconf(_SC_PAGE_SIZE) / 1024; // in case x86-64 is configured to use 2MB pages
    residentSet = rss * pageSizeKb / 1024.0;
    return residentSet;
}
