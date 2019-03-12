setup_artifact_variables_clang() {
  local LLVM_VERSION_MAJOR="${LLVM_VERSION/.*/}"
  local LLVM_VERSION_MINOR="${LLVM_VERSION/*./}"
  local version=""
  [[ "${LLVM_VERSION_MAJOR}" -le 6 ]] && version="-${LLVM_VERSION}"
  [[ "${LLVM_VERSION_MAJOR}" -ge 7 ]] && version="-${LLVM_VERSION_MAJOR}"

  # Only set LLVM_CONFIG if not set yet
  [[ -z ${LLVM_CONFIG+x} ]] && LLVM_CONFIG=$(which "llvm-config${version}")


  local bin_path=""
  bin_path="$(dirname "$(readlink -f "$(which "clang${version}")")")"

  BITCODE_CC="${bin_path}/clang"
  BITCODE_CXX="${bin_path}/clang++"

  #[[ -z ${SANITIZER_C_COMPILER+x} ]] && 
  SANITIZER_C_COMPILER="${BITCODE_CC}"
  #[[ -z ${SANITIZER_CXX_COMPILER+x} ]] && 
  SANITIZER_CXX_COMPILER="${BITCODE_CXX}"
}

# Check if the binary artifact is installed
is_installed_clang() {
    # Check if llvm-config with the right version exists    
    which "llvm-config-${LLVM_VERSION}"
}