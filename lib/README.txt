The klee and kleaver code is organized as follows:

lib/Basic   - Low level support for both klee and kleaver which should
              be independent of LLVM.

lib/Support - Higher level support, but only used by klee. This can
              use LLVM facilities.

lib/Expr    - The core kleaver expression library.

lib/Solver  - The kleaver solver library.

lib/Module  - klee facilities for working with LLVM modules, including
              the shadow module/instruction structures we use during
              execution.

lib/Core    - The core symbolic virtual machine.

