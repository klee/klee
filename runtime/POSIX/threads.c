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

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

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
  unsigned int newIdx = MAX_THREADS;
  unsigned int i;
  for (i = 0; i < MAX_THREADS; i++) {
    if (!__tsync.threads[i].allocated) {
      __tsync.threads[i].allocated = 1;
      newIdx = i;
      break;
    }
  }

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

  __thread_preempt(0);

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
    memset(&__tsync.threads[idx], 0, sizeof(__tsync.threads[idx]));
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

  memset(&__tsync.threads[thread], 0, sizeof(__tsync.threads[thread]));

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
    memset(&__tsync.threads[thread], 0, sizeof(__tsync.threads[thread]));
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

////////////////////////////////////////////////////////////////////////////////
// POSIX Mutexes
////////////////////////////////////////////////////////////////////////////////

static void _mutexattr_init(pthread_mutexattr_t *attr) {
  memset(attr,0,sizeof(pthread_mutexattr_t));
}

static int _get_mutexattr_data(const pthread_mutexattr_t *attr) {
  return attr->__align;
}

static void _set_mutexattr_data(pthread_mutexattr_t *attr, int val) {
  attr->__align = val;
}

int pthread_mutexattr_init(pthread_mutexattr_t *attr)
{
  _mutexattr_init(attr);

  return 0;
}
int pthread_mutexattr_destroy(pthread_mutexattr_t *attr)
{
  return 0;
}
int pthread_mutexattr_settype(pthread_mutexattr_t *attr, int type)
{
  _set_mutexattr_data(attr, type);
  return 0;
}

static void _mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *attr) {
  mutex_data_t *mdata = (mutex_data_t*)malloc(sizeof(mutex_data_t));
  memset(mdata, 0, sizeof(mutex_data_t));

  *((mutex_data_t**)mutex) = mdata;

  mdata->wlist = klee_get_wlist();
  mdata->taken = 0;
  mdata->queued = 0;
  if(attr != 0) {
    if(_get_mutexattr_data(attr) == PTHREAD_MUTEX_RECURSIVE)
      mdata->count = 0;
    else
      mdata->count = -1;
  }
  else
    mdata->count = -1;
}

static mutex_data_t *_get_mutex_data(pthread_mutex_t *mutex) {
  mutex_data_t *mdata = *((mutex_data_t**)mutex);

  if (mdata == STATIC_MUTEX_VALUE) {
    _mutex_init(mutex, 0);

    mdata = *((mutex_data_t**)mutex);
  }

  return mdata;
}

int pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *attr) {
  _mutex_init(mutex, attr);

  return 0;
}

int pthread_mutex_destroy(pthread_mutex_t *mutex) {
  mutex_data_t *mdata = _get_mutex_data(mutex);

  free(mdata);

  return 0;
}

static int _atomic_mutex_lock(mutex_data_t *mdata, char try) {
  if (mdata->taken && mdata->count >= 0 && mdata->owner == pthread_self()) {
    mdata->count++;
    return 0;
  }
  else if (mdata->queued > 0 || mdata->taken) {
    if (try) {
      errno = EBUSY;
      return -1;
    } else {
      mdata->queued++;
      __thread_sleep(mdata->wlist);
      mdata->queued--;
    }
  }
  mdata->taken = 1;
  mdata->owner = pthread_self();
  if(mdata->count != -1)
    mdata->count = 1;

  return 0;
}

int pthread_mutex_lock(pthread_mutex_t *mutex) {
  //__thread_preempt(0);

  mutex_data_t *mdata = _get_mutex_data(mutex);

  int res = _atomic_mutex_lock(mdata, 0);

  if (res == 0)
    __thread_preempt(0);

  return res;
}

int pthread_mutex_trylock(pthread_mutex_t *mutex) {
  //__thread_preempt(0);

  mutex_data_t *mdata = _get_mutex_data(mutex);

  int res = _atomic_mutex_lock(mdata, 1);

  if (res == 0)
    __thread_preempt(0);

  return res;
}

