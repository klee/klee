install_binary_artifact_clang() {
  local LLVM_VERSION_MAJOR="${LLVM_VERSION/.*/}"
  local LLVM_VERSION_MINOR="${LLVM_VERSION/*./}"

  # Check versions: no support for LLVM < 3.5
  [[ "${LLVM_VERSION_MAJOR}" -eq 3 ]] && [[ "${LLVM_VERSION_MINOR}" -lt 5 ]] && return 1


  local version=""
  [[ "${LLVM_VERSION_MAJOR}" -le 6 ]] && version="-${LLVM_VERSION}"
  [[ "${LLVM_VERSION_MAJOR}" -ge 7 ]] && version="-${LLVM_VERSION_MAJOR}"


  if [[ "${LLVM_VERSION_MAJOR}" -eq 3 && "${LLVM_VERSION_MINOR}" -gt 7 ]]; then
    # Add certificate
    apt update -y
    dependencies=(
      ca-certificates
      wget
    )

    apt -y --no-install-recommends install "${dependencies[@]}"
    wget -O - https://apt.llvm.org/llvm-snapshot.gpg.key| apt-key add -

    # Add repository
    echo "deb http://apt.llvm.org/xenial/ llvm-toolchain-xenial${version} main" >> /etc/apt/sources.list
  fi

  apt update -y
  dependencies=(
    "llvm${version}"
    "clang${version}"
  )

  #Install essential dependencies
  apt -y --no-install-recommends install "${dependencies[@]}"
}

setup_artifact_variables_clang() {
  local LLVM_VERSION_MAJOR="${LLVM_VERSION/.*/}"
  local LLVM_VERSION_MINOR="${LLVM_VERSION/*./}"
  local version=""
  [[ "${LLVM_VERSION_MAJOR}" -le 6 ]] && version="-${LLVM_VERSION}"
  [[ "${LLVM_VERSION_MAJOR}" -ge 7 ]] && version="-${LLVM_VERSION_MAJOR}"

  # Only set LLVM_CONFIG if not set yet
  if [[ -z ${LLVM_CONFIG+x} ]]; then
    LLVM_CONFIG=$(which "llvm-config${version}")
  fi

  BITCODE_CC=$(which "clang${version}")
  BITCODE_CXX=$(which "clang++${version}")

  SANITIZER_C_COMPILER="${BITCODE_CC}"
  SANITIZER_CXX_COMPILER="${BITCODE_CXX}"
}

# Check if the binary artifact is installed
is_installed_clang() {
    # Check if llvm-config with the right version exists    
    which "llvm-config-${LLVM_VERSION}"
}

get_docker_config_id_clang() {
  return 0
}
