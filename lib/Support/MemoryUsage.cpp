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
    std::string ignore;
    std::ifstream ifs("/proc/self/stat", std::ios_base::in);
    for(int i = 1; i <= 22; i++) // we're not interested in the first 22 fields
    	ifs >> ignore;
    
    unsigned long vsize; // swap + ram usage in mb
    ifs >> vsize;
    vsize /= 1048576.0;
		
		ifs.close();
    return vsize;
}
