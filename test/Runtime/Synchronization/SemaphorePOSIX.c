// RUN: %llvmgcc %s -emit-llvm -O0 -c -o %t1.bc
// RUN: %klee --no-output --exit-on-error --posix-runtime --libc=uclibc %t1.bc --sym-args 0 3 1

#include <assert.h>
#include <pthread.h>
#include <semaphore.h>
 
#define ITERATIONS 10000
sem_t sem_writer;
sem_t sem_reader;
 
void *writerCode(void *p)
{
  int i;
  int err;
  for(i=0; i<ITERATIONS; ++i) {
    err = sem_wait(&sem_writer);
    assert(err == 0 && "Wait on writer semaphore");
    err = sem_post(&sem_reader);
    assert(err == 0 && "Post to reader semaphore");
  }
  return NULL;
}
 
void *readerCode(void *p)
{
  int i;
  int err;
  for(i=0; i<ITERATIONS; ++i) {
    err = sem_wait(&sem_reader);
    assert(err == 0 && "Wait on reader semaphore");
    err = sem_post(&sem_writer);
    assert(err == 0 && "Post to writer semaphore");
  }
  return NULL;
}

int main(int argc, char **argv) {
  int i;
  int err;
  int num_threads = 2*argc;
  pthread_t thread[num_threads];

  err = sem_init(&sem_writer, 0, 2);
  assert(err == 0 && "Writer semaphore init");
  err = sem_init(&sem_reader, 0, 0);
  assert(err == 0 && "Reader semaphore init");

  for (i=0; i<num_threads; ++i) {
    if(i%2 == 0)
      err = pthread_create(&thread[i], NULL, writerCode, NULL);
    else
      err = pthread_create(&thread[i], NULL, readerCode, NULL);
    assert(err == 0 && "Thread create");
  }
 
  for (i=0; i <num_threads; ++i) {
   pthread_join(thread[i], NULL);
  assert(err == 0 && "Thread join");
  }

  err = sem_destroy(&sem_writer);
  assert(err == 0 && "Writer semaphore destroy");
  err = sem_destroy(&sem_reader);
  assert(err == 0 && "Reader semaphore destroy");
  return 0;
}
