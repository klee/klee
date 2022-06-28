required_variables_clang=(
  "LLVM_VERSION"
)

artifact_dependency_clang=("cmake")

setup_variables_clang() {
  local v_a
  v_a=(${LLVM_VERSION//./ })

  LLVM_VERSION_MAJOR="0"
  LLVM_VERSION_MINOR="0"

  if [[ "${#v_a[@]}" -ge 1 ]]; then
    LLVM_VERSION_MAJOR="${v_a[0]}"
  fi
  if [[ "${#v_a[@]}" -ge 2 ]]; then
    LLVM_VERSION_MINOR="${v_a[1]}"
  fi

  LLVM_VERSION_SHORT="${LLVM_VERSION_MAJOR}${LLVM_VERSION_MINOR}"
}

export_variables_clang=(
  "LLVM_CONFIG"
  "BITCODE_CC"
  "BITCODE_CXX"
)