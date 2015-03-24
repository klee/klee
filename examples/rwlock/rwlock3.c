/*
 * This code can deadlock 
 */


#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <klee/klee.h>

pthread_rwlock_t cs0, cs1;

int globalX = 0;
int globalY = 0;

void * work0 (void *arg)
{
  do {
    pthread_rwlock_trywrlock (&cs0);
  }while(errno == EBUSY);
  printf("work0: got write lock 0\n");
  globalX++;
  do {
    pthread_rwlock_trywrlock (&cs0);
  }while(errno == EBUSY);
  printf("work0: got write lock 0\n");
  globalY++;
  pthread_rwlock_unlock (&cs0);
  printf("work0: released write lock 0\n");
  pthread_rwlock_unlock (&cs0);
  printf("work0: released write lock 0\n");
  return 0;
}


void * work1 (void *arg)
{
  int r;

  klee_make_symbolic(&r, sizeof(r), "input");
  
  if(r)
    {
      pthread_rwlock_wrlock (&cs0);
      printf("work1: got write lock 0\n");
      globalX++;
      pthread_rwlock_wrlock (&cs1);
      printf("work1: got write lock 1\n");
      globalY++;
    }
  else
    {
      pthread_rwlock_wrlock (&cs1);
      printf("work1: got write lock 1\n");
      globalX++;
      pthread_rwlock_wrlock (&cs0);
      printf("work1: got write lock 0\n");
      globalY++;
    }


  if(r)
    {
      pthread_rwlock_unlock (&cs1);
      printf("work1: released write lock 1\n");
      pthread_rwlock_unlock (&cs0);
      printf("work1: released write lock 0\n");
    }
  else
    {
      pthread_rwlock_unlock (&cs0);
      printf("work1: released write lock 0\n");
      pthread_rwlock_unlock (&cs1);
      printf("work1: released write lock 1\n");
    }
  return 0;
}

int main (int argc, char *argv[])
{
  pthread_t t0, t1;

  int rc;
  
  srand(time(0));
  
  if(argc != 1)
    return 1;
  
  pthread_rwlock_init (&cs0, 0);
  pthread_rwlock_init (&cs1, 0);
  
  printf ("START\n");
  
  printf("t0 = %u \n", (unsigned int) t0);

  rc = pthread_create (&t0, 0, work0, 0);
  rc = pthread_create (&t1, 0, work1, 0);

  pthread_join(t0, 0);
  pthread_join(t1, 0);

  printf ("TOTAL = (%d,%d)\n", globalX, globalY);
  printf ("STOP\n");
  
  return 0;
}

