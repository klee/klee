//===-- klee-replay.h ------------------------------------------*- C++ -*--===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef KLEE_REPLAY_H
#define KLEE_REPLAY_H

#define _LARGEFILE64_SOURCE
#define _FILE_OFFSET_BITS 64

#include "klee/Config/config.h"
// FIXME: This is a hack.
#include "../../runtime/POSIX/fd.h"
#include <sys/time.h>

// temporary directory used for replay
extern char replay_dir[];

// whether to keep the replay directory or delete it
extern int keep_temps;

void replay_create_files(exe_file_system_t *exe_fs);
void replay_delete_files();

void process_status(int status,
		    time_t elapsed,
		    const char *pfx)
  __attribute__((noreturn));

#endif
