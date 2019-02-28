required_variables_clang=(
  "LLVM_VERSION"
)

artifact_dependency_clang=("sanitizer")

setup_variables_clang() {
  LLVM_VERSION_SHORT="${LLVM_VERSION/./}"
}

export_variables_clang=(
  "LLVM_CONFIG"
  "BITCODE_CC"
  "BITCODE_CXX"
)