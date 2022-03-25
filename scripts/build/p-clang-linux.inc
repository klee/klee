setup_artifact_variables_clang() {
  local LLVM_VERSION_MAJOR="${LLVM_VERSION/.*/}"
  local LLVM_VERSION_MINOR="${LLVM_VERSION/*./}"
  local version=""
  [[ "${LLVM_VERSION_MAJOR}" -le 6 ]] && version="-${LLVM_VERSION}"
  [[ "${LLVM_VERSION_MAJOR}" -ge 7 ]] && version="-${LLVM_VERSION_MAJOR}"

  # Only set LLVM_CONFIG if not set yet
  if [[ -z "${LLVM_CONFIG:-}" ]]; then
    LLVM_CONFIG=$(which "llvm-config${version}")
  fi


  local bin_path=""
  bin_path="$(dirname "$(readlink -f "$(which "clang${version}")")")"

  BITCODE_CC="${bin_path}/clang"
  BITCODE_CXX="${bin_path}/clang++"
}

# Check if the binary artifact is installed
is_installed_clang() {
  local LLVM_VERSION_MAJOR="${LLVM_VERSION/.*/}"
  local LLVM_VERSION_MINOR="${LLVM_VERSION/*./}"
  local version=""
  [[ "${LLVM_VERSION_MAJOR}" -le 6 ]] && version="${LLVM_VERSION}"
  [[ "${LLVM_VERSION_MAJOR}" -ge 7 ]] && version="${LLVM_VERSION_MAJOR}"

  # Check if clang with the right version exists
  which "clang-${version}"
}