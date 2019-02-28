# Build scripts for uclibc
setup_build_variables_uclibc() {
  UCLIBC_PATH="${BASE}/klee-uclibc-${LLVM_VERSION_SHORT}"
  uclibc_url="https://github.com/klee/klee-uclibc.git"
}

download_uclibc() {
  source "${DIR}/common-functions"
  git_clone_or_update "${uclibc_url}" "${UCLIBC_PATH}" "${UCLIBC_VERSION}"
}

build_uclibc() { 
  cd "${UCLIBC_PATH}" || return 1
  ./configure --make-llvm-lib --with-cc "${BITCODE_CC}" --with-llvm-config "${LLVM_CONFIG}"
  make || return 1
  touch .is_installed
}

install_uclibc() {
  return 0  
}

# Check if the binary artifact is installed
is_installed_uclibc() {
  (
    setup_build_variables_uclibc
    [[ -f "${UCLIBC_PATH}"/.is_installed ]]
  ) || return 1
}

get_docker_config_id_uclibc() {
  (
    source "${DIR}/common-functions"
    setup_build_variables_uclibc
    uclibc_remote_commit="$(get_git_hash "${uclibc_url}" "${UCLIBC_VERSION}")"
    echo "${uclibc_remote_commit}_${LLVM_VERSION_SHORT}"
  )
}

get_build_artifacts_uclibc() {
  (
    setup_build_variables_uclibc
    echo "${UCLIBC_PATH}"
  )
}

setup_artifact_variables_uclibc() {
  setup_build_variables_uclibc
}