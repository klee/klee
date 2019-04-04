setup_build_variables_metasmt() {
  METASMT_SRC_PATH="${BASE}/metaSMT-${METASMT_VERSION}"
  METASMT_BUILD_PATH="${BASE}/metaSMT-${METASMT_VERSION}-build"
  METASMT_INSTALL_PATH="${BASE}/metaSMT-${METASMT_VERSION}-install"
  METASMT_DEPS_PATH="${BASE}/metaSMT-${METASMT_VERSION}-deps"
}

download_metasmt() {
  source "${DIR}/common-functions"
  # Clone
  git_clone_or_update https://github.com/hoangmle/metaSMT.git "${METASMT_SRC_PATH}" "${METASMT_VERSION}"
  cd "${METASMT_SRC_PATH}"
  git submodule update --init
  # Bootstrap
  git_clone_or_update https://github.com/agra-uni-bremen/dependencies.git "${METASMT_SRC_PATH}/dependencies" "development"
}

build_metasmt() {
  # MetaSMT might need to cherry-pick some git commits, but git requires email and user to be set to do this operation
  # Provide a temporary value for this
  export GIT_COMMITTER_EMAIL="foo@bar"
  export GIT_COMMITTER_NAME="foobar"
  cd "${METASMT_SRC_PATH}"
  ./bootstrap.sh -d deps -m RELEASE build -DmetaSMT_ENABLE_TESTS=off \
    --build stp-git-basic --build boolector-3.0.0 --build minisat-git \
    --build lingeling-bbc-9230380-161217 --build yices-2.6.1 --build Z3-4.8.4 \
    --build cvc4-1.6 \
    --deps "${METASMT_DEPS_PATH}/" \
    --install "${METASMT_INSTALL_PATH}/" \
    -j "$(nproc)" \
    "${METASMT_BUILD_PATH}"

  # Build
  cd "${METASMT_BUILD_PATH}"
  make
}

install_metasmt() {
  cd "${METASMT_BUILD_PATH}"
  make install
  touch "${METASMT_INSTALL_PATH}"/.is_installed
}

get_docker_config_id_metasmt() {
  echo "${METASMT_VERSION}"
}

# Check if the binary artifact is installed
is_installed_metasmt() {
  (
  setup_build_variables_metasmt
  [[ -f "${METASMT_INSTALL_PATH}"/.is_installed ]]
  )
}

get_docker_config_id_metasmt() {
  (
    echo "${METASMT_VERSION}"
  )
}

get_build_artifacts_metasmt() {
  (
    setup_build_variables_metasmt
    echo "${METASMT_INSTALL_PATH}/"
    echo "${METASMT_BUILD_PATH}/"
    echo "${METASMT_DEPS_PATH}/"
  )
}