required_variables_libcxx=(
  "LLVM_VERSION"
)

required_variables_check_libcxx() {
  return 0
}

artifact_dependency_libcxx(){
  echo "clang"
}
