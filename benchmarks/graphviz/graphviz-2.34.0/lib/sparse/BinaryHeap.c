/* $Id$Revision: */
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

#include "BinaryHeap.h"

BinaryHeap BinaryHeap_new(int (*cmp)(void*item1, void*item2)){
  BinaryHeap h;
  int max_len = 1<<8, i;

  h = MALLOC(sizeof(struct BinaryHeap_struct));
  h->max_len = max_len;
  h->len = 0;
  h->heap = MALLOC(sizeof(void*)*max_len);
  h->id_to_pos = MALLOC(sizeof(int)*max_len);
  for (i = 0; i < max_len; i++) (h->id_to_pos)[i] = -1;

  h->pos_to_id = MALLOC(sizeof(int)*max_len);
  h->id_stack = IntStack_new();
  h->cmp = cmp;
  return h;
}


void BinaryHeap_delete(BinaryHeap h, void (*del)(void* item)){
  int i;
  if (!h) return;
  FREE(h->id_to_pos);
  FREE(h->pos_to_id);
  IntStack_delete(h->id_stack);
  if (del) for (i = 0; i < h->len; i++) del((h->heap)[i]);
  FREE(h->heap);
  FREE(h);
}

static BinaryHeap BinaryHeap_realloc(BinaryHeap h){
  int max_len0 = h->max_len, max_len = h->max_len, i;
  max_len = max_len + MAX(0.2*max_len, 10);
  h->max_len = max_len;

  h->heap = REALLOC(h->heap, sizeof(void*)*max_len);
  if (!(h->heap)) return NULL;

  h->id_to_pos = REALLOC(h->id_to_pos, sizeof(int)*max_len);
  if (!(h->id_to_pos)) return NULL;

  h->pos_to_id = REALLOC(h->pos_to_id, sizeof(int)*max_len);
  if (!(h->pos_to_id)) return NULL;

  for (i = max_len0; i < max_len; i++) (h->id_to_pos)[i] = -1;
  return h;

}

#define ParentPos(pos) (pos - 1)/2
#define ChildrenPos1(pos) (2*(pos)+1)
#define ChildrenPos2(pos) (2*(pos)+2)

static void swap(BinaryHeap h, int parentPos, int nodePos){
  int parentID, nodeID;
  void *tmp;
  void **heap = h->heap;
  int *id_to_pos = h->id_to_pos, *pos_to_id = h->pos_to_id;

  assert(parentPos < h->len);
  assert(nodePos < h->len);

  parentID = pos_to_id[parentPos];
  nodeID = pos_to_id[nodePos];

  tmp = heap[parentPos];
  heap[parentPos] = heap[nodePos];
  heap[nodePos] = tmp;

  pos_to_id[parentPos] = nodeID;
  id_to_pos[nodeID] = parentPos;

  pos_to_id[nodePos] = parentID;
  id_to_pos[parentID] = nodePos;

}

static int siftUp(BinaryHeap h, int nodePos){
  int parentPos;

  void **heap = h->heap;


  if (nodePos != 0) {
    parentPos = ParentPos(nodePos);

    if ((h->cmp)(heap[parentPos], heap[nodePos]) == 1) {/* if smaller than parent, swap */
      swap(h, parentPos, nodePos);
      nodePos = siftUp(h, parentPos);
    }
  }
  return nodePos;
}

static int siftDown(BinaryHeap h, int nodePos){
  int childPos, childPos1, childPos2;

  void **heap = h->heap;


  childPos1 = ChildrenPos1(nodePos);
  childPos2 = ChildrenPos2(nodePos);
  if (childPos1 > h->len - 1) return nodePos;/* no child */
  if (childPos1 == h->len - 1) {
    childPos = childPos1;/* one child */
  } else {/* two child */
    if ((h->cmp)(heap[childPos1], heap[childPos2]) == 1){ /* pick the smaller of the two child */
      childPos = childPos2;
    } else {
      childPos = childPos1;
    }
  }

  if ((h->cmp)(heap[nodePos], heap[childPos]) == 1) {
    /* if larger than child, swap */
    swap(h, nodePos, childPos);
    nodePos = siftDown(h, childPos);
  } 

  return nodePos;
}

