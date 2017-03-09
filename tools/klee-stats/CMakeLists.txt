#===------------------------------------------------------------------------===#
#
#                     The KLEE Symbolic Virtual Machine
#
# This file is distributed under the University of Illinois Open Source
# License. See LICENSE.TXT for details.
#
#===------------------------------------------------------------------------===#
install(PROGRAMS klee-stats DESTINATION bin)

# Copy into the build directory's binary directory
# so system tests can find it
configure_file(klee-stats "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/klee-stats" COPYONLY)
