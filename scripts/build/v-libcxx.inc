required_variables_libcxx=(
  "LLVM_VERSION"
)

required_variables_check_libcxx() {
  local LLVM_VERSION_MAJOR="${LLVM_VERSION/.*/}"
  local LLVM_VERSION_MINOR="${LLVM_VERSION/*./}"
  # Check versions: no support for LLVM < 3.8
#  if [[ "${LLVM_VERSION_MAJOR}" -eq 3 ]]; then
#    [[ "${LLVM_VERSION_MINOR}" -lt 8 ]] && { echo "Version <= 3.7 not supported"; return 1; }
#  fi
  return 0
}

artifact_dependency_libcxx(){
  echo "clang"
}