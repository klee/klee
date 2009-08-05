//===-- klee-replay.h ------------------------------------------*- C++ -*--===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef __KLEE_REPLAY_H__
#define __KLEE_REPLAY_H__

#define _LARGEFILE64_SOURCE
#define _FILE_OFFSET_BITS 64

// FIXME: This is a hack.
#include "../../runtime/POSIX/fd.h"
#include <sys/time.h>

void replay_create_files(exe_file_system_t *exe_fs);

void process_status(int status,
		    time_t elapsed,
		    const char *pfx)
  __attribute__((noreturn));

#endif

