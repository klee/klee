# Meta-package for solvers
required_variables_sanitizer_compiler=("")

# On which artifacts does STP depend on
artifact_dependency_sanitizer_compiler(){
  echo "sanitizer"
  if [[ -n ${SANITIZER_BUILD+x} ]]; then
    [[ "${SANITIZER_BUILD}" == "memory" ]] && echo "llvm"
    [[ "${SANITIZER_BUILD}" != "memory" ]] && echo "clang"
  fi
}


# Setup general variables
setup_variables_sanitizer_compiler() {
  for sanitizer in "${SANITIZERS[@]}"; do
    [[ -z "${sanitizer}" ]] && continue
    SANITIZER_CMAKE_C_COMPILER=("")
  done
}