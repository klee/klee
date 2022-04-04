#===------------------------------------------------------------------------===#
#
#                     The KLEE Symbolic Virtual Machine
#
# This file is distributed under the University of Illinois Open Source
# License. See LICENSE.TXT for details.
#
#===------------------------------------------------------------------------===#

###############################################################################
# Compiler warnings
###############################################################################
# FIXME: -Wunused-parameter fires a lot so for now suppress it.
add_compile_options(
  "-Wall"
  "-Wextra"
  "-Wno-unused-parameter"
)

###############################################################################
# Warnings as errors
###############################################################################
option(WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)
if (WARNINGS_AS_ERRORS)
  add_compile_options("-Werror")
  message(STATUS "Treating compiler warnings as errors")
else()
  message(STATUS "Not treating compiler warnings as errors")
endif()
