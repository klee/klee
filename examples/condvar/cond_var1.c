#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


#define NUM_THREADS  2



volatile int count = 0;
int thread_ids[2] = { 0, 1};
pthread_mutex_t count_mutex;
pthread_cond_t count_threshold_cv;

void *
inc_count (void* arg)
{
  pthread_mutex_lock (&count_mutex);
  printf("signaling\n");
  pthread_cond_signal (&count_threshold_cv);
  printf("signaled\n");
  
  pthread_mutex_unlock (&count_mutex);
  return 0;
}

void *
watch_count (void *idp)
{
  pthread_mutex_lock (&count_mutex);
  
  printf("sleeping\n");
  pthread_cond_wait (&count_threshold_cv, &count_mutex);
  printf ("woke up\n");
  
  pthread_mutex_unlock (&count_mutex);
  
  return 0;
}

int
main (int argc, char *argv[])
{
  int i;
  pthread_t threads[2];
  

  /* Initialize mutex and condition variable objects */
  pthread_mutex_init (&count_mutex, NULL);
  
  pthread_cond_init (&count_threshold_cv, NULL);

  
  pthread_create (&threads[1], 0, watch_count, 0);
  pthread_create (&threads[0], 0, inc_count, 0);
  

  /* Wait for all threads to complete */
  for (i = 0; i < NUM_THREADS; i++)
    {
      pthread_join (threads[i], NULL);
    }
  printf("main exited\n");
  
    
  pthread_mutex_destroy (&count_mutex);
  
  return 0;
}
