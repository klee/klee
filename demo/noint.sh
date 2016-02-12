#!/bin/bash
make -f Makefile.nointerpolant $1.klee
make -f Makefile.nointerpolant clean

