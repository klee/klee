/*
 * First KLEE tutorial: testing a small function
 */

#include <klee/klee.h>

int my_islower(int x) {
  if (x >= 'a' && x <= 'z')
    return 1;
  else return 0;
}

int main() {
  char c;
  klee_make_symbolic(&c, sizeof(c), "input");
  return my_islower(c);
}
