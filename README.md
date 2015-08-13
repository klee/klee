KLEE-TAINT Symbolic Virtual Machine
===================================

The idea is to be able to mark memory containing sensitive data and
track it at runtime.  It allows the user to check if any data (like
crypto keys) is leaked or reaches a certain sink.

KLEE is extended with a couple of special function handlers:

int klee_get_taint (void *message, size_t size); // get taint value from
message
void klee_set_taint (int taint, void *message, size_t size); // set taint
to message

As an example, see here how the taint gets propagated from "a" and "b"
to "c":
```
  int a = 1;
  int b = 100;
  int c = 0;
  klee_set_taint (1, &a, sizeof (a));
  klee_set_taint (2, &b, sizeof (b));
  c = a + b;
  klee_assert (klee_get_taint (&c, sizeof (c)) == (1 | 2));
```
For more advanced taint tracking (like when a conditional brach is
decided using a tainted variable), we taint also the "program counter"
so any future assign gets tainted with the PC. For instance, here "c"
gets tainted indirectly by "a":
```
  int a = 1;
  int c = 0;
  klee_set_taint (1, &a, sizeof (a));
  if (a==1)
            c=1;
  else
            c=0;
  klee_assert (klee_get_taint (&c, sizeof (c)) == 1);
```
The tainting mechanism can be enabled or disabled from command line:
```
$ klee --help |grep taint
  -taint                 - Track tainting (none by default).
       =none             -   Don't track tainting (default)
       =direct           -   Track direct tainting assignments
       =control-flow     -   Track control-flow indirect tainting
       =regions          -   Region-based tainting(EXPERIMENTAL)
```
(The "regions" mode uses LLVM SESE region analysis to turn off the
PC taint when the control flow converges to a common point after a
conditional branch.)


