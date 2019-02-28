required_variables_llvm=(
  "LLVM_VERSION"
  "ENABLE_OPTIMIZED"
  "ENABLE_DEBUG"
  "DISABLE_ASSERTIONS"
  "REQUIRES_RTTI"
)

required_variables_check_llvm() {
  check_bool "ENABLE_DEBUG"
  check_bool "ENABLE_OPTIMIZED"
  check_bool "DISABLE_ASSERTIONS"
  check_bool "REQUIRES_RTTI"
}

export_variables_llvm=(
  "LLVM_CONFIG"
  "BITCODE_CC"
  "BITCODE_CXX"
)

artifact_dependency_llvm=("sanitizer")

setup_variables_llvm() {
  LLVM_VERSION_SHORT="${LLVM_VERSION/./}"
}

check_export_variables_llvm() {
  # Check for variables set and not empty
  [[ -n ${LLVM_CONFIG:-} ]] || (echo "LLVM_CONFIG not set"; exit 1)
  [[ -n ${LLVM_INSTALL:-} ]] || (echo "LLVM_INSTALL not set"; exit 1)
  [[ -n ${BITCODE_CC:-} ]] || (echo "BITCODE_CC not set"; exit 1)
  [[ -n ${BITCODE_CXX:-} ]] || (echo "BITCODE_CXX not set"; exit 1)
}