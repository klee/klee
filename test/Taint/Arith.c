// RUN: %llvmgcc %s -emit-llvm -g -O0 -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee -taint=direct --output-dir=%t.klee-out -exit-on-error %t.bc

#include<klee/klee.h>

void
test_Add ()
{
  int a = 1;
  int b = 100;
  int c = 0;
  klee_set_taint (1, &a, sizeof (a));
  klee_set_taint (2, &b, sizeof (b));
  c = a + b;
  klee_assert (klee_get_taint (&c, sizeof (c)) == (1 | 2));
}

void
test_Sub ()
{
  int a = 1;
  int b = 100;
  int c = 0;
  klee_set_taint (1, &a, sizeof (a));
  klee_set_taint (2, &b, sizeof (b));
  c = a - b;
  klee_assert (klee_get_taint (&c, sizeof (c)) == (1 | 2));
}

void
test_Mul ()
{
  int a = 1;
  int b = 100;
  int c = 0;
  klee_set_taint (1, &a, sizeof (a));
  klee_set_taint (2, &b, sizeof (b));
  c = a * b;
  klee_assert (klee_get_taint (&c, sizeof (c)) == (1 | 2));

}

void
test_UDiv ()
{
  int a = 1;
  int b = 100;
  int c = 0;
  klee_set_taint (1, &a, sizeof (a));
  klee_set_taint (2, &b, sizeof (b));
  c = a / b;
  klee_assert (klee_get_taint (&c, sizeof (c)) == (1 | 2));

}

void
test_SDiv ()
{
  int a = -1;
  int b = 100;
  int c = 0;
  klee_set_taint (1, &a, sizeof (a));
  klee_set_taint (2, &b, sizeof (b));
  c = a / b;
  klee_assert (klee_get_taint (&c, sizeof (c)) == (1 | 2));
}

void
test_URem ()
{
  unsigned int a = 120;
  unsigned int b = 100;
  unsigned int c = 0;
  klee_set_taint (1, &a, sizeof (a));
  klee_set_taint (2, &b, sizeof (b));
  c = a % b;
  klee_assert (klee_get_taint (&c, sizeof (c)) == (1 | 2));
}

void
test_SRem ()
{
  int a = 120;
  int b = 100;
  int c = 0;
  klee_set_taint (1, &a, sizeof (a));
  klee_set_taint (2, &b, sizeof (b));
  c = a % b;
  klee_assert (klee_get_taint (&c, sizeof (c)) == (1 | 2));
}

void
test_And ()
{
  int a = 1;
  int b = 100;
  int c = 0;
  klee_set_taint (1, &a, sizeof (a));
  klee_set_taint (2, &b, sizeof (b));
  c = a & b;
  printf("C taint: %d\n", klee_get_taint (&c, sizeof (c)));
  klee_assert (klee_get_taint (&c, sizeof (c)) == (1 | 2));

}

void
test_Or ()
{
  int a = 1;
  int b = 100;
  int c = 0;
  klee_set_taint (1, &a, sizeof (a));
  klee_set_taint (2, &b, sizeof (b));
  c = a | b;
  klee_assert (klee_get_taint (&c, sizeof (c)) == (1 | 2));

}

void
test_Xor ()
{
  int a = 1;
  int b = 100;
  int c = 0;
  klee_set_taint (1, &a, sizeof (a));
  klee_set_taint (2, &b, sizeof (b));
  c = a ^ b;
  klee_assert (klee_get_taint (&c, sizeof (c)) == (1 | 2));

}

void
test_Shl ()
{
  int a = 1;
  int b = 8;
  int c = 0;
  klee_set_taint (1, &a, sizeof (a));
  klee_set_taint (2, &b, sizeof (b));
  c = a << b;
  klee_assert (klee_get_taint (&c, sizeof (c)) == (1 | 2));

}

void
test_LShr ()
{
  int a = 1;
  int b = 8;
  int c = 0;
  klee_set_taint (1, &a, sizeof (a));
  klee_set_taint (2, &b, sizeof (b));
  c = a >> b;
  klee_assert (klee_get_taint (&c, sizeof (c)) == (1 | 2));

}

void
test_AShr ()
{
  int a = -1;
  int b = 1;
  int c = 0;
  klee_set_taint (1, &a, sizeof (a));
  klee_set_taint (2, &b, sizeof (b));
  c = a >> b;
  klee_assert (klee_get_taint (&c, sizeof (c)) == (1 | 2));
}

void
test_ICmp ()
{
  int a = 1;
  int b = 100;
  int c = 0;
  klee_set_taint (1, &a, sizeof (a));
  klee_set_taint (2, &b, sizeof (b));
  c = a == b;
  klee_assert (klee_get_taint (&c, sizeof (c)) == (1 | 2));

}


int 
main (int argc, char *argv[])
{
  test_Add ();
  test_Sub ();
  test_Mul ();
  test_UDiv ();
  test_SDiv ();
  test_URem ();
  test_SRem ();
  test_And ();
  test_Or ();
  test_Xor ();
  test_Shl ();
  test_LShr ();
  test_AShr ();
  test_ICmp ();
return 0;
}
