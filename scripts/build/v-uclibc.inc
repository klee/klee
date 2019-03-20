required_variables_uclibc=(
  "UCLIBC_VERSION"
  "LLVM_VERSION"
)

artifact_dependency_uclibc(){
  # Add llvm if needed; otherwise only use clang package
# TODO this is quite distribution specific; should be handled in a more general case
  echo "clang"
}
