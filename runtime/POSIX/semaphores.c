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

#include "threads.h"

#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <klee/klee.h>


////////////////////////////////////////////////////////////////////////////////
// POSIX Semaphores
////////////////////////////////////////////////////////////////////////////////
#define SEM_VALUE_MAX 32000
#define __SIZEOF_SEM_T	32

typedef union
{
  char __size[__SIZEOF_SEM_T];
  long int __align;
} sem_posix_t;

static sem_data_t *_get_sem_data(sem_posix_t *sem) {
  sem_data_t *sdata = *((sem_data_t**)sem);

  return sdata;
}

int sem_init(sem_posix_t *sem, int pshared, unsigned int value) {
  if (pshared != 0) 
        klee_report_error(__FILE__, __LINE__, "semaphore shared between processes is not supported", "user.err");
  
  if(value > SEM_VALUE_MAX) {
    errno = EINVAL;
    return -1;
  }
  
  sem_data_t *sdata = (sem_data_t*)malloc(sizeof(sem_data_t));
  memset(sdata, 0, sizeof(sem_data_t));

  *((sem_data_t**)sem) = sdata;

  sdata->wlist = klee_get_wlist();
  sdata->count = value;

  return 0;
}

int sem_destroy(sem_posix_t *sem) {
  sem_data_t *sdata = _get_sem_data(sem);

  free(sdata);

  return 0;
}

static int _atomic_sem_lock(sem_data_t *sdata, char try) {
  sdata->count--;
  if (sdata->count < 0) {
    if (try) {
      sdata->count++;
      errno = EBUSY;
      return -1;
    } else {
      __thread_sleep(sdata->wlist);
    }
  }

  return 0;
}

int sem_wait(sem_posix_t *sem) {
  //__thread_preempt(0);

  sem_data_t *sdata = _get_sem_data(sem);

  int res = _atomic_sem_lock(sdata, 0);

  if (res == 0)
    __thread_preempt(0);

  return res;
}

int sem_trywait(sem_posix_t *sem) {
  //__thread_preempt(0);

  sem_data_t *sdata = _get_sem_data(sem);

  int res = _atomic_sem_lock(sdata, 1);

  if (res == 0)
    __thread_preempt(0);

  return res;
}

static int _atomic_sem_unlock(sem_data_t *sdata) {
  sdata->count++;

  if (sdata->count <= 0)
    __thread_notify_one(sdata->wlist);

  return 0;
}

int sem_post(sem_posix_t *sem) {
  //__thread_preempt(0);

  sem_data_t *sdata = _get_sem_data(sem);

  int res = _atomic_sem_unlock(sdata);

  if (res == 0)
    __thread_preempt(0);

  return res;
}
