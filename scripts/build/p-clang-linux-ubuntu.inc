install_binary_artifact_clang() {
  local LLVM_VERSION_MAJOR="${LLVM_VERSION/.*/}"
  local LLVM_VERSION_MINOR="${LLVM_VERSION/*./}"

  local version=""
  [[ "${LLVM_VERSION_MAJOR}" -le 6 ]] && version="-${LLVM_VERSION}"
  [[ "${LLVM_VERSION_MAJOR}" -ge 7 ]] && version="-${LLVM_VERSION_MAJOR}"

  source "${DIR}/common-functions"

  # Add certificate
  with_sudo apt update -y
  dependencies=(
    ca-certificates
    wget
    lsb-release
    gnupg
  )
  with_sudo apt -y --no-install-recommends install "${dependencies[@]}"

  # Add LLVM upstream repository if available
  codename="$(lsb_release --codename --short)"
  if wget -q "https://apt.llvm.org/${codename}/dists/llvm-toolchain-${codename}${version}/"; then
    wget -O - https://apt.llvm.org/llvm-snapshot.gpg.key| with_sudo apt-key add -

    apt_entry="deb http://apt.llvm.org/${codename}/ llvm-toolchain-${codename}${version} main"
    if ! grep -rq "${apt_entry}" /etc/apt; then
      echo "${apt_entry}" | with_sudo tee -a /etc/apt/sources.list
      with_sudo apt update -y
    fi
  fi


  with_sudo apt update -y
  dependencies=(
    "llvm${version}"
    "clang${version}"
  )

  #Install essential dependencies
  with_sudo apt -y --no-install-recommends install "${dependencies[@]}" || return 1
}

get_docker_config_id_clang() {
  return 0
}
