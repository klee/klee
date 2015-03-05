/*
 * This code can deadlock 
 */


#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>

pthread_mutex_t cs0, cs1;

int globalX = 0;
int globalY = 0;


void
print_safe()
{
  printf("-------- safe\n");
}

void
print_deadlock()
{
  printf("------------- could deadlock\n");
}

void * work0 (void *arg)
{
  pthread_mutex_lock (&cs0);
  printf("work0: got lock 0\n");
  globalX++;
  pthread_mutex_lock (&cs1);
  printf("work0: got lock 1\n");
  globalY++;
  pthread_mutex_unlock (&cs1);
  printf("work0: released lock 1\n");
  pthread_mutex_unlock (&cs0);
  printf("work0: released lock 0\n");
  return 0;
}


void * work1 (void *arg)
{
  int r = 0;
  
  if(r)
    {
      pthread_mutex_lock (&cs0);
      printf("work1: got lock 0\n");
      globalX++;
      pthread_mutex_lock (&cs1);
      printf("work1: got lock 1\n");
      print_safe();
      globalY++;
    }
  else
    {
      pthread_mutex_lock (&cs1);
      printf("work1: got lock 1\n");
      globalX++;
      pthread_mutex_lock (&cs0);
      printf("work1: got lock 0\n");
      print_deadlock();
      globalY++;
    }


  if(r)
    {
      pthread_mutex_unlock (&cs1);
      print_safe();
      pthread_mutex_unlock (&cs0);
    }
  else
    {
      pthread_mutex_unlock (&cs0);
      printf("work1: released lock 0\n");
      print_deadlock();
      pthread_mutex_unlock (&cs1);
      printf("work1: released lock 1\n");
    }
  return 0;
}



int main (int argc, char *argv[])
{
  //pthread_t h[2];

  pthread_t t0, t1;

  int rc;
  
  srand(time(0));
  
  if(argc != 1)
    return 1;
  
  pthread_mutex_init (&cs0, 0);
  pthread_mutex_init (&cs1, 0);
  
  printf ("START\n");
  
  printf("t0 = %u \n", (unsigned int) t0);

  rc = pthread_create (&t0, 0, work0, 0);
  rc = pthread_create (&t1, 0, work1, 0);


  //rc = pthread_create (&h[0], 0, work0, 0);
  //rc = pthread_create (&h[1], 0, work1, 0);
    
  //rc = pthread_join (h[0], 0);
  //rc = pthread_join (h[1], 0);

  pthread_join(t0, 0);
  pthread_join(t1, 0);

    
  //pthread_mutex_destroy (&cs0);
  //pthread_mutex_destroy (&cs1);
  
  printf ("TOTAL = (%d,%d)\n", globalX, globalY);
  printf ("STOP\n");
  
  return 0;
}

