/* sum_list.cpp
Тест на списке (суммирование).
*/
#include <stdlib.h>
#include "klee/klee.h"

typedef struct node {
  struct node *next;
  unsigned data;
} *SLL;

unsigned sum_n(SLL s, unsigned n) {
  SLL res = s;
  unsigned sum = 0;
  while (res && n > 0) {
    sum += res->data;
    res = res->next;
    n--;
  }
  return sum;
}

#define SIZE 6

int main() {
  SLL s;
  klee_make_symbolic(&s, sizeof(s), "s");
  unsigned res = sum_n(s, SIZE);
  return 0;
}

