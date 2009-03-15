//===----------------------------------------------------------------------===//
// Klee Symbolic Virtual Machine
//===----------------------------------------------------------------------===//
                                                             Daniel Dunbar

klee is a symbolic virtual machine built on top of the LLVM compiler
infrastructure. Currently, there are two primary components.

1. The core symbolic virtual machine engine; this is responsible for
executing LLVM bitcode modules with support for symbolic values. This
is comprised of the code in lib/.

2. An emulation layer for the Linux system call interface, with
additional support for making parts of the operating environment
symbolic. This is found in models/simple.

Additionally, there is a simple library in runtime/ which supports
replaying computed inputs on native code. There is a more complicated
library in replay/ which supports running inputs computed as part of
the system call emulation layer natively -- setting up files, pipes,
etc. on the native system to match the inputs that the emulation layer
provided.

For further information, see the docs in www/.
