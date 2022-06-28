# Package script for LLVM builds OSX
install_binary_artifact_cmake () {
  # Use the brew provided package
  LLVM_VERSION_MAJOR="${LLVM_VERSION/.*/}"
  brew install "cmake"
}

# Check if the binary artifact is installed
is_installed_cmake() {
  if which cmake ; then
    return 0
  fi

  return 1
}