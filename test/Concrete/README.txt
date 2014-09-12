This directory contains tests which exist just to test the execution of Concrete
code paths -- essentially, that we correctly implement the semantics of the LLVM
IR language. The tests are run using a helper script ``ConcreteTest.py`` which
builds the test bitcode, executes it using both ``lli`` and ``klee``, and checks
that they got the same output.