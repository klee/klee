#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <klee/klee.h>

pthread_barrier_t barrier;

void * work0 (void *arg)
{
  printf("work0: wait at barrier\n");
  pthread_barrier_wait (&barrier);
  printf("work0: pass the barrier\n");
  return 0;
}


void * work1 (void *arg)
{
  printf("work1: wait at barrier\n");
  pthread_barrier_wait (&barrier);
  printf("work1: pass the barrier\n");
  return 0;
}



int main (int argc, char *argv[])
{
  pthread_t t0, t1;

  int rc;
  int nr;
  
  srand(time(0));
  
  if(argc != 1)
    return 1;

  klee_make_symbolic(&nr, sizeof(nr), "input");
  
  pthread_barrier_init (&barrier, 0, nr);
  
  printf("t0 = %u \n", (unsigned int) t0);

  rc = pthread_create (&t0, 0, work0, 0);
  rc = pthread_create (&t1, 0, work1, 0);

  pthread_join(t0, 0);
  
  pthread_join(t1, 0);

  pthread_barrier_destroy (&barrier);    
  
  printf ("STOP\n");
  
  return 0;
}

