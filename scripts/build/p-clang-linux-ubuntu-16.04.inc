install_binary_artifact_clang() {
  local LLVM_VERSION_MAJOR="${LLVM_VERSION/.*/}"
  local LLVM_VERSION_MINOR="${LLVM_VERSION/*./}"

  local version=""
  [[ "${LLVM_VERSION_MAJOR}" -le 6 ]] && version="-${LLVM_VERSION}"
  [[ "${LLVM_VERSION_MAJOR}" -ge 7 ]] && version="-${LLVM_VERSION_MAJOR}"

  # apt.llvm packages for Trusty clang 5.0 package is broken with atomics. Don't use it.
  if [[ "${LLVM_VERSION_MAJOR}" -gt 6 ]]; then
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

get_docker_config_id_clang() {
  return 0
}
