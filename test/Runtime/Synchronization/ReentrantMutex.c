// RUN: %llvmgcc %s -emit-llvm -O0 -c -o %t1.bc
// RUN: %klee --no-output --exit-on-error --posix-runtime --libc=uclibc %t1.bc --sym-args 0 3 1

#include <assert.h>
#include <pthread.h>
 
#define ITERATIONS 10000
pthread_mutex_t mutex;
int sharedData=0;
 
void *threadCode(void *p)
{
  int i;
  int err;
  for(i=0; i<ITERATIONS; ++i) {
    err = pthread_mutex_lock(&mutex);
    assert(err == 0 && "First lock");
    err = pthread_mutex_lock(&mutex);
    assert(err == 0 && "Second lock");
    ++sharedData;
    err = pthread_mutex_unlock(&mutex);
    assert(err == 0 && "First unlock");
    err = pthread_mutex_unlock(&mutex);
    assert(err == 0 && "Second unlock");
  }
  return NULL;
}
 
int main(int argc, char **argv) {
  int i;
  int err;
  int num_threads = argc;
  pthread_t thread[num_threads];

  pthread_mutexattr_t attr;
  err = pthread_mutexattr_init(&attr);
  assert(err == 0 && "Attribute init");
  err = pthread_mutexattr_settype(&attr,PTHREAD_MUTEX_RECURSIVE);
  assert(err == 0 && "Settype");
  err = pthread_mutex_init(&mutex, &attr);
  assert(err == 0 && "Mutex init");

  for (i=0; i<num_threads; ++i) {
    err = pthread_create(&thread[i], NULL, threadCode, NULL);
    assert(err == 0 && "Thread create");
  }
 
  for (i=0; i <num_threads; ++i) {
   pthread_join(thread[i], NULL);
  assert(err == 0 && "Thread join");
  }
 
  assert(sharedData == num_threads*ITERATIONS);

  err = pthread_mutex_destroy(&mutex);
  assert(err == 0 && "Mutex destroy");
  err = pthread_mutexattr_destroy(&attr);
  assert(err == 0 && "Attribute destroy");
  return 0;
}
