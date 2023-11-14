# Build scripts for Bitwuzla
# Variables that any artifact of this package might depend on
setup_build_variables_bitwuzla() {
  BITWUZLA_SUFFIX="${SANITIZER_SUFFIX}"

  BITWUZLA_BUILD_PATH="${BASE}/bitwuzla-${BITWUZLA_VERSION}-build${BITWUZLA_SUFFIX}"
  BITWUZLA_INSTALL_PATH="${BASE}/bitwuzla-${BITWUZLA_VERSION}-install${BITWUZLA_SUFFIX}"
  bitwuzla_url="https://github.com/bitwuzla/bitwuzla.git"

  return 0
}

download_bitwuzla() {
  source "${DIR}/common-functions"
  # Download Bitwuzla
  git_clone_or_update "${bitwuzla_url}" "${BASE}/bitwuzla-${BITWUZLA_VERSION}" "${BITWUZLA_VERSION}" "${BITWUZLA_COMMIT}"
}

build_bitwuzla() {
  pushd "${BASE}/bitwuzla-${BITWUZLA_VERSION}"

  args=(--build-dir "${BITWUZLA_BUILD_PATH}" --prefix "${BITWUZLA_INSTALL_PATH}" --static --no-unit-testing)

  if [[ -n "${SANITIZER_SUFFIX:-}" ]]; then
    if [[ "${SANITIZER_BUILD:-}" == "address" ]]; then
      args+=("--asan")
    fi
    if [[ "${SANITIZER_BUILD:-}" == "undefined" ]]; then
      args+=("--ubsan")
    fi
    args+=("--assertions" "debug")
  else
    args+=("--no-assertions" "release")
  fi

  ./configure.py "${args[@]}"
  popd
  cd "${BITWUZLA_BUILD_PATH}" || return 1
  yes n | ninja install
  touch "${BITWUZLA_INSTALL_PATH}/.bitwuzla_installed"
}

install_bitwuzla() {
  return 0
}

# Check if the binary artifact is installed
is_installed_bitwuzla() {
  (
    setup_build_variables_bitwuzla
    [[ -f "${BITWUZLA_INSTALL_PATH}/.bitwuzla_installed" ]]
  ) || return 1
}

setup_artifact_variables_bitwuzla() {
  setup_build_variables_bitwuzla
}

get_build_artifacts_bitwuzla() {
  (
    setup_build_variables_bitwuzla
    echo "${BITWUZLA_INSTALL_PATH}"
  )
}

get_docker_config_id_bitwuzla() {
  (
    source "${DIR}/common-functions"
    setup_build_variables_bitwuzla

    biwuzla_remote_commit="$(get_git_hash "${bitwuzla_url}" "${BITWUZLA_VERSION}")"
    echo "${biwuzla_remote_commit}"
  )
}
