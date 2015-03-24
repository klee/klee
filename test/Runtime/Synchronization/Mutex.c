// RUN: %llvmgcc %s -emit-llvm -O0 -c -o %t1.bc
// RUN: %klee --no-output --exit-on-error --posix-runtime --libc=uclibc %t1.bc --sym-arg 1 --sym-arg 1

#include <assert.h>
#include <pthread.h>
 
#define ITERATIONS 10000
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
int sharedData=0;
 
void *threadCode(void *p)
{
  int i;
  int err;
  for(i=0; i<ITERATIONS; ++i) {
    err = pthread_mutex_lock(&mutex);
    assert(err == 0 && "Mutex lock");
    ++sharedData;
    err = pthread_mutex_unlock(&mutex);
    assert(err == 0 && "Mutex unlock");
  }
  return NULL;
}
 
int main(int argc, char **argv) {
  int i;
  int err;
  int num_threads = argc;
  pthread_t thread[num_threads];

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
  return 0;
}

