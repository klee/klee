required_variables_uclibc=(
  "UCLIBC_VERSION"
  "LLVM_VERSION"
)

artifact_dependency_uclibc(){
  # Add llvm if needed; otherwise only use clang package
# TODO this is quite distribution specific; should be handled in a more general case
  local LLVM_VERSION_MAJOR="${LLVM_VERSION/.*/}"
  local LLVM_VERSION_MINOR="${LLVM_VERSION/*./}"
  # Check versions: no support for LLVM < 3.5
  if [[ "${LLVM_VERSION_MAJOR}" -eq 3 ]]; then
    [[ "${LLVM_VERSION_MINOR}" -lt 5 ]] && echo "llvm"
  fi

  echo "clang"
}