static int _atomic_mutex_unlock(mutex_data_t *mdata) {
  if (mdata->taken && mdata->count > 0 && mdata->owner == pthread_self()) {
    mdata->count--;
    if(mdata->count != 0)
      return 0;
  }
  else if (!mdata->taken || mdata->owner != pthread_self()) {
    errno = EPERM;
    return -1;
  }

  mdata->taken = 0;

  if (mdata->queued > 0)
    __thread_notify_one(mdata->wlist);

  return 0;
}

int pthread_mutex_unlock(pthread_mutex_t *mutex) {
  //__thread_preempt(0);

  mutex_data_t *mdata = _get_mutex_data(mutex);

  int res = _atomic_mutex_unlock(mdata);

  if (res == 0)
    __thread_preempt(0);

  return res;
}

////////////////////////////////////////////////////////////////////////////////
// POSIX Condition Variables
////////////////////////////////////////////////////////////////////////////////

static void _cond_init(pthread_cond_t *cond, const pthread_condattr_t *attr) {
  condvar_data_t *cdata = (condvar_data_t*)malloc(sizeof(condvar_data_t));
  memset(cdata, 0, sizeof(condvar_data_t));

  *((condvar_data_t**)cond) = cdata;

  cdata->wlist = klee_get_wlist();
}

static condvar_data_t *_get_condvar_data(pthread_cond_t *cond) {
  condvar_data_t *cdata = *((condvar_data_t**)cond);

  if (cdata == STATIC_CVAR_VALUE) {
    _cond_init(cond, 0);

    cdata = *((condvar_data_t**)cond);
  }

  return cdata;
}

int pthread_cond_init(pthread_cond_t *cond, const pthread_condattr_t *attr) {
  _cond_init(cond, attr);

  return 0;
}

int pthread_cond_destroy(pthread_cond_t *cond) {
  condvar_data_t *cdata = _get_condvar_data(cond);

  free(cdata);

  return 0;
}

int pthread_cond_timedwait(pthread_cond_t *cond, pthread_mutex_t *mutex,
    const struct timespec *abstime) {
  assert(0 && "not implemented");
  return -1;
}

static int _atomic_cond_wait(condvar_data_t *cdata, mutex_data_t *mdata) {
  if (cdata->queued > 0) {
    if (cdata->mutex != mdata) {
      errno = EINVAL;
      return -1;
    }
  } else {
    cdata->mutex = mdata;
  }

  if (_atomic_mutex_unlock(mdata) != 0) {
    errno = EPERM;
    return -1;
  }

  cdata->queued++;
  __thread_sleep(cdata->wlist);
  cdata->queued--;

  if (_atomic_mutex_lock(mdata, 0) != 0) {
    errno = EPERM;
    return -1;
  }

  return 0;
}

int pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex) {
  //__thread_preempt(0);

  condvar_data_t *cdata = _get_condvar_data(cond);
  mutex_data_t *mdata = _get_mutex_data(mutex);

  int res = _atomic_cond_wait(cdata, mdata);

  if (res == 0)
    __thread_preempt(0);

  return res;
}

static int _atomic_cond_notify(condvar_data_t *cdata, char all) {
  if (cdata->queued > 0) {
    if (all)
      __thread_notify_all(cdata->wlist);
    else
      __thread_notify_one(cdata->wlist);
  }

  return 0;
}

int pthread_cond_broadcast(pthread_cond_t *cond) {
  //__thread_preempt(0);

  condvar_data_t *cdata = _get_condvar_data(cond);

  int res = _atomic_cond_notify(cdata, 1);

  if (res == 0)
    __thread_preempt(0);

  return res;
}

int pthread_cond_signal(pthread_cond_t *cond) {
  //__thread_preempt(0);

  condvar_data_t *cdata = _get_condvar_data(cond);

  int res = _atomic_cond_notify(cdata, 0);

  if (res == 0)
    __thread_preempt(0);

  return res;
}

////////////////////////////////////////////////////////////////////////////////
// POSIX Barriers
////////////////////////////////////////////////////////////////////////////////

static void _barrier_init(pthread_barrier_t *barrier, const pthread_barrierattr_t *attr, unsigned int count) {
  barrier_data_t *bdata = (barrier_data_t*)malloc(sizeof(barrier_data_t));
  memset(bdata, 0, sizeof(barrier_data_t));

  *((barrier_data_t**)barrier) = bdata;

  bdata->wlist = klee_get_wlist();
  bdata->curr_event = 0;
  bdata->init_count = count;
  bdata->left = count;
}

static barrier_data_t *_get_barrier_data(pthread_barrier_t *barrier) {
  barrier_data_t *bdata = *((barrier_data_t**)barrier);

  if (bdata == STATIC_BARRIER_VALUE) {
    _barrier_init(barrier, 0, 0);

    bdata = *((barrier_data_t**)barrier);
  }

  return bdata;
}

int pthread_barrier_init(pthread_barrier_t *barrier, const pthread_barrierattr_t *attr, unsigned int count) {
  _barrier_init(barrier, attr, count);

  return 0;
}

int pthread_barrier_destroy(pthread_barrier_t *barrier) {
  barrier_data_t *bdata = _get_barrier_data(barrier);

  free(bdata);

  return 0;
}

int pthread_barrier_wait(pthread_barrier_t *barrier) {
  barrier_data_t *bdata = *((barrier_data_t**)barrier);
  int result = 0;

  if (bdata == STATIC_BARRIER_VALUE) {
      errno = EINVAL;
      return -1;
  }

  --bdata->left;

  if (bdata->left == 0) {
    ++bdata->curr_event;
    bdata->left = bdata->init_count;

    __thread_notify_all(bdata->wlist);

    result = PTHREAD_BARRIER_SERIAL_THREAD;

    __thread_preempt(0);
  }
  else {
    __thread_sleep(bdata->wlist);
  }

  return result;
}

////////////////////////////////////////////////////////////////////////////////
// POSIX Read Write Locks
////////////////////////////////////////////////////////////////////////////////

static void _rwlock_init(pthread_rwlock_t *rwlock, const pthread_rwlockattr_t *attr) {
  rwlock_data_t *rwdata = (rwlock_data_t*)malloc(sizeof(rwlock_data_t));
  memset(rwdata, 0, sizeof(rwlock_data_t));

  *((rwlock_data_t**)rwlock) = rwdata;

  rwdata->wlist_readers = klee_get_wlist();
  rwdata->wlist_writers = klee_get_wlist();
  rwdata->nr_readers = 0;
  rwdata->nr_readers_queued = 0;
  rwdata->nr_writers_queued = 0;
  rwdata->writer_taken = 0;
}

static rwlock_data_t *_get_rwlock_data(pthread_rwlock_t *rwlock) {
  rwlock_data_t *rwdata = *((rwlock_data_t**)rwlock);

  if (rwdata == STATIC_RWLOCK_VALUE) {
    _rwlock_init(rwlock, 0);

    rwdata = *((rwlock_data_t**)rwlock);
  }

  return rwdata;
}

int pthread_rwlock_init(pthread_rwlock_t *rwlock, const pthread_rwlockattr_t *attr) {
  _rwlock_init(rwlock, attr);

  return 0;
}

int pthread_rwlock_destroy(pthread_rwlock_t *rwlock) {
  rwlock_data_t *rwdata = _get_rwlock_data(rwlock);

  free(rwdata);

  return 0;
}

