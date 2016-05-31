/*
 * First KLEE tutorial: testing a small function
 */

#include <klee/klee.h>

int my_islower(int x) {
  int y[20];
  if (x >= 0 && x <= 20)
    return y[x];
  else return 0;
}

int main() {
  int c = 0xff000000;
  klee_make_symbolic(&c, sizeof(c), "inputc");
  return my_islower(c);
}
