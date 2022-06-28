#===------------------------------------------------------------------------===#
#
#                     The KLEE Symbolic Virtual Machine
#
# This file is distributed under the University of Illinois Open Source
# License. See LICENSE.TXT for details.
#
#===------------------------------------------------------------------------===#

set(OZERO_OPT "-O0 -Xclang -disable-O0-optnone")
configure_file(Makefile.cmake.test.in Makefile.cmake.test @ONLY)