static int _atomic_rwlock_rdlock(rwlock_data_t *rwdata, char try) {
  if (rwdata == STATIC_RWLOCK_VALUE) {
    errno = EINVAL;
    return -1;
  }

  if (!rwdata->writer_taken && rwdata->nr_writers_queued == 0) {
    if (++rwdata->nr_readers == 0) {
      --rwdata->nr_readers;
      errno = EAGAIN;
      return -1;
    }

    return 0;
  }

  if (try != 0) {
    errno = EBUSY;
    return -1;
  }
  else {
    if (++rwdata->nr_readers_queued == 0) {
      --rwdata->nr_readers_queued;
      errno = EAGAIN;
      return -1;
    }

    __thread_sleep(rwdata->wlist_readers);
    ++rwdata->nr_readers;
    --rwdata->nr_readers_queued;
  }

  return 0;
}

int pthread_rwlock_rdlock(pthread_rwlock_t *rwlock) {
  //__thread_preempt(0);

  rwlock_data_t *rwdata = *((rwlock_data_t**)rwlock);

  int res = _atomic_rwlock_rdlock(rwdata, 0);

  if (res == 0)
    __thread_preempt(0);

  return res;
}

int pthread_rwlock_tryrdlock(pthread_rwlock_t *rwlock) {
  //__thread_preempt(0);

  rwlock_data_t *rwdata = *((rwlock_data_t**)rwlock);

  int res = _atomic_rwlock_rdlock(rwdata, 1);

  if (res == 0)
    __thread_preempt(0);

  return res;
}

static int _atomic_rwlock_wrlock(rwlock_data_t *rwdata, char try) {
  if (rwdata == STATIC_RWLOCK_VALUE) {
    errno = EINVAL;
    return -1;
  }

  if (!rwdata->writer_taken && rwdata->nr_readers == 0) {
    rwdata->writer = pthread_self();
    rwdata->writer_taken = 1;
    return 0;
  }

  if (try != 0) {
    errno = EBUSY;
    return -1;
  }
  else {
    if (++rwdata->nr_writers_queued == 0) {
      --rwdata->nr_writers_queued;
      errno = EAGAIN;
      return -1;
    }

    __thread_sleep(rwdata->wlist_writers);
    rwdata->writer = pthread_self();
    rwdata->writer_taken = 1;
    --rwdata->nr_writers_queued;
  }

  return 0;
}

int pthread_rwlock_wrlock(pthread_rwlock_t *rwlock) {
  //__thread_preempt(0);

  rwlock_data_t *rwdata = *((rwlock_data_t**)rwlock);

  int res = _atomic_rwlock_wrlock(rwdata, 0);

  if (res == 0)
    __thread_preempt(0);

  return res;
}

int pthread_rwlock_trywrlock(pthread_rwlock_t *rwlock) {
  //__thread_preempt(0);

  rwlock_data_t *rwdata = *((rwlock_data_t**)rwlock);

  int res = _atomic_rwlock_wrlock(rwdata, 1);

  if (res == 0)
    __thread_preempt(0);

  return res;
}

static int _atomic_rwlock_unlock(rwlock_data_t *rwdata) {
  if (rwdata == STATIC_RWLOCK_VALUE) {
    errno = EINVAL;
    return -1;
  }

  if (rwdata->writer_taken && rwdata->writer == pthread_self())
    rwdata->writer_taken = 0;
  else if (rwdata->writer_taken && rwdata->writer != pthread_self()) {
    errno = EPERM;
    return -1;
  } else {
    if (rwdata->nr_readers > 0)
      --rwdata->nr_readers;
  }

  if (rwdata->nr_readers == 0 && rwdata->nr_writers_queued)
    __thread_notify_one(rwdata->wlist_writers);
  else {
    if (rwdata->nr_readers_queued > 0)
      __thread_notify_all(rwdata->wlist_readers);
  }

  return 0;
}

int pthread_rwlock_unlock(pthread_rwlock_t *rwlock) {
  //__thread_preempt(0);

  rwlock_data_t *rwdata = *((rwlock_data_t**)rwlock);

  int res = _atomic_rwlock_unlock(rwdata);

  if (res == 0)
    __thread_preempt(0);

  return res;
}
