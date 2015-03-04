Building Klee with Yices
=============================

[![Build Status](https://travis-ci.org/SRI-CSL/klee.svg?branch=master)](https://travis-ci.org/SRI-CSL/klee)

`Yices` is an alternative SMT solver that can be used with `klee`
instead of the STP solver. To build `klee` with `yices` enabled as 
an SMT solver, follow these steps: 

  1. Download and install `yices` from its
     [website](http://yices.csl.sri.com/).  Version 2.3.0 or later is
     required.

  2. Follow the build instructions for `klee` as described 
     [here](http://klee.github.io/experimental/) adding this 
     extra flag to the step that configures `klee` (07):

```
 --with-yices=<where you installed yices>
```

for example, if you installed `yices` in its default location,
you would add:


```
 --with-yices=/usr/local
```

After successfully building `klee` with `yices` you can select `yices`
as the solver to be used by passing `klee` the extra commandline 
argument `-use-yices`.