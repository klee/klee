install_binary_artifact_sanitizer() {
  if [[ -z "${SANITIZER_BUILD:-}" ]]; then
    return 1
  fi

  source "${DIR}/common-functions"
  local SANITIZER_LLVM_VERSION_MAJOR="${SANITIZER_LLVM_VERSION/.*/}"
  local version="-${SANITIZER_LLVM_VERSION_MAJOR}"

  local LLVM_UPSTREAM_USED=0
  # Check if package in standard repository otherwise use upstream
  with_sudo apt-get update -y
  if ! apt-cache show "llvm${version}"; then
    if [[ -z "$(which wget)" ]]; then
      # Add certificate
      with_sudo apt-get update -y
      dependencies=(
        ca-certificates
        wget
        lsb-release
        gnupg
      )
      with_sudo apt-get -y --no-install-recommends install "${dependencies[@]}"
    fi

    # Add LLVM upstream repository if available
    codename="$(lsb_release --codename --short)"
    if wget -q "https://apt.llvm.org/${codename}/dists/llvm-toolchain-${codename}${version}/"; then
      LLVM_UPSTREAM_USED=1
      apt_entry="deb http://apt.llvm.org/${codename}/ llvm-toolchain-${codename}${version} main"
      if ! grep -rq "${apt_entry}" /etc/apt; then
        wget -O - https://apt.llvm.org/llvm-snapshot.gpg.key| with_sudo apt-key add -
        echo "${apt_entry}" | with_sudo tee -a /etc/apt/sources.list
        with_sudo apt-get update -y
      fi
    fi
  fi

  dependencies=(
    "llvm${version}"
    "clang${version}"
  )

  if [[ "${SANITIZER_LLVM_VERSION_MAJOR}" -ge 14 && "${LLVM_UPSTREAM_USED}" -eq 1 ]]; then
    dependencies+=("libclang-rt${version}-dev")
  fi

  #Install essential dependencies
  with_sudo apt-get -y --no-install-recommends install "${dependencies[@]}" || return 1
}

get_docker_config_id_sanitizer() {
  return 0
}
