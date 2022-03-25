install_binary_artifact_sanitizer() {
  if [[ -z "${SANITIZER_BUILD:=}" ]]; then
    return 1
  fi

  source "${DIR}/common-functions"
  local SANITIZER_LLVM_VERSION_MAJOR="${SANITIZER_LLVM_VERSION/.*/}"
  local SANITIZER_LLVM_VERSION_MINOR="${SANITIZER_LLVM_VERSION/*./}"

  local version=""
  [[ "${SANITIZER_LLVM_VERSION_MAJOR}" -le 6 ]] && version="-${SANITIZER_LLVM_VERSION}"
  [[ "${SANITIZER_LLVM_VERSION_MAJOR}" -ge 7 ]] && version="-${SANITIZER_LLVM_VERSION_MAJOR}"

  # Add certificate
  with_sudo apt update -y
  dependencies=(
    ca-certificates
    wget
    lsb-release
    gnupg
  )

  with_sudo apt -y --no-install-recommends install "${dependencies[@]}"
  wget -O - https://apt.llvm.org/llvm-snapshot.gpg.key | with_sudo apt-key add -

  # Add repository
  codename="$(lsb_release --codename --short)"
  apt_entry="deb http://apt.llvm.org/${codename}/ llvm-toolchain-${codename}${version} main"
  if [[ ! $(grep -rq "${apt_entry}" /etc/apt) ]]; then
    echo "${apt_entry}" | with_sudo tee -a /etc/apt/sources.list
    with_sudo apt update -y
  fi

  with_sudo apt update -y
  dependencies=(
    "llvm${version}"
    "clang${version}"
  )

  #Install essential dependencies
  with_sudo apt -y --no-install-recommends install "${dependencies[@]}" || return 1
}

get_docker_config_id_sanitizer() {
  return 0
}
