Tracer-X for KLEE
=================

In this project we build upon experience with [TRACER](http://paella.ddns.comp.nus.edu.sg/tracer) symbolic execution tool to advance further by combining the technology with advanced symbolic execution approaches. Visit the [TRACER-X Homepage](http://paella.ddns.comp.nus.edu.sg/tracerx/).

This early prototype is based on [KLEE](https://github.com/klee/klee): Below we provide its original README. It is still being actively developed by the team and not yet ready for release. Needless to say, we do not have resources to provide technical support. There are several [example programs](https://github.com/feliciahalim/klee-examples) that we use for testing it.

KLEE Symbolic Virtual Machine README
------------------------------------

[![Build Status](https://travis-ci.org/klee/klee.svg?branch=master)](https://travis-ci.org/klee/klee)

`KLEE` is a symbolic virtual machine built on top of the LLVM compiler
infrastructure. Currently, there are two primary components:

  1. The core symbolic virtual machine engine; this is responsible for
     executing LLVM bitcode modules with support for symbolic
     values. This is comprised of the code in lib/.

  2. A POSIX/Linux emulation layer oriented towards supporting uClibc,
     with additional support for making parts of the operating system
     environment symbolic.

Additionally, there is a simple library for replaying computed inputs
on native code (for closed programs). There is also a more complicated
infrastructure for replaying the inputs generated for the POSIX/Linux
emulation layer, which handles running native programs in an
environment that matches a computed test input, including setting up
files, pipes, environment variables, and passing command line
arguments.

Coverage information can be found [here](http://vm-klee.doc.ic.ac.uk:55555/index.html).

For further information, see the [webpage](http://klee.github.io/).
