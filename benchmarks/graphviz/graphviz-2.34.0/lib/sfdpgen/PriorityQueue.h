#ifndef PRIORITY_QUEUE_H
#define PRIORITY_QUEUE_H
#include "LinkedList.h"
struct PriorityQueue_struct {
  /* a simple priority queue structure: entries are all integers, gains are all integers in [0, gainmax], total n elements */
  int count;/* how many entries are in?*/

  /* max index value of an entry */
  int n;

  /* only ngain buckets are allowed */
  int ngain;

  /* current highest gain */
  int gain_max;

  /* the ngain buckets. Each bucket i holds all entries with gain = i.*/
  DoubleLinkedList *buckets;

  /* a mapping which maps an entry's index to an element in a DoubleLinkedList */
  DoubleLinkedList *where;

  /* the gain of entry i is gain[i] */
  int *gain;
};

typedef struct PriorityQueue_struct *PriorityQueue;


PriorityQueue PriorityQueue_new(int n, int ngain);

void PriorityQueue_delete(PriorityQueue q);

/* if entry i is already in the list, then an update of gain is carried out*/
PriorityQueue PriorityQueue_push(PriorityQueue q, int i, int gain);

int PriorityQueue_pop(PriorityQueue q, int *i, int *gain);/* return 0 if nmothing left, 1 otherwise */

int PriorityQueue_remove(PriorityQueue q, int i);/* return 0 if error */
int PriorityQueue_get_gain(PriorityQueue q, int i);

#endif