int BinaryHeap_insert(BinaryHeap h, void *item){
  int len = h->len, id = len, flag, pos;

  /* insert an item, and return its ID. This ID can be used later to extract the item */
  if (len > h->max_len - 1) {
    if (BinaryHeap_realloc(h) == NULL) return BinaryHeap_error_malloc;
  }
  
  /* check if we have IDs in the stack to reuse. If no, then assign the last pos as the ID */
  id = IntStack_pop(h->id_stack, &flag);
  if (flag) id = len;

  h->heap[len] = item;
  h->id_to_pos[id] = len;
  h->pos_to_id[len] = id;

  (h->len)++;

  pos = siftUp(h, len);
  assert(h->id_to_pos[id] == pos);
  assert(h->pos_to_id[pos] == id);


  return id;
}


void* BinaryHeap_get_min(BinaryHeap h){
  /* return the min item */
  return h->heap[0];
}

void* BinaryHeap_extract_min(BinaryHeap h){
  /* return and remove the min item */
  if (h->len == 0) return NULL;
  return BinaryHeap_extract_item(h, (h->pos_to_id)[0]);
}

void* BinaryHeap_extract_item(BinaryHeap h, int id){
  /* extract an item with ID out and delete it */
  void *item;
  int *id_to_pos = h->id_to_pos;
  int pos;

  if (id >= h->max_len) return NULL;

  pos = id_to_pos[id];

  if (pos < 0) return NULL;

  assert(pos < h->len);

  item = (h->heap)[pos];

  IntStack_push(h->id_stack, id);

  if (pos < h->len - 1){/* move the last item to occupy the position of extracted item */
    swap(h, pos, h->len - 1);
    (h->len)--;
    pos = siftUp(h, pos);
    pos = siftDown(h, pos);
  } else {
    (h->len)--;
  }
 
  (h->id_to_pos)[id] = -1;

  return item;

}

int BinaryHeap_reset(BinaryHeap h, int id, void *item){
  /* reset value of an item with specified id */
  int pos;

  if (id >= h->max_len) return -1;
  pos = h->id_to_pos[id];
  if (pos < 0) return -1;

  h->heap[pos] = item;

  pos = siftUp(h, pos);

  pos = siftDown(h, pos);

  return pos;

}
void* BinaryHeap_get_item(BinaryHeap h, int id){
  /* get an item with ID out, without deleting */
  int pos;

  if (id >= h->max_len) return NULL;
  
  pos = h->id_to_pos[id];
  
  if (pos < 0) return NULL;
  return (h->heap)[pos];
}

void BinaryHeap_sanity_check(BinaryHeap h){
  int i, key_spare, parentPos, *id_to_pos = h->id_to_pos, *pos_to_id = h->pos_to_id;
  void **heap = h->heap;
  int *mask;

  /* check that this is a binary heap: children is smaller than parent */
  for (i = 1; i < h->len; i++){
    parentPos = ParentPos(i);
    assert((h->cmp)(heap[i], heap[parentPos]) >= 0);
  }

  mask = MALLOC(sizeof(int)*(h->len + IntStack_get_length(h->id_stack)));
  for (i = 0; i < h->len + IntStack_get_length(h->id_stack); i++) mask[i] = -1;

  /* check that spare keys has negative id_to_pos mapping */
  for (i = 0; i <= h->id_stack->last; i++){
    key_spare = h->id_stack->stack[i];
    assert(h->id_to_pos[key_spare] < 0);
    mask[key_spare] = 1;/* mask spare ID */
  }

  /* check that  
     pos_to_id[id_to_pos[i]] = i, for i not in the id_stack & i < length(id_stack)+len
     id_to_pos[pos_to_id[i]] = i, 0 <= i < len
  */
  for (i = 1; i < h->len; i++){
    assert(mask[pos_to_id[i]] == -1);/* that id is in use so can't be spare */
    mask[pos_to_id[i]] = 1;
    assert(id_to_pos[pos_to_id[i]] == i);
  }

  /* all IDs, spare or in use, are ccounted for and give a contiguous set */
  for (i = 0; i < h->len + IntStack_get_length(h->id_stack); i++) assert(mask[i] =- 1);

  FREE(mask);
}
void BinaryHeap_print(BinaryHeap h, void (*pnt)(void*)){
  int i, k = 2;

  for (i = 0; i < h->len; i++){
    pnt(h->heap[i]);
    fprintf(stderr, "(%d) ",(h->pos_to_id)[i]);
    if (i == k-2) {
      fprintf(stderr, "\n");
      k *= 2;
    }
  }
  fprintf(stderr, "\nSpare keys =");
  for (i = 0; i <= h->id_stack->last; i++){
    fprintf(stderr,"%d(%d) ",h->id_stack->stack[i], h->id_to_pos[h->id_stack->stack[i]]);
  }
  fprintf(stderr, "\n");
}


