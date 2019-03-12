# Package script for LLVM builds OSX
install_binary_artifact_clang () {
  # Use the brew provided package
  LLVM_VERSION_MAJOR="${LLVM_VERSION/.*/}"
  brew install "llvm\@${LLVM_VERSION_MAJOR}"
}

# Check if the binary artifact is installed
is_installed_clang() {
    # Check if llvm-config with the right version exists
    LLVM_VERSION_MAJOR="${LLVM_VERSION/.*/}"
    [[ -f "/usr/local/opt/llvm@${LLVM_VERSION_MAJOR}/bin/llvm-config" ]]
}

setup_artifact_variables_clang() {
  local LLVM_VERSION_MAJOR="${LLVM_VERSION/.*/}"

  BITCODE_CC="/usr/local/opt/llvm@${LLVM_VERSION_MAJOR}/bin/clang"
  BITCODE_CXX="/usr/local/opt/llvm@${LLVM_VERSION_MAJOR}/bin/clang++"

  SANITIZER_C_COMPILER="${BITCODE_CC}"
  SANITIZER_CXX_COMPILER="${BITCODE_CXX}"
  LLVM_CONFIG="/usr/local/opt/llvm@${LLVM_VERSION_MAJOR}/bin/llvm-config"
}