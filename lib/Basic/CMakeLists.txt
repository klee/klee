#===------------------------------------------------------------------------===#
#
#                     The KLEE Symbolic Virtual Machine
#
# This file is distributed under the University of Illinois Open Source
# License. See LICENSE.TXT for details.
#
#===------------------------------------------------------------------------===#
klee_add_component(kleeBasic
	CmdLineOptions.cpp
	ConstructSolverChain.cpp
	KTest.cpp
	Statistics.cpp
)
