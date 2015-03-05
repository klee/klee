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
 * All contributors are listed in CLOUD9-AUTHORS file.
 *
 */

#include "multiprocess.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <klee/klee.h>

////////////////////////////////////////////////////////////////////////////////
// The PThreads API
////////////////////////////////////////////////////////////////////////////////

pthread_t pthread_self(void) {
  pthread_t result;

  klee_get_context(&result);

  return result;
}

int pthread_create(pthread_t *thread, const pthread_attr_t *attr,
    void *(*start_routine)(void*), void *arg) {

  if (INJECT_FAULT(pthread_create, EAGAIN)) {
    return -1;
  }

  unsigned int newIdx;
  STATIC_LIST_ALLOC(__tsync.threads, newIdx);

  if (newIdx == MAX_THREADS) {
    errno = EAGAIN;
    return -1;
  }

  thread_data_t *tdata = &__tsync.threads[newIdx];
  tdata->terminated = 0;
  tdata->joinable = 1; // TODO: Read this from an attribute
  tdata->wlist = klee_get_wlist();

  klee_thread_create(newIdx, start_routine, arg);

  *thread = newIdx;

  return 0;
}

void pthread_exit(void *value_ptr) {
  unsigned int idx = pthread_self();
  thread_data_t *tdata = &__tsync.threads[idx];

  if (tdata->joinable) {
    tdata->terminated = 1;
    tdata->ret_value = value_ptr;

    __thread_notify_all(tdata->wlist);
  } else {
    STATIC_LIST_CLEAR(__tsync.threads, idx);
  }

  klee_thread_terminate(); // Does not return
}


int pthread_join(pthread_t thread, void **value_ptr) {
  if (thread >= MAX_THREADS) {
    errno = ESRCH;
    return -1;
  }

  if (thread == pthread_self()) {
    errno = EDEADLK;
    return -1;
  }

  thread_data_t *tdata = &__tsync.threads[thread];

  if (!tdata->allocated) {
    errno = ESRCH;
    return -1;
  }

  if (!tdata->joinable) {
    errno = EINVAL;
    return -1;
  }

  if (!tdata->terminated)
    __thread_sleep(tdata->wlist);

  if (value_ptr) {
    *value_ptr = tdata->ret_value;
  }

  STATIC_LIST_CLEAR(__tsync.threads, thread);

  return 0;
}

int pthread_detach(pthread_t thread) {
  if (thread >= MAX_THREADS) {
    errno = ESRCH;
  }

  thread_data_t *tdata = &__tsync.threads[thread];

  if (!tdata->allocated) {
    errno = ESRCH;
    return -1;
  }

  if (!tdata->joinable) {
    errno = EINVAL;
    return -1;
  }

  if (tdata->terminated) {
    STATIC_LIST_CLEAR(__tsync.threads, thread);
  } else {
    tdata->joinable = 0;
  }

  return 0;
}

int pthread_attr_init(pthread_attr_t *attr) {
  klee_warning("pthread_attr_init does nothing");
  return 0;
}

int pthread_attr_destroy(pthread_attr_t *attr) {
  klee_warning("pthread_attr_destroy does nothing");
  return 0;
}

int pthread_once(pthread_once_t *once_control, void (*init_routine)(void)) {
  if (*once_control == 0) {
    init_routine();

    *once_control = 1;
  }

  return 0;
}

int pthread_equal(pthread_t thread1, pthread_t thread2) {
  return thread1 == thread2;
}
