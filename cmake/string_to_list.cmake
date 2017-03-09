#===------------------------------------------------------------------------===#
#
#                     The KLEE Symbolic Virtual Machine
#
# This file is distributed under the University of Illinois Open Source
# License. See LICENSE.TXT for details.
#
#===------------------------------------------------------------------------===#

function(string_to_list s output_var)
  string(REPLACE " " ";" _output "${s}")
  set(${output_var} ${_output} PARENT_SCOPE)
endfunction()
