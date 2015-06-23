/*
 * Cloud9 Parallel Symbolic Execution Engine
 *
 * Copyright (c) 2011, Dependable Systems Laboratory, EPFL
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the Dependable Systems Laboratory, EPFL nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE DEPENDABLE SYSTEMS LABORATORY, EPFL BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Contributors:
 *
 * Stefan Bucur <stefan.bucur@epfl.ch>
 * Vlad Ureche <vlad.ureche@epfl.ch>
 * Cristian Zamfir <cristian.zamfir@epfl.ch>
 * Ayrat Khalimov <ayrat.khalimov@epfl.ch>
 * Prof. George Candea <george.candea@epfl.ch>
 *
 * External Contributors:
 * Calin Iorgulescu <calin.iorgulescu@gmail.com>
 * Tudor Cazangiu <tudor.cazangiu@gmail.com>
 *
 * Stefan Bucur <sbucur@google.com> (contributions done while at Google)
 * Lorenzo Martignoni <martignlo@google.com>
 * Burak Emir <bqe@google.com>
 *
 */

#ifndef THREADS_H_
#define THREADS_H_

#include <stdint.h>
#include <sys/types.h>
#include <klee/klee.h>

#define MAX_THREADS     16

typedef uint64_t wlist_id_t;

#define DEFAULT_THREAD  0

#define PTHREAD_ONCE_INIT       0

#define STATIC_MUTEX_VALUE      0
#define STATIC_CVAR_VALUE       0
#define STATIC_BARRIER_VALUE    0
#define STATIC_RWLOCK_VALUE     0

#define PTHREAD_BARRIER_SERIAL_THREAD    -1

typedef struct {
  wlist_id_t wlist;

  void *ret_value;

  char allocated;
  char terminated;
  char joinable;
} thread_data_t;

typedef struct {
  wlist_id_t wlist;

  char taken;
  unsigned int owner;
  int count;
  unsigned int queued;

  char allocated;
} mutex_data_t;

typedef struct {
  wlist_id_t wlist;

  mutex_data_t *mutex;
  unsigned int queued;
} condvar_data_t;

typedef struct {
  wlist_id_t wlist;

  unsigned int curr_event;
  unsigned int left;
  unsigned int init_count;
} barrier_data_t;

typedef struct {
  wlist_id_t wlist_readers;
  wlist_id_t wlist_writers;

  unsigned int nr_readers;
  unsigned int nr_readers_queued;
  unsigned int nr_writers_queued;
  unsigned int writer;
  char writer_taken;
} rwlock_data_t;

typedef struct {
    wlist_id_t wlist;

    int count;
    char allocated;
} sem_data_t;

typedef struct {
  thread_data_t threads[MAX_THREADS];
} tsync_data_t;

extern tsync_data_t __tsync;

void klee_init_threads(void);

static inline void __thread_preempt(int yield) {
  klee_thread_preempt(yield);
}

static inline void __thread_sleep(uint64_t wlist) {
  klee_thread_sleep(wlist);
}

static inline void __thread_notify(uint64_t wlist, int all) {
  klee_thread_notify(wlist, all);
}

static inline void __thread_notify_one(uint64_t wlist) {
  __thread_notify(wlist, 0);
}

static inline void __thread_notify_all(uint64_t wlist) {
  __thread_notify(wlist, 1);
}

#endif /* THREADS_H_ */
