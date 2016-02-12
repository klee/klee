#!/bin/bash
make -f Makefile.interpolant $1.klee
make -f Makefile.interpolant clean

