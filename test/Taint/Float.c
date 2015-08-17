// RUN: %llvmgcc %s -emit-llvm -g -O0 -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee -taint=direct --output-dir=%t.klee-out -exit-on-error %t.bc

#include<klee/klee.h>
void
test_FAdd ()
{
  float a = 1.0;
  float b = 100.0;
  float c = 0.0;
  klee_set_taint (1, &a, sizeof (a));
  klee_set_taint (2, &b, sizeof (b));
  c = a + b;
  klee_assert (klee_get_taint (&c, sizeof (c)) == (1 | 2));
}

void
test_FSub ()
{
  float a = 1.0;
  float b = 100.0;
  float c = 0.0;
  klee_set_taint (1, &a, sizeof (a));
  klee_set_taint (2, &b, sizeof (b));
  c = a - b;
  klee_assert (klee_get_taint (&c, sizeof (c)) == (1 | 2));
}

void
test_FMul ()
{
  float a = 1.0;
  float b = 100.0;
  float c = 0.0;
  klee_set_taint (1, &a, sizeof (a));
  klee_set_taint (2, &b, sizeof (b));
  c = a * b;
  klee_assert (klee_get_taint (&c, sizeof (c)) == (1 | 2));

}

void
test_FDiv ()
{
  float a = 1.0;
  float b = 100.0;
  float c = 0.0;
  klee_set_taint (1, &a, sizeof (a));
  klee_set_taint (2, &b, sizeof (b));
  c = a / b;
  klee_assert (klee_get_taint (&c, sizeof (c)) == (1 | 2));

}

void
test_FRem ()
{
  // y esto?
}

void
test_FPTrunc ()
{
  double a = 1.0;
  float c = 0.0;
  klee_set_taint (4, &a, sizeof (a));
  c = a ;
  klee_assert (klee_get_taint (&c, sizeof (c)) ==  4 );
}

void
test_FPExt ()
{
  float a = 1.0;
  double c = 0.0;
  klee_set_taint (1, &a, sizeof (a));
  c = a ;
  klee_assert (klee_get_taint (&c, sizeof (c)) == 1 );
}

void
test_FPToUI ()
{
  float a = 1.0;
  unsigned int c = 0;
  klee_set_taint (1, &a, sizeof (a));
  c = a ;
  klee_assert (klee_get_taint (&c, sizeof (c)) == 1 );
}

void
test_FPToSI ()
{
  float a = 1.0;
  int c = 0;
  klee_set_taint (1, &a, sizeof (a));
  c = a ;
  klee_assert (klee_get_taint (&c, sizeof (c)) == 1 );
}

void
test_UIToFP ()
{
  float c = 1.0;
  unsigned int a = 0;
  klee_set_taint (1, &a, sizeof (a));
  c = a ;
  klee_assert (klee_get_taint (&c, sizeof (c)) == 1 );
}

void
test_SIToFP ()
{
  float c = 1.0;
  int a = 0;
  klee_set_taint (1, &a, sizeof (a));
  c = a ;
  klee_assert (klee_get_taint (&c, sizeof (c)) == (1 ));
}

void
test_FCmp ()
{
  float a = 1.0;
  float b = 2.0;
  float c = 10.0;
  float d = 20.0;
  float result;
  klee_set_taint (1, &a, sizeof (a));
  klee_set_taint (2, &b, sizeof (b));
  klee_set_taint (4, &c, sizeof (c));
  klee_set_taint (8, &d, sizeof (d));
  if (a>b)
    result=c;
  else
    result=d;
  klee_assert (klee_get_taint (&result, sizeof (result)) == 8);
}

int 
main (int argc, char *argv[])
{
    test_FAdd ();
    test_FSub ();
    test_FMul ();
    test_FDiv ();
    test_FPTrunc ();
    test_FPExt ();
    test_FPToUI ();
    test_FPToSI ();
    test_UIToFP ();
    test_SIToFP ();
    test_FCmp ();
}
