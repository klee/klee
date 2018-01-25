/* $Id$ $Revision$ */
/* vim:set shiftwidth=4 ts=8: */

/*************************************************************************
 * Copyright (c) 2011 AT&T Intellectual Property 
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors: See CVS logs. Details at http://www.graphviz.org/
 *************************************************************************/

#include "LinkedList.h"
#include "PriorityQueue.h"
#include "memory.h"
#include "logic.h"
#include "assert.h"
#include "arith.h"

#define MALLOC gmalloc
#define REALLOC grealloc
#define FREE free
#define MEMCPY memcpy


PriorityQueue PriorityQueue_new(int n, int ngain){
  PriorityQueue q;
  int i;
  q = N_GNEW(1,struct PriorityQueue_struct);
  q->count = 0;
  q->n = n;
  q->ngain = ngain;
  q->gain_max = -1;/* no entries yet */
  q->buckets = N_GNEW((ngain+1),DoubleLinkedList);
  for (i = 0; i < ngain+1; i++) (q->buckets)[i] = NULL;

  q->where = N_GNEW((n+1),DoubleLinkedList);
  for (i = 0; i < n+1; i++) (q->where)[i] = NULL;

  q->gain = N_GNEW((n+1),int);
  for (i = 0; i < n+1; i++) (q->gain)[i] = -999;
  return q;

}

void PriorityQueue_delete(PriorityQueue q){
  int i;

  if (q){
    if (q->buckets){
      for (i = 0; i < q->ngain+1; i++) DoubleLinkedList_delete((q->buckets)[i], free);
      FREE(q->buckets);
    }

    if (q->where){
      FREE(q->where);
    }

    FREE(q->gain);
    FREE(q);
  }
}

PriorityQueue PriorityQueue_push(PriorityQueue q, int i, int gain){
  DoubleLinkedList l;
  int *data, gainold;

  assert(q);
  assert(gain <= q->ngain);


  if (!(q->where)[i]){
    /* this entry is no in the queue yet, so this is a new addition */

    (q->count)++;
    if (gain > q->gain_max) q->gain_max = gain;
    q->gain[i] = gain;

    data = N_GNEW(1,int);
    data[0] = i;
    if ((l = (q->buckets)[gain])){
      (q->buckets)[gain] = (q->where)[i] = DoubleLinkedList_prepend(l, data);
    } else {
      (q->buckets)[gain] = (q->where)[i] = DoubleLinkedList_new(data);
    }

  } else {
    /* update gain for an exisiting entry */
    l = q->where[i];
    gainold = q->gain[i];
    q->where[i] = NULL; 
    (q->count)--;
    DoubleLinkedList_delete_element(l, free, &((q->buckets)[gainold]));
    return PriorityQueue_push(q, i, gain);
  }
  return q;
}

int PriorityQueue_pop(PriorityQueue q, int *i, int *gain){
  int gain_max;
  DoubleLinkedList l;
  int *data;

  if (!q || q->count <= 0) return 0;
  *gain = gain_max = q->gain_max;
  (q->count)--;
  l = (q->buckets)[gain_max];
  data = (int*) DoubleLinkedList_get_data(l);
  *i = data[0];

  DoubleLinkedList_delete_element(l, free, &((q->buckets)[gain_max]));
  if (!(q->buckets)[gain_max]){/* the bin that contain the best gain is empty now after poping */
    while (gain_max >= 0 && !(q->buckets)[gain_max]) gain_max--;
    q->gain_max = gain_max;
  }
  q->where[*i] = NULL;
  q->gain[*i] = -999;
  return 1;
}




int PriorityQueue_get_gain(PriorityQueue q, int i){
  return q->gain[i];
}

int PriorityQueue_remove(PriorityQueue q, int i){
  /* remove an entry from the queue. If error, return 0. */
  int gain, gain_max;
  DoubleLinkedList l;

  if (!q || q->count <= 0) return 0;
  gain = q->gain[i];
  (q->count)--;
  l = (q->where)[i];

  DoubleLinkedList_delete_element(l, free, &((q->buckets)[gain]));

  if (gain == (gain_max = q->gain_max) && !(q->buckets)[gain_max]){/* the bin that contain the best gain is empty now after poping */
    while (gain_max >= 0 && !(q->buckets)[gain_max]) gain_max--;
    q->gain_max = gain_max;
  }
  q->where[i] = NULL;
  q->gain[i] = -999;
  return 1;
}

/*

main(){
  int i, gain;

    
    PriorityQueue q;
    q = PriorityQueue_new(10,100);
    PriorityQueue_push(q, 3, 1);
    PriorityQueue_push(q, 2, 2);
    PriorityQueue_push(q, 4, 2);
    PriorityQueue_push(q, 5, 2);
    PriorityQueue_push(q, 1, 100);
    PriorityQueue_push(q, 2,  1);
    while (PriorityQueue_pop(q, &i, &gain)){
      printf("i = %d gain = %d\n", i, gain);
    }
    
    printf("=========\n");
    PriorityQueue_push(q, 3, 1);
    PriorityQueue_push(q, 2, 2);
    PriorityQueue_push(q, 4, 2);
    PriorityQueue_push(q, 5, 2);
    PriorityQueue_push(q, 1, 100);
    PriorityQueue_push(q, 2,  1);
    PriorityQueue_push(q, 2,  100);
    while (PriorityQueue_pop(q, &i, &gain)){
      printf("i = %d gain = %d\n", i, gain);
    }


    printf("====after removing 3 and 2 =====\n");
    PriorityQueue_push(q, 3, 1);
    PriorityQueue_push(q, 2, 2);
    PriorityQueue_push(q, 4, 2);
    PriorityQueue_push(q, 5, 2);
    PriorityQueue_push(q, 1, 100);
    PriorityQueue_push(q, 2,  1);
    PriorityQueue_push(q, 2,  100);
    PriorityQueue_remove(q, 3);
    PriorityQueue_remove(q, 2);
    while (PriorityQueue_pop(q, &i, &gain)){
      printf("i = %d gain = %d\n", i, gain);
    }
    PriorityQueue_delete(q);

}

*/